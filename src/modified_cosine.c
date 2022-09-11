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

#include <float.h>
#include <postgres.h>
#include <fmgr.h>
#include <utils/array.h>
#include <utils/float.h>
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
    Index *restrict query_used = NULL;
    Index *restrict query_stack = NULL;
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

    query_used = palloc0(query_len * sizeof(Index));
    query_stack = palloc(query_len * sizeof(Index));

    for(Index reference_index = 0; reference_index < reference_len; reference_index++)
    {
        float4 low_bound = reference_mzs[reference_index] - tolerance;
        float4 high_bound = reference_mzs[reference_index] + tolerance;
        float4 best_match_peak = 1.0f;
        float4 highest_intensity = FLT_MIN;
        Index stack_top = 0;

        for(Index peak2 = lowest_idx; peak2 < query_len; peak2++)
        {
            if(float4_gt(query_mzs[peak2], high_bound))
            {
                elog(DEBUG1, "break0 [%f,%f]", reference_mzs[reference_index], query_mzs[peak2]);
                break;
            }

            if(float4_lt(query_mzs[peak2], low_bound))
            {
                elog(DEBUG1, "skip0 [%f,%f]", reference_mzs[reference_index], query_mzs[peak2]);
                continue;
            }

            if(float4_gt(query_peaks[peak2], highest_intensity) && !query_used[peak2])
            {
                query_stack[stack_top++] = peak2;
                highest_intensity = query_peaks[peak2];
                elog(DEBUG1, "stacked0 [%f,%f]", reference_mzs[reference_index], query_mzs[peak2]);
            }
        }

        low_bound = low_bound + shift;
        high_bound = high_bound + shift;

        for(Index query_index = lowest_idx; query_index < query_len; query_index++)
        {
            if(float4_gt(query_mzs[query_index], high_bound))
            {
                elog(DEBUG1, "break-shift [%f,%f]", reference_mzs[reference_index], query_mzs[query_index]);
                break;
            }

            if(float4_lt(query_mzs[query_index], low_bound))
            {
                elog(DEBUG1, "skip-shift [%f,%f]", reference_mzs[reference_index], query_mzs[query_index]);
                lowest_idx = query_index;
                continue;
            }

            if(float4_gt(query_peaks[query_index], highest_intensity) && !query_used[query_index])
            {
                query_stack[stack_top++] = query_index;
                highest_intensity = query_peaks[query_index];
                elog(DEBUG1, "stacked-shift [%f,%f]", reference_mzs[reference_index], query_mzs[query_index]);
            }
        }

        if(stack_top > 0)
        {
            Index forward = reference_index + 1;
            float4 best_match_mz = NAN;

            while(forward < reference_len)
            {
                if(float4_gt(reference_mzs[forward], query_mzs[query_stack[stack_top - 1]] + tolerance))
                {
                    elog(DEBUG1, "forward break [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    query_used[query_stack[stack_top - 1]] = 1;
                    best_match_peak = query_peaks[query_stack[stack_top - 1]];
                    best_match_mz = query_mzs[query_stack[stack_top - 1]];
                    break;
                }

                if(float4_gt(reference_peaks[forward], highest_intensity))
                {
                    elog(DEBUG1, "forward colide [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    forward = reference_index + 1;
                    highest_intensity = query_peaks[query_stack[stack_top - 1]];
                    best_match_mz = query_mzs[query_stack[stack_top - 1]];
                    best_match_peak = query_peaks[query_stack[stack_top - 1]];
                    stack_top--;
                    elog(DEBUG1, "best match: %f", best_match_peak);
                }

                if(!stack_top)
                {
                    elog(DEBUG1, "backtrace break");
                    break;
                }

                forward++;
            }

            if(stack_top)
            {
                if(forward >= reference_len)
                {
                    best_match_mz = query_mzs[query_stack[0]];
                    best_match_peak = query_peaks[query_stack[0]];
                }

                elog(DEBUG1, "calc:[%f, %f] - [%f, %f]", reference_mzs[reference_index]
                    , reference_peaks[reference_index]
                    , best_match_mz, best_match_peak);
                score = float4_pl(score, 
                    calc_score(reference_peaks[reference_index], best_match_peak, reference_mzs[reference_index], best_match_mz, intensity_power, mz_power));
            }
        }
    }

    if(float4_ne(score, 0.0f))
    {
        float4 norm1 = calc_norm(reference_peaks, reference_mzs, reference_len, intensity_power, mz_power);
        float4 norm2 = calc_norm(query_peaks, query_mzs, query_len, intensity_power, mz_power);

        score = float4_div(score, sqrtf(norm1 * norm2));
    }

    pfree(query_used);
    pfree(query_stack);
    PG_FREE_IF_COPY(reference, 0);
    PG_FREE_IF_COPY(query, 1);

    if(float4_lt(score, 0.0f))
        score = 0.0f;
    else if(float4_gt(score, 1.0f))
        score = 1.0f;

    PG_RETURN_FLOAT4(score);
}
