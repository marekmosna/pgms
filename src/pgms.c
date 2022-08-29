/* 
 * This file is part of the PGMS distribution (https://github.com/genesissoftware-tech/pgms or https://ip-147-251-124-124.flt.cloud.muni.cz/chemdb/pgms).
 * Copyright (c) 2022 Jakub Galgonek (jakub.galgonek@uochb.cas.cz) for function _PG_init
 * Copyright (c) 2022 Marek Mosna (marek.mosna@genesissoftware.eu) for rest of functions
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

#include "pgms.h"
#include "mgf.h"

#include <catalog/namespace.h>
#include <utils/syscache.h>
#include <libpq/libpq-fs.h>
#include <catalog/pg_type.h>
#include <storage/large_object.h>
#include <funcapi.h>

PG_MODULE_MAGIC;

Oid spectrumOid;

void _PG_init()
{
    Oid spaceid = LookupExplicitNamespace("pgms", false);
#if PG_VERSION_NUM >= 120000
    spectrumOid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
#else
    spectrumOid = GetSysCacheOid2(TYPENAMENSP, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
#endif
}

PG_FUNCTION_INFO_V1(load_mgf_lo);
Datum load_mgf_lo(PG_FUNCTION_ARGS)
{
    bool next;
    FuncCallContext *funcctx = NULL;
    struct Parser* parser = NULL;

    if(SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext = NULL;
        TupleDesc tuple_desc = NULL;

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        if(get_call_result_type(fcinfo, NULL, &tuple_desc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                , errmsg("unsupported return type")));

        funcctx->attinmeta = TupleDescGetAttInMetadata(tuple_desc);
        parser = parser_init_lob(
            inv_open(PG_GETARG_OID(0), INV_READ, funcctx->multi_call_memory_ctx)
            , funcctx->attinmeta
        );
        parser_globals(parser);
        funcctx->user_fctx = (void*)parser;
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    parser = (struct Parser*) funcctx->user_fctx;

    PG_TRY();
    {
        next = parser_next(parser);
    }
    PG_CATCH();
    {
        parser_close(parser);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if(next)
    {
        SRF_RETURN_NEXT(funcctx, parser_get_tuple(parser));
    }

    parser_close(parser);
    SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(load_mgf_varchar);
Datum load_mgf_varchar(PG_FUNCTION_ARGS)
{
    bool next;
    FuncCallContext *funcctx = NULL;
    struct Parser* parser = NULL;

    if(SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext = NULL;
        TupleDesc tuple_desc = NULL;

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        if(get_call_result_type(fcinfo, NULL, &tuple_desc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                , errmsg("unsupported return type")));

        funcctx->attinmeta = TupleDescGetAttInMetadata(tuple_desc);
        parser = parser_init_varchar(PG_GETARG_VARCHAR_P(0), funcctx->attinmeta);
        parser_globals(parser);
        funcctx->user_fctx = (void*)parser;
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    parser = (struct Parser*) funcctx->user_fctx;

    PG_TRY();
    {
        next = parser_next(parser);
    }
    PG_CATCH();
    {
        parser_close(parser);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if(next)
    {
        SRF_RETURN_NEXT(funcctx, parser_get_tuple(parser));
    }

    parser_close(parser);
    SRF_RETURN_DONE(funcctx);
}
