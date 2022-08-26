/* 
 * This file is part of the PGMS distribution (https://github.com/genesissoftware-tech/pgms or https://ip-147-251-124-124.flt.cloud.muni.cz/chemdb/pgms).
 * Copyright (c) 2022 Marek Mosna (info@genesissoftware.eu).
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


PG_FUNCTION_INFO_V1(intersect_mz);
Datum intersect_mz(PG_FUNCTION_ARGS)
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

    ndims = ARR_NDIM(spec2);
    dims = ARR_DIMS(spec2);
    nitems = ArrayGetNItems(ndims, dims);
    int len2 = nitems / ndims;
    float *restrict mz2 = (float4 *) ARR_DATA_PTR(spec2);

    const float tolerance = PG_GETARG_FLOAT4(2);

    size_t count_intersect = 0;
    size_t count_union = 0;

    Index peak1 = 0, peak2 = 0;
    while (peak1 < len1 && peak2 < len2)
    {
        float low_bound = mz1[peak1] - tolerance;
        float high_bound = mz1[peak1] + tolerance;

        if (low_bound < mz2[peak2])
        {
            peak1++;
            count_union++;
        }
        else if (high_bound > mz2[peak2])
        {
            peak2++;
            count_union++;
        }
        else
        {
            peak1++;
            peak2++;
            count_union++;
            count_intersect++;
        }
    }

    while (peak1 < len1)
        count_union++;

    while (peak2 < len2)
        count_union++;

    PG_FREE_IF_COPY(spec1, 0);
    PG_FREE_IF_COPY(spec2, 1);

    PG_RETURN_FLOAT4(count_intersect == 0 ? 0.0f : (float)count_intersect/(float)count_union);
}
