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

#include "spectrum.h"

PG_FUNCTION_INFO_V1(intersect_mz);
Datum intersect_mz(PG_FUNCTION_ARGS)
{
    size_t reference_len = 0;
    size_t query_len = 0;
    float4 *restrict reference_mzs;
    float4 *restrict query_mzs;
    size_t count_intersect = 0;
    size_t count_union = 0;
    Index lowest_idx = 0;

    Datum reference = PG_GETARG_DATUM(0);
    Datum query = PG_GETARG_DATUM(1);
    const float tolerance = PG_GETARG_FLOAT4(2);

    reference_len = spectrum_length(reference);
    reference_mzs = spectrum_data(reference);

    query_len = spectrum_length(query);
    query_mzs = spectrum_data(query);

    elog(DEBUG1, "reference of %ld against query of %ld",
        reference_len, query_len);

    for(Index reference_index = 0; reference_index < reference_len; reference_index++)
    {
        float4 reference_low = reference_mzs[reference_index] - tolerance;
        float4 reference_high = reference_mzs[reference_index] + tolerance;

        for(Index query_index = lowest_idx; query_index < query_len; query_index++)
        {
            if(float4_gt(query_mzs[query_index], reference_high))
            {
                elog(DEBUG1, "break [%f,%f]", reference_mzs[reference_index], query_mzs[query_index]);
                break;
            }

            if(float4_lt(query_mzs[query_index], reference_low))
            {
                elog(DEBUG1, "skip [%f,%f]", reference_mzs[reference_index], query_mzs[query_index]);
                lowest_idx = query_index + 1;
                count_union++;
            }
            else
            {
                elog(DEBUG1, "match [%f,%f] while %ld", reference_mzs[reference_index], query_mzs[query_index], count_union);
                lowest_idx = query_index + 1;
                count_intersect++;
                count_union++;
                break;
            }
        }
    }

    PG_FREE_IF_COPY(PG_DETOAST_DATUM(reference), 0);
    PG_FREE_IF_COPY(PG_DETOAST_DATUM(query), 1);

    if(count_intersect && count_union != 0)
        PG_RETURN_FLOAT4(float4_div(count_intersect, count_union));
    else
        PG_RETURN_FLOAT4(0.0f);
}
