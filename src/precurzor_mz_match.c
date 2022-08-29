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
#include <float.h>

#define DALTON_TYPE "Dalton"
#define PPM_TYPE    "ppm"

PG_FUNCTION_INFO_V1(precurzor_mz_match);
Datum precurzor_mz_match(PG_FUNCTION_ARGS)
{
    const float query = PG_GETARG_FLOAT4(0);
    const float reference = PG_GETARG_FLOAT4(1);
    const float tolerance = PG_GETARG_FLOAT4(2);
    VarChar* tolerance_type = PG_GETARG_VARCHAR_P(3);
    bool match = false;
    float dif = fabsf(reference - query);

    elog(DEBUG1, "%f in %f by %s", dif, tolerance, VARDATA(tolerance_type));

    if(!strcmp(VARDATA(tolerance_type), DALTON_TYPE))
        match = (dif <= tolerance);
    else
    {
        float mean = fabsf(reference + query) / 2;
        match = (dif/mean*1e6 <= tolerance);
    }

    PG_RETURN_FLOAT4(match ? 1.0f : 0.0f);
}
