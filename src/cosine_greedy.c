#include <postgres.h>
#include <fmgr.h>
#include <math.h>
#include <utils/array.h>

#include "cosine.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(cosine_greedy);
Datum cosine_greedy(PG_FUNCTION_ARGS)
{
	int ndims;
	int *dims;
	int nitems;
    ArrayType *spec1 = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *spec2 = PG_GETARG_ARRAYTYPE_P(1);

    ndims = ARR_NDIM(spec1);
    dims = ARR_DIMS(spec1);
    nitems = ArrayGetNItems(ndims, dims);
    int len1 = nitems / ndims;
    float *restrict mz1 = (float4 *) ARR_DATA_PTR(spec1);
    float *restrict intensities1 = mz1 + len1;

    ndims = ARR_NDIM(spec2);
    dims = ARR_DIMS(spec2);
    nitems = ArrayGetNItems(ndims, dims);
    int len2 = nitems / ndims;
    float *restrict mz2 = (float4 *) ARR_DATA_PTR(spec2);
    float *restrict intensities2 = mz2 + len2;

    float tolerance = PG_GETARG_FLOAT4(2);
    float mz_power = PG_GETARG_FLOAT4(3);
    float intensity_power = PG_GETARG_FLOAT4(4);


    int matches = 0;
    int lowest_idx = 0;
    float score = 0;

    for(int peak1 = 0; peak1 < len1; peak1++)
    {
        float low_bound = mz1[peak1] - tolerance;
        float high_bound = mz1[peak1] + tolerance;

        for(int peak2 = lowest_idx; peak2 < len2; peak2++)
        {
            if(mz2[peak2] > high_bound)
                break;

            lowest_idx = peak2 + 1;

            if(mz2[peak2] < low_bound)
                continue;

            matches++;
            score += calc_score(intensities1[peak1], intensities2[peak2], mz1[peak1], mz2[peak2], intensity_power, mz_power);
            break;
        }
    }

    if(score !=0)
    {
        float norm1 = calc_norm(intensities1, mz1, len1, intensity_power, mz_power);
        float norm2 = calc_norm(intensities2, mz2, len2, intensity_power, mz_power);

        score /= sqrtf(norm1 * norm2);
    }

    PG_FREE_IF_COPY(spec1, 0);
    PG_FREE_IF_COPY(spec2, 1);

    if(isfinite(score) && score < 0)
        score = 0;
    else if(isfinite(score) && score > 1)
        score = 1;
    else if(!isfinite(score))
        score = NAN;

    PG_RETURN_FLOAT4(score);
}


PG_FUNCTION_INFO_V1(cosine_greedy_simple);
Datum cosine_greedy_simple(PG_FUNCTION_ARGS)
{
	int ndims;
	int *dims;
	int nitems;
    ArrayType *spec1 = PG_GETARG_ARRAYTYPE_P(0);
    ArrayType *spec2 = PG_GETARG_ARRAYTYPE_P(1);

    ndims = ARR_NDIM(spec1);
    dims = ARR_DIMS(spec1);
    nitems = ArrayGetNItems(ndims, dims);
    int len1 = nitems / ndims;
    float *restrict mz1 = (float4 *) ARR_DATA_PTR(spec1);
    float *restrict intensities1 = mz1 + len1;

    ndims = ARR_NDIM(spec2);
    dims = ARR_DIMS(spec2);
    nitems = ArrayGetNItems(ndims, dims);
    int len2 = nitems / ndims;
    float *restrict mz2 = (float4 *) ARR_DATA_PTR(spec2);
    float *restrict intensities2 = mz2 + len2;

    float tolerance = PG_GETARG_FLOAT4(2);


    float score = 0;
    int matches = 0;
    int lowest_idx = 0;

    for(int peak1 = 0; peak1 < len1; peak1++)
    {
        float low_bound = mz1[peak1] - tolerance;
        float high_bound = mz1[peak1] + tolerance;

        for(int peak2 = lowest_idx; peak2 < len2; peak2++)
        {
            if(mz2[peak2] > high_bound)
                break;

            lowest_idx = peak2 + 1;

            if(mz2[peak2] < low_bound)
                continue;

            matches++;
            score += intensities1[peak1] * intensities2[peak2];
            break;
        }
    }

    if(score !=0)
    {
        float norm1 = calc_simple_norm(intensities1, len1);
        float norm2 = calc_simple_norm(intensities2, len2);

        score /= sqrtf(norm1 * norm2);
    }

    PG_FREE_IF_COPY(spec1, 0);
    PG_FREE_IF_COPY(spec2, 1);

    if(isfinite(score) && score < 0)
        score = 0;
    else if(isfinite(score) && score > 1)
        score = 1;
    else if(!isfinite(score))
        score = NAN;

    PG_RETURN_FLOAT4(score);
}
