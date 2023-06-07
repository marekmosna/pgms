/* 
 * This file is part of the PGMS distribution (https://github.com/genesissoftware-tech/pgms or https://ip-147-251-124-124.flt.cloud.muni.cz/chemdb/pgms).
 * Copyright (c) 2022 Marek Mosna (marek.mosna@genesissoftware.eu) for functions spectrum_max_intensity and spectrum_normalize
 * Copyright (c) 2022 Jakub Galgonek (jakub.galgonek@uochb.cas.cz) for functions spectrum_input and spectrum_output
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
#include "spectrum.h"

#include <catalog/pg_type.h>
#include <utils/lsyscache.h>
#include <utils/array.h>

Oid spectrumOid;

PG_FUNCTION_INFO_V1(spectrum_input);
Datum spectrum_input(PG_FUNCTION_ARGS)
{
    char *data = PG_GETARG_CSTRING(0);
    Oid	infuncid;
    Oid ioparams;
    FmgrInfo infuncinfo;

    elog(DEBUG1, "spectrum_input: %s", data);
    getTypeInputInfo(FLOAT4ARRAYOID, &infuncid, &ioparams);
    fmgr_info(infuncid, &infuncinfo);
    elog(DEBUG1, "in_fnc(%d), ioparams(%d)",infuncid, ioparams);
    PG_RETURN_DATUM(InputFunctionCall(&infuncinfo, data, ioparams, -1));
}


PG_FUNCTION_INFO_V1(spectrum_output);
Datum spectrum_output(PG_FUNCTION_ARGS)
{
    Oid	outfuncid;
    bool ioparams;
    FmgrInfo outfuncinfo;

    elog(DEBUG1, "spectrum_output");
    getTypeOutputInfo(FLOAT4ARRAYOID, &outfuncid, &ioparams);
    fmgr_info(outfuncid, &outfuncinfo);
    elog(DEBUG1, "out_fnc(%d), ioparams(%d)",outfuncid, ioparams);
    PG_RETURN_CSTRING(OutputFunctionCall(&outfuncinfo, PG_GETARG_DATUM(0)));
}

size_t spectrum_length(Datum spectrum)
{
    ArrayType *s = DatumGetArrayTypeP(spectrum);
    int ndims = ARR_NDIM(s);
    int *dims = ARR_DIMS(s);
    int nitems = ArrayGetNItems(ndims, dims);
    return nitems / ndims;
}

extern float4* spectrum_data(Datum spectrum)
{
    ArrayType *s = DatumGetArrayTypeP(spectrum);
    return (float4 *) ARR_DATA_PTR(s);
}
