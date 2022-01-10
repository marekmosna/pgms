#include <postgres.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include <utils/rangetypes.h>
#include "pgms.h"


typedef struct Parser
{
    //TODO: add real implementation
    int count;
}
Parser;


static void set_parameter(AttInMetadata *attinmeta, Datum *values, bool *isnull, char *name, char *value)
{
    TupleDesc tupdesc = attinmeta->tupdesc;

    int idx = 0;

    while(idx < tupdesc->natts && !TupleDescAttr(tupdesc, idx)->attisdropped && strcmp(name, NameStr(TupleDescAttr(tupdesc, idx)->attname)))
        idx++;

    if(idx == tupdesc->natts || tupdesc->attrs[idx].atttypid == spectrumOid)
        return;

    Oid typid = TupleDescAttr(tupdesc, idx)->atttypid;

    // special treatment of mgf ranges
    if(typid == INT4RANGEOID || typid == INT8RANGEOID || typid == NUMRANGEOID)
    {
        char *ptr = value;

        while(*ptr != '\0' && isspace((unsigned char) *ptr))
            ptr++;

        if(*ptr != '(' && *ptr != '[' && strncmp(ptr, "empty", 5))
        {
            char *lbound = value;
            char *rbound = value;

            ptr = strchr(ptr + 1, '-');

            if(ptr != NULL)
            {
                lbound = palloc(ptr - value + 1);
                memcpy(lbound, value, ptr - value);
                lbound[ptr - value] = '\0';
                rbound = ptr + 1;
            }

            Datum (*in)(PG_FUNCTION_ARGS) = typid == NUMRANGEOID ? numeric_in : typid == INT4RANGEOID ? int4in : int8in;
            Datum lval = DirectFunctionCall3(in, CStringGetDatum(lbound), ObjectIdGetDatum(InvalidOid), Int32GetDatum(-1));
            Datum rval = DirectFunctionCall3(in, CStringGetDatum(rbound), ObjectIdGetDatum(InvalidOid), Int32GetDatum(-1));

            RangeBound lower = { .infinite = false, .inclusive = true, .lower = true, .val = lval };
            RangeBound upper = { .infinite = false, .inclusive = true, .lower = false, .val = rval };

            TypeCacheEntry *typcache = lookup_type_cache(typid, TYPECACHE_RANGE_INFO);
            values[idx] = RangeTypePGetDatum(make_range(typcache, &lower, &upper, false));
            isnull[idx] = false;

            if(lbound != rbound)
                pfree(lbound);

            return;
        }
    }

    values[idx] = InputFunctionCall(&attinmeta->attinfuncs[idx], value, attinmeta->attioparams[idx], attinmeta->atttypmods[idx]);
    isnull[idx] = false;
}


static void set_spectrum(AttInMetadata *attinmeta, Datum *values, bool *isnull, float4* data, int count)
{
    TupleDesc tupdesc = attinmeta->tupdesc;

    size_t size = count * sizeof(float4) + VARHDRSZ;
    void *result = palloc(size);
    SET_VARSIZE(result, size);

    memcpy(VARDATA(result), data, count * sizeof(float4));

    for(int idx = 0; idx < tupdesc->natts; idx++)
    {
        if(tupdesc->attrs[idx].atttypid == spectrumOid)
        {
            values[idx] = PointerGetDatum(result);
            isnull[idx] = false;
        }
    }
}


static Parser *parser_init()
{
    //TODO: add real implementation

    Parser *parser = palloc(sizeof(Parser));
    parser->count = 0;

    return parser;
}


static void parser_close(Parser *parser)
{
    //TODO: add real implementation

    pfree(parser);
}


static bool parser_next(Parser *parser, AttInMetadata *attinmeta, Datum *values, bool *isnull)
{
    //TODO: add real implementation

    if(parser->count++ < 3)
    {
        set_parameter(attinmeta, values, isnull, "PARAM1", " 123 ");
        set_parameter(attinmeta, values, isnull, "PARAM2", " 12.3 ");
        set_parameter(attinmeta, values, isnull, "PARAM3", " 12-34 ");
        set_parameter(attinmeta, values, isnull, "PARAM4", " 12.3 - 45.6 ");
        set_parameter(attinmeta, values, isnull, "PARAM5", " [ 12.3 , 45.6 ) ");
        set_parameter(attinmeta, values, isnull, "PARAM6", "(string)");

        set_spectrum(attinmeta, values, isnull, (float4 []) {0,1,2,3}, 4);

        return true;
    }

    return false;
}


PG_FUNCTION_INFO_V1(load_from_mgf);
Datum load_from_mgf(PG_FUNCTION_ARGS)
{
    if(SRF_IS_FIRSTCALL())
    {
        FuncCallContext *funcctx = SRF_FIRSTCALL_INIT();
        MemoryContext oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        TupleDesc tuple_desc;

        if(get_call_result_type(fcinfo, NULL, &tuple_desc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("unsupported return type")));

        funcctx->attinmeta = TupleDescGetAttInMetadata(tuple_desc);
        funcctx->user_fctx = parser_init();

        MemoryContextSwitchTo(oldcontext);
    }

    FuncCallContext *funcctx = SRF_PERCALL_SETUP();

    AttInMetadata *attinmeta = funcctx->attinmeta;
    Parser *parser = (Parser *) funcctx->user_fctx;

    Datum values[attinmeta->tupdesc->natts];
    bool isnull[attinmeta->tupdesc->natts];

    for(int i = 0; i < attinmeta->tupdesc->natts; i++)
        values[i] = (Datum) 0;

    for(int i = 0; i < attinmeta->tupdesc->natts; i++)
        isnull[i] = true;

    bool next;

    PG_TRY();
    {
        next = parser_next(parser, attinmeta, values, isnull);
    }
    PG_CATCH();
    {
        parser_close(parser);

        PG_RE_THROW();
    }
    PG_END_TRY();

    if(next)
    {
        HeapTuple tuple = heap_form_tuple(attinmeta->tupdesc, values, isnull);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    parser_close(parser);
    SRF_RETURN_DONE(funcctx);
}
