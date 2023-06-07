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
#include <utils/float.h>

#include "cosine.h"
#include "spectrum.h"

PG_FUNCTION_INFO_V1(cosine_neutral_losses);
Datum cosine_neutral_losses(PG_FUNCTION_ARGS)
{
    size_t reference_len = 0;
    size_t query_len = 0;
    float4 *restrict reference_mzs = NULL;
    float4 *restrict reference_peaks = NULL;
    float4 *restrict query_mzs = NULL;
    float4 *restrict query_peaks = NULL;
    Index *restrict query_used = NULL;
    Index *restrict query_stack = NULL;
    Index lowest_idx = 0;
    float4 score = 0.0f;

    Datum reference = PG_GETARG_DATUM(0);
    Datum query = PG_GETARG_DATUM(1);
    const float4 reference_precursor_mz = PG_GETARG_FLOAT4(2);
    const float4 query_precursor_mz = PG_GETARG_FLOAT4(3);
    const float4 tolerance = PG_GETARG_FLOAT4(4);
    const float4 mz_power = PG_GETARG_FLOAT4(5);
    const float4 intensity_power = PG_GETARG_FLOAT4(6);
    const float4 shift = reference_precursor_mz - query_precursor_mz;
    calc_score_func_t calc_score = determine_calc_score(mz_power, intensity_power);
    calc_norm_func_t calc_norm = determine_calc_norm(mz_power, intensity_power);

    reference_len = spectrum_length(reference);
    reference_mzs = spectrum_data(reference);
    reference_peaks = reference_mzs + reference_len;

    query_len = spectrum_length(query);
    query_mzs = spectrum_data(query);
    query_peaks = query_mzs + query_len;

    query_used = (Index*) palloc0(query_len * sizeof(Index));
    query_stack = (Index*) palloc(query_len * sizeof(Index));

    for(Index reference_index = 0; reference_index < reference_len; reference_index++)
    {
        float4 low_bound = reference_mzs[reference_index] - tolerance;
        float4 high_bound = reference_mzs[reference_index] + tolerance;
        float4 best_match_peak = 1.0f;
        float4 highest_intensity = -get_float4_infinity();
        Index stack_top = 0;

        if(float4_gt(reference_mzs[reference_index], reference_precursor_mz))
        {
            elog(NOTICE, "loss reference [%f]", reference_mzs[reference_index]);
            continue;
        }

        if(float4_gt(query_mzs[lowest_idx], query_precursor_mz))
        {
            elog(NOTICE, "loss query [%f]", query_mzs[lowest_idx]);
            lowest_idx++;
            continue;
        }

        for(Index query_index = lowest_idx; query_index < query_len; query_index++)
        {
            if(float4_gt(float4_pl(query_mzs[query_index], shift), high_bound))
            {
                elog(DEBUG1, "break-shift [%f,%f] on %f"
                    , reference_mzs[reference_index]
                    , query_mzs[query_index]
                    , high_bound);
                break;
            }

            if(float4_lt(float4_pl(query_mzs[query_index], shift), low_bound))
            {
                elog(DEBUG1, "skip-shift [%f,%f] on %f"
                    , reference_mzs[reference_index]
                    , query_mzs[query_index]
                    , low_bound);

                lowest_idx = query_index;
                continue;
            }

            if(float4_gt(query_peaks[query_index], highest_intensity) && !query_used[query_index])
            {
                query_stack[stack_top++] = query_index;
                highest_intensity = query_peaks[query_index];
                elog(NOTICE, "stacked-shift [%f,%f]", reference_mzs[reference_index], query_mzs[query_index]);
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
                    elog(NOTICE, "forward break [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    query_used[query_stack[stack_top - 1]] = 1;
                    best_match_peak = query_peaks[query_stack[stack_top - 1]];
                    best_match_mz = query_mzs[query_stack[stack_top - 1]];
                    break;
                }

                if(float4_gt(reference_peaks[forward], highest_intensity))
                {
                    elog(NOTICE, "forward colide [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    forward = reference_index + 1;
                    highest_intensity = query_peaks[query_stack[stack_top - 1]];
                    best_match_mz = query_mzs[query_stack[stack_top - 1]];
                    best_match_peak = query_peaks[query_stack[stack_top - 1]];
                    stack_top--;
                    elog(NOTICE, "best match: %f", best_match_peak);
                }

                if(!stack_top)
                {
                    elog(NOTICE, "backtrace break");
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

                elog(NOTICE, "calc:[%f, %f] - [%f, %f]", reference_mzs[reference_index]
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
    PG_FREE_IF_COPY(PG_DETOAST_DATUM(reference), 0);
    PG_FREE_IF_COPY(PG_DETOAST_DATUM(query), 1);

    if(float4_lt(score, 0.0f))
        score = 0.0f;
    else if(float4_gt(score, 1.0f))
        score = 1.0f;

    PG_RETURN_FLOAT4(score);
}
