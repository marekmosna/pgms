/*
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


This code implements the shortest augmenting path algorithm for the
rectangular assignment problem.  This implementation is based on the
pseudocode described in pages 1685-1686 of:

    DF Crouse. On implementing 2D rectangular assignment algorithms.
    IEEE Transactions on Aerospace and Electronic Systems
    52(4):1679-1696, August 2016
    doi: 10.1109/TAES.2016.140952

Author: PM Larsen & Jakub Galgonek
*/

#include <postgres.h>
#include <fmgr.h>
#include <math.h>
#include <float.h>
#include <utils/array.h>

#include "cosine.h"
#include "utils.h"

PG_MODULE_MAGIC;

static inline int augmenting_path(int nr, int nc, const float *restrict cost, float offset, const float *restrict u,
        const float *restrict v, int *restrict path, int *restrict row4col, float *restrict shortest_paths, int i,
        bool *restrict sr, bool *restrict sc, int *restrict remaining, float *restrict pmin)
{
    float min = 0;
    int num_remaining = nc;

    // Filling this up in reverse order ensures that the solution of a
    // constant cost matrix is the identity matrix (c.f. #11602).
    for(int it = 0; it < nc; it++)
        remaining[it] = nc - it - 1;

    for(int i = 0; i < nr; i++)
        sr[i] = false;

    for(int i = 0; i < nc; i++)
        sc[i] = false;

    for(int i = 0; i < nc; i++)
        shortest_paths[i] = INFINITY;


    // find shortest augmenting path
    int sink = -1;
    while(sink == -1)
    {
        int index = -1;
        float lowest = INFINITY;
        sr[i] = true;

        for(int it = 0; it < num_remaining; it++)
        {
            int j = remaining[it];

            // true cost value is offset - cost[i * nc + j]
            float r = min + offset - cost[i * nc + j] - u[i] - v[j];

            if(r < shortest_paths[j])
            {
                path[j] = i;
                shortest_paths[j] = r;
            }

            // When multiple nodes have the minimum cost, we select one which
            // gives us a new sink node. This is particularly important for
            // integer cost matrices with small co-efficients.
            if(shortest_paths[j] < lowest || (shortest_paths[j] == lowest && row4col[j] == -1))
            {
                lowest = shortest_paths[j];
                index = it;
            }
        }

        min = lowest;
        int j = remaining[index];

        if(min == INFINITY)
            return -1; // infeasible cost matrix

        if(row4col[j] == -1)
            sink = j;
        else
            i = row4col[j];

        sc[j] = true;
        remaining[index] = remaining[--num_remaining];
    }

    *pmin = min;
    return sink;
}


void solve_rectangular_linear_sum_assignment(int nr, int nc, const float *restrict cost, float offset,
        int *restrict matched, float *restrict score)
{
    if(offset == INFINITY)
    {
        *score = NAN;
        *matched = -1;
        return;
    }

    // handle trivial inputs
    if(nr == 0 || nc == 0)
        return;

    // initialize variables
    float *restrict u = palloc(nr * sizeof(float));
    float *restrict v = palloc(nc * sizeof(float));
    float *restrict shortest_paths = palloc(nc * sizeof(float));
    int *restrict path = palloc(nc * sizeof(int));
    int *restrict col4row = palloc(nr * sizeof(int));
    int *restrict row4col = palloc(nc * sizeof(int));
    bool *restrict sr = palloc(nr * sizeof(bool));
    bool *restrict sc = palloc(nc * sizeof(bool));
    int *restrict remaining = palloc(nc * sizeof(int));
    bool infeasible = false;

    memset(u, 0, nr * sizeof(float));
    memset(v, 0, nc * sizeof(float));
    memset(path, -1, nc * sizeof(int));
    memset(col4row, -1, nr * sizeof(int));
    memset(row4col, -1, nc * sizeof(int));

    // iteratively build the solution
    for(int row = 0; row < nr; row++)
    {
        float min;
        int sink = augmenting_path(nr, nc, cost, offset, u, v, path, row4col, shortest_paths, row, sr, sc, remaining, &min);

        if(sink < 0)
        {
            infeasible = true;
            break;
        }

        // update dual variables
        u[row] += min;

        for(int i = 0; i < nr; i++)
            if(sr[i] && i != row)
                u[i] += min - shortest_paths[col4row[i]];

        for(int j = 0; j < nc; j++)
            if(sc[j])
                v[j] -= min - shortest_paths[j];

        // augment previous solution
        while(true)
        {
            int i = path[sink];
            row4col[sink] = i;

            swap(col4row[i], sink);

            if(i == row)
                break;
        }
    }

    if(!infeasible)
    {
        int cnt = 0;
        float sum = 0;

        for(int i = 0; i < nr; i++)
        {
            if(cost[i * nc + col4row[i]] > 0)
            {
                sum += cost[i * nc + col4row[i]];
                cnt++;
            }
        }

        *score += sum;
        *matched += cnt;
    }
    else
    {
        *score = NAN;
        *matched = -1;
    }

    pfree(remaining);
    pfree(sc);
    pfree(sr);
    pfree(row4col);
    pfree(col4row);
    pfree(path);
    pfree(shortest_paths);
    pfree(v);
    pfree(u);
}

PG_FUNCTION_INFO_V1(cosine_hungarian);
Datum cosine_hungarian(PG_FUNCTION_ARGS)
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
