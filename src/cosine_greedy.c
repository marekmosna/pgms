#include <float.h>
#include <postgres.h>
#include <fmgr.h>
#include <utils/array.h>
#include <utils/float.h>

#include "cosine.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(cosine_greedy);
Datum cosine_greedy(PG_FUNCTION_ARGS)
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
    float4 tolerance = PG_GETARG_FLOAT4(2);
    float4 mz_power = PG_GETARG_FLOAT4(3);
    float4 intensity_power = PG_GETARG_FLOAT4(4);

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

    for(Index peak1 = 0; peak1 < reference_len; peak1++)
    {
        float4 low_bound = reference_mzs[peak1] - tolerance;
        float4 high_bound = reference_mzs[peak1] + tolerance;
        float4 best_match_peak = 1.0f;
        float4 best_match_mz = NAN;
        float4 lowest_delta = FLT_MAX;
        Index stack_top = 0;

        for(Index peak2 = lowest_idx; peak2 < query_len; peak2++)
        {
            if(float4_gt(query_mzs[peak2], high_bound))
            {
                elog(NOTICE, "break [%f,%f]", reference_mzs[peak1], query_mzs[peak2]);
                break;
            }

            if(float4_lt(query_mzs[peak2], low_bound))
            {
                elog(NOTICE, "skip [%f,%f]", reference_mzs[peak1], query_mzs[peak2]);
                lowest_idx = peak2;
                continue;
            }

            if(float4_lt(fabs(float4_mi(query_peaks[peak2], reference_peaks[peak1])), lowest_delta) && !query_used[peak2])
            {
                query_stack[stack_top++] = peak2;
                lowest_delta = fabs(float4_mi(query_peaks[peak2], reference_peaks[peak1]));
                elog(NOTICE, "stacked [%f,%f]", reference_mzs[peak1], query_mzs[peak2]);
            }
        }

        if(stack_top > 0)
        {
            Index forward = peak1 + 1;
            while(true)
            {
                if(float4_gt(reference_mzs[forward], query_mzs[query_stack[stack_top - 1]] + tolerance))
                {
                    elog(NOTICE, "forward break [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    query_used[query_stack[stack_top - 1]] = 1;
                    best_match_peak = query_peaks[query_stack[stack_top - 1]];
                    best_match_mz = query_mzs[query_stack[stack_top - 1]];
                    break;
                }

                if(float4_lt(fabs(float4_mi(query_peaks[query_stack[stack_top - 1]], reference_peaks[forward])), lowest_delta))
                {
                    elog(NOTICE, "forward colide [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    lowest_delta = fabs(float4_mi(query_peaks[query_stack[stack_top - 1]], reference_peaks[forward]));
                    forward = peak1 + 1;
                    best_match_mz = query_mzs[query_stack[stack_top - 1]];
                    best_match_peak = query_peaks[query_stack[stack_top - 1]];
                    stack_top--;
                    elog(NOTICE, "best match: %f", best_match_peak);
                }

                if(!stack_top)
                {
                    best_match_mz = query_mzs[query_stack[0]];
                    best_match_peak = query_peaks[query_stack[0]];
                    elog(NOTICE, "backtrace break");
                    break;
                }

                forward++;
            }

            if(stack_top)
                score += calc_score(reference_peaks[peak1], best_match_peak, reference_mzs[peak1], best_match_mz, intensity_power, mz_power);
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

PG_FUNCTION_INFO_V1(cosine_greedy_simple);
Datum cosine_greedy_simple(PG_FUNCTION_ARGS)
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
    float4 tolerance = PG_GETARG_FLOAT4(2);

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

    for(Index peak1 = 0; peak1 < reference_len; peak1++)
    {
        float4 low_bound = reference_mzs[peak1] - tolerance;
        float4 high_bound = reference_mzs[peak1] + tolerance;
        float4 best_match = 1.0f;
        float4 lowest_delta = FLT_MAX;
        Index stack_top = 0;

        for(Index peak2 = lowest_idx; peak2 < query_len; peak2++)
        {
            if(float4_gt(query_mzs[peak2], high_bound))
            {
                elog(NOTICE, "break [%f,%f]", reference_mzs[peak1], query_mzs[peak2]);
                break;
            }

            if(float4_lt(query_mzs[peak2], low_bound))
            {
                elog(NOTICE, "skip [%f,%f]", reference_mzs[peak1], query_mzs[peak2]);
                lowest_idx = peak2;
                continue;
            }

            if(float4_lt(fabs(float4_mi(query_peaks[peak2], reference_peaks[peak1])), lowest_delta) && !query_used[peak2])
            {
                query_stack[stack_top++] = peak2;
                lowest_delta = fabs(float4_mi(query_peaks[peak2], reference_peaks[peak1]));
                elog(NOTICE, "stacked [%f,%f]", reference_mzs[peak1], query_mzs[peak2]);
            }
        }

        if(stack_top > 0)
        {
            Index forward = peak1 + 1;
            while(true)
            {
                if(float4_gt(reference_mzs[forward], query_mzs[query_stack[stack_top - 1]] + tolerance))
                {
                    elog(NOTICE, "forward break [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    query_used[query_stack[stack_top - 1]] = 1;
                    best_match = query_peaks[query_stack[stack_top - 1]];
                    break;
                }

                if(float4_lt(fabs(float4_mi(query_peaks[query_stack[stack_top - 1]], reference_peaks[forward])), lowest_delta))
                {
                    elog(NOTICE, "forward colide [%f,%f]", reference_mzs[forward], query_mzs[query_stack[stack_top - 1]]);
                    lowest_delta = fabs(float4_mi(query_peaks[query_stack[stack_top - 1]], reference_peaks[forward]));
                    forward = peak1 + 1;
                    best_match = query_peaks[query_stack[--stack_top]];
                    elog(NOTICE, "best match: %f", best_match);
                }

                if(!stack_top)
                {
                    best_match = query_peaks[query_stack[0]];
                    elog(NOTICE, "backtrace break");
                    break;
                }

                forward++;
            }

            if(stack_top)
                score += reference_peaks[peak1] * best_match;
        }
    }

    if(score != 0.0f)
    {
        float4 norm1 = calc_simple_norm(reference_peaks, reference_len);
        float4 norm2 = calc_simple_norm(query_peaks, query_len);

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
