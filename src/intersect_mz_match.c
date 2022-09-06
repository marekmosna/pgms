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
#include <float.h>
#include <utils/array.h>
#include <math.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(intersect_mz);
Datum intersect_mz(PG_FUNCTION_ARGS)
{
    int ndims;
    int *dims;
    int nitems;
    size_t reference_len;
    size_t query_len;
    float4 *restrict reference_mzs;
    float4 *restrict query_mzs;
    size_t count_intersect = 0;
    size_t count_union = 0;
    Index peak1 = 0;
    Index peak2 = 0;
    const float tolerance = PG_GETARG_FLOAT4(2);
    ArrayType *reference = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *query = PG_GETARG_ARRAYTYPE_P(1);

    ndims = ARR_NDIM(reference);
    dims = ARR_DIMS(reference);
    nitems = ArrayGetNItems(ndims, dims);
    reference_len = nitems / ndims;
    reference_mzs = (float4 *) ARR_DATA_PTR(reference);

    ndims = ARR_NDIM(query);
    dims = ARR_DIMS(query);
    nitems = ArrayGetNItems(ndims, dims);
    query_len = nitems / ndims;
    query_mzs = (float4 *) ARR_DATA_PTR(query);

    elog(DEBUG1, "reference of %ld against query of %ld",
        reference_len, query_len);

    while (peak1 < reference_len && peak2 < query_len)
    {
        float4 low_bound = reference_mzs[peak1] - tolerance;
        float4 high_bound = reference_mzs[peak1] + tolerance;

        if (query_mzs[peak2] < low_bound)
        {
            peak1++;
            count_union++;
            elog(DEBUG1, "too low: ref %f against query %f",low_bound, query_mzs[peak2]);
        }
        else if (query_mzs[peak2] > high_bound)
        {
            peak2++;
            count_union++;
            elog(DEBUG1, "too high: shift refquery to %d",peak2);
        }
        else
        {
            elog(DEBUG1, "intersection on ref %d against query on %d",
                peak1, peak2);
            peak1++;
            peak2++;
            count_union++;
            count_intersect++;
        }
    }

    while (peak1 < reference_len)
    {
        count_union++;
        peak1++;
    }

    while (peak2 < query_len)
    {
        count_union++;
        peak2++;
    }

    PG_FREE_IF_COPY(reference, 0);
    PG_FREE_IF_COPY(query, 1);

    PG_RETURN_FLOAT4(count_intersect == 0 ? 0.0f : (float4)count_intersect/(float4)count_union);
}
