/* 
 * This file is part of the PGMS distribution (https://github.com/genesissoftware-tech/pgms or https://ip-147-251-124-124.flt.cloud.muni.cz/chemdb/pgms).
 * Copyright (c) 2022 Marek Mosna (marek.mosna@genesissoftware.eu).
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <postgres.h>
#include <fmgr.h>
#include <math.h>
#include <float.h>
#include <utils/float.h>
#include <utils/array.h>
#include <utils/lsyscache.h>

#define DALTON_TYPE "Dalton"
#define PPM_TYPE    "ppm"

PG_MODULE_MAGIC;

typedef enum {
    DALTON,
    PPM
} tolerance_type_e;

static float4 Dalton(float4 ref_precursor, float4 query_precursor, float4 tolerance)
{
    float4 dif = fabsf(float4_mi(ref_precursor, query_precursor));
    return float4_le(dif, tolerance) ? 1.0f : 0.0f;
}

static float4 ppm(float4 ref_precursor, float4 query_precursor, float4 tolerance)
{
    float4 dif = fabsf(float4_mi(ref_precursor, query_precursor));
    float4 mean = fabsf(float4_pl(ref_precursor, query_precursor)) / 2;
    return float4_le(
            float4_div(dif
                , float4_mul(mean,1e6f))
            , tolerance) ? 1.0f : 0.0f;
}

PG_FUNCTION_INFO_V1(precurzor_mz_match);
Datum precurzor_mz_match(PG_FUNCTION_ARGS)
{
    const float4 reference = PG_GETARG_FLOAT4(0);
    const float4 query = PG_GETARG_FLOAT4(1);
    const float4 tolerance = PG_GETARG_FLOAT4(2);
    VarChar* tolerance_type = PG_GETARG_VARCHAR_P(3);
    tolerance_type_e type = !strcmp(VARDATA(tolerance_type), DALTON_TYPE)? DALTON : PPM;
    float4 result = 0.0f;

    // elog(DEBUG1, "%f in %f by %s", dif, tolerance, VARDATA(tolerance_type));

    if(type == DALTON)
        result = Dalton(reference, query, tolerance);
    else
        result = ppm(reference, query, tolerance);

    PG_RETURN_FLOAT4(result);
}

PG_FUNCTION_INFO_V1(precurzor_mz_match_array);
Datum precurzor_mz_match_array(PG_FUNCTION_ARGS)
{
    int ndims;
    int *dims;
    int nitems;
    size_t reference_len;
    size_t query_len;
    float4 match = 0.0f;
    ArrayType *result = NULL;
    ArrayType *reference = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *query = PG_GETARG_ARRAYTYPE_P(1);
    const float4 tolerance = PG_GETARG_FLOAT4(2);
    VarChar* tolerance_type = PG_GETARG_VARCHAR_P(3);
    bool is_symetric = PG_GETARG_BOOL(4);
    tolerance_type_e type = !strcmp(VARDATA(tolerance_type), DALTON_TYPE)? DALTON : PPM;
    Datum *elems = NULL;
    bool *nulls = NULL;
    int16 elmlen = 0;
    bool elmbyval = true;
    char elmalign = ' ';
    float4 *reference_precursors = NULL;
    float4 *query_precursors = NULL;
    int *lbs = NULL;

    ndims = ARR_NDIM(reference);
    dims = ARR_DIMS(reference);
    nitems = ArrayGetNItems(ndims, dims);
    reference_len = nitems / ndims;
    reference_precursors = (float4 *) ARR_DATA_PTR(reference);

    ndims = ARR_NDIM(query);
    dims = ARR_DIMS(query);
    nitems = ArrayGetNItems(ndims, dims);
    query_len = nitems / ndims;
    query_precursors = (float4 *) ARR_DATA_PTR(query);

    elems = (Datum*) palloc(query_len * reference_len * sizeof(Datum));
    nulls = (bool*) palloc(query_len * reference_len * sizeof(bool));
    dims = (int*) palloc0(2 * sizeof(int));
    lbs = (int*) palloc0(2 * sizeof(int));

    elog(DEBUG1, "reference precursor mz of %ld "
        "vs query precursor mz of %ld in %f tolerance by %s tolerance type"
        , reference_len, query_len, tolerance, VARDATA(tolerance_type));

    for(Index reference_index = 0; reference_index < reference_len; reference_index++)
        for(Index query_index = (is_symetric ? reference_index : 0); query_index < query_len; query_index++)
        {
            Index result_index = reference_index *query_len + query_index;
            if(type == DALTON)
                match = Dalton(reference_precursors[reference_index], query_precursors[query_index], tolerance);
            else
                match = ppm(reference_precursors[reference_index], query_precursors[query_index], tolerance);

            elems[result_index] = Float4GetDatum(match);
            nulls[result_index] = false;

            elog(DEBUG1, "precursor mz match [%d, %d] {%f, %f} = [%d] {%f}"
                , reference_index, query_index
                , reference_precursors[reference_index]
                , query_precursors[query_index]
                , result_index, match);

            if(is_symetric)
            {
                result_index = query_index*reference_len + reference_index;
                elems[result_index] = Float4GetDatum(match);
                nulls[result_index] = false;
                elog(DEBUG1, "symetric precursor mz match [%d, %d] = [%d] {%f}"
                    , query_index
                    , reference_index
                    , result_index
                    , match);
            }
        }
    dims[0] = reference_len;
    dims[1] = query_len;
    lbs[0] = 1;
    lbs[1] = 1;

    get_typlenbyvalalign(ARR_ELEMTYPE(reference), &elmlen, &elmbyval, &elmalign);
    result = construct_md_array(elems, nulls,2
    , dims, lbs, ARR_ELEMTYPE(reference), elmlen, elmbyval, elmalign);

    pfree(elems);
    pfree(nulls);
    pfree(dims);
    pfree(lbs);

    PG_RETURN_ARRAYTYPE_P(result);
}
