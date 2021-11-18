#include <postgres.h>
#include <fmgr.h>
#include <math.h>
#include <float.h>
#include "cosine.h"
#include "utils.h"
#include "lsap.h"


PG_FUNCTION_INFO_V1(cosine_hungarian);
Datum cosine_hungarian(PG_FUNCTION_ARGS)
{
    void *spec1 = PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    void *spec2 = PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

    int len1 = (VARSIZE(spec1) - VARHDRSZ) / sizeof(float4) / 2;
    float *restrict mz1 = (float4 *) VARDATA(spec1);
    float *restrict intensities1 = mz1 + len1;

    int len2 = (VARSIZE(spec2) - VARHDRSZ) / sizeof(float4) / 2;
    float *restrict mz2 = (float4 *) VARDATA(spec2);
    float *restrict intensities2 = mz2 + len2;

    const float tolerance = PG_GETARG_FLOAT4(2);
    const float mz_power = PG_GETARG_FLOAT4(3);
    const float intensity_power = PG_GETARG_FLOAT4(4);


    int *restrict used1 = palloc(len1 * sizeof(int));
    int *restrict used2 = palloc(len2 * sizeof(int));
    int *restrict paired1 = palloc(len1 * len2 * sizeof(int));
    int *restrict paired2 = palloc(len1 * len2 * sizeof(int));

    memset(used1, 0, len1 * sizeof(int));
    memset(used2, 0, len2 * sizeof(int));

    int pairs = 0;
    int lowest_idx = 0;

    for(int peak1 = 0; peak1 < len1; peak1++)
    {
        float low_bound = mz1[peak1] - tolerance;
        float high_bound = mz1[peak1] + tolerance;

        for(int peak2 = lowest_idx; peak2 < len2; peak2++)
        {
            if(mz2[peak2] > high_bound)
                break;

            if(mz2[peak2] < low_bound)
            {
                lowest_idx = peak2 + 1;
                continue;
            }

            used1[peak1]++;
            used2[peak2]++;

            paired1[pairs] = peak1;
            paired2[pairs] = peak2;
            pairs++;
        }
    }


    float score = 0;
    int matches = 0;

    if(pairs > 0)
    {
        int *restrict map1 = palloc(len1 * sizeof(int));
        int *restrict map2 = palloc(len2 * sizeof(int));

        memset(map1, -1, len1 * sizeof(int));
        memset(map2, -1, len2 * sizeof(int));

        int selected1 = 0;
        int selected2 = 0;

        for(int i = 0; i < pairs; i++)
        {
            if(used1[paired1[i]] != 1 || used2[paired2[i]] != 1)
            {
               if(map1[paired1[i]] == -1)
                   map1[paired1[i]] = selected1++;

               if(map2[paired2[i]] == -1)
                   map2[paired2[i]] = selected2++;
            }
        }

        if(selected1 > selected2)
        {
            swap(selected1, selected2);
            swap(paired1, paired2);
            swap(map1, map2);

            swap(intensities1, intensities2);
            swap(mz1, mz2);
            swap(len1, len2);
        }

        float *restrict cost = palloc(selected1 * selected2 * sizeof(float));
        memset(cost, 0, selected1 * selected2 * sizeof(float));

        float max = 0;

        for(int i = 0; i < pairs; i++)
        {
            float s = calc_score(intensities1[paired1[i]], intensities2[paired2[i]], mz1[paired1[i]], mz2[paired2[i]],
                    intensity_power, mz_power);

            if(map1[paired1[i]] != -1 && map2[paired2[i]] != -1)
            {
                if(s == 0)
                    s = FLT_MIN;

                if(s > max)
                    max = s;

                cost[map1[paired1[i]] * selected2 + map2[paired2[i]]] = s;
            }
            else
            {
                score += s;
                matches++;
            }
        }

        solve_rectangular_linear_sum_assignment(selected1, selected2, cost, max, &matches, &score);

        if(score != 0)
        {
            float norm1 = calc_norm(intensities1, mz1, len1, intensity_power, mz_power);
            float norm2 = calc_norm(intensities2, mz2, len2, intensity_power, mz_power);

            score /= sqrtf(norm1 * norm2);
        }

        pfree(cost);
        pfree(map2);
        pfree(map1);
    }

    pfree(paired2);
    pfree(paired1);
    pfree(used2);
    pfree(used1);

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
