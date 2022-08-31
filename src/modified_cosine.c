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
#include <utils/array.h>
#include "cosine.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(modified_cosine);
Datum modified_cosine(PG_FUNCTION_ARGS)
{
    int ndims = 0;
    int *dims = NULL;
    int nitems = 0;
    int reference_len = 0;
    int query_len = 0;
    float4 *restrict reference_mzs = NULL;
    float4 *restrict reference_peaks = NULL;
    float4 *restrict query_mzs = NULL;
    float4 *restrict query_peaks = NULL;
    size_t matches = 0;
    Index lowest_idx = 0;
    float4 score = 0.0f;

    ArrayType *reference = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *query = PG_GETARG_ARRAYTYPE_P(1);
    const float4 shift = PG_GETARG_FLOAT4(2);
    const float4 tolerance = PG_GETARG_FLOAT4(3);
    const float4 mz_power = PG_GETARG_FLOAT4(4);
    const float4 intensity_power = PG_GETARG_FLOAT4(5);

    ndims = ARR_NDIM(reference);
    dims = ARR_DIMS(reference);
    nitems = ArrayGetNItems(ndims, dims);
    reference_len = nitems / ndims;
    reference_mzs = (float4 *) ARR_DATA_PTR(reference);
    reference_peaks = reference_mzs + reference_len;

    ndims = ARR_NDIM(query);
    dims = ARR_DIMS(query);
    nitems = ArrayGetNItems(ndims, dims);
    query_len = nitems / ndims;
    query_mzs = (float4 *) ARR_DATA_PTR(query);
    query_peaks = query_mzs + query_len;

    for(Index reference_index = 0; reference_index < reference_len; reference_index++)
    {
        float4 low_bound = reference_mzs[reference_index] - tolerance;
        float4 high_bound = reference_mzs[reference_index] + tolerance;

        for(Index query_index = lowest_idx; query_index < query_len; query_index++)
        {
            if(Max(query_mzs[query_index], query_mzs[query_index] + shift) > high_bound)
                break;

            lowest_idx = query_index + 1;

            if(Min(query_mzs[query_index], query_mzs[query_index] + shift) < low_bound)
                continue;

            matches++;
            score += calc_score(reference_peaks[reference_index], query_peaks[query_index], reference_mzs[reference_index], query_mzs[query_index], intensity_power, mz_power);
            break;
        }
    }

    if(score != 0.0f)
    {
        float4 norm1 = calc_norm(reference_peaks, reference_mzs, reference_len, intensity_power, mz_power);
        float4 norm2 = calc_norm(query_peaks, query_mzs, query_len, intensity_power, mz_power);

        score /= sqrtf(norm1 * norm2);
    }

    PG_FREE_IF_COPY(reference, 0);
    PG_FREE_IF_COPY(query, 1);

    if(isfinite(score) && score < 0.0f)
        score = 0.0f;
    else if(isfinite(score) && score > 1.0f)
        score = 1.0f;
    else if(!isfinite(score))
        score = NAN;

    PG_RETURN_FLOAT4(score);
}
