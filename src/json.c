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

#include "pgms.h"
#include "spectrum.h"

#include <funcapi.h>
#include <plpgsql.h>
#include <utils/builtins.h>
#include <catalog/pg_type.h>
#include <catalog/namespace.h>
#if PG_VERSION_NUM >= 130000
    #include <utils/jsonb.h>
#else
    #include <utils/jsonapi.h>
#endif
#include <nodes/pg_list.h>
#include <access/htup_details.h>

#include <utils/syscache.h>

typedef struct
{
    JsonbIterator   *it;
    Datum*          values;
    bool*           isnull;
    JsonbIteratorToken state;
    size_t          cnt;
} json_ctx_t;

void json_ctx_next(AttInMetadata *attinmeta, json_ctx_t *json_ctx);

static void json_ctx_free(json_ctx_t* ctx)
{
    if(!ctx)
        return;
    if(ctx->values)
        pfree(ctx->values);
    if(ctx->isnull)
        pfree(ctx->isnull);
    pfree(ctx);
}

static json_ctx_t* json_ctx_create(Jsonb* jb, TupleDesc td)
{
    json_ctx_t* json_ctx = (json_ctx_t*) palloc0(sizeof(json_ctx_t));

    json_ctx->it = JsonbIteratorInit(&jb->root);
    json_ctx->values = palloc(td->natts * sizeof(Datum));
    json_ctx->isnull = palloc(td->natts * sizeof(bool));

    if(JB_ROOT_IS_OBJECT(jb))
        json_ctx->cnt = 1;
    else if(JB_ROOT_IS_ARRAY(jb))
        json_ctx->cnt = JB_ROOT_COUNT(jb);
    else
        json_ctx->cnt = 0;

    return json_ctx;
}

static dummyret parser_set_column_array(json_ctx_t* ctx, AttInMetadata *attinmeta, Index columnIndex, List *l, size_t dim)
{
#define ArrayInnerTraversal(list, n, res)                   \
    do {                                                    \
        for(Index i = 0; i < (n); i++)                      \
        {                                                   \
            StringInfo record = (StringInfo) lfirst(        \
                list_nth_cell((list), i)                    \
            );                                              \
            Pointer pbegin = StringInfoToCString(record);   \
            Pointer pend = pbegin + record->len -1;         \
                                                            \
            NormalizeNumericSignSuffix(pbegin, pend);       \
            appendStringInfoString((result), pbegin);       \
            appendStringInfoCharMacro((result)              \
                , i == (n) - 1 ? ' ' : ',');                \
            pfree(record->data);                            \
        }                                                   \
    } while(false)

    StringInfo result = makeStringInfo();

    if(columnIndex < ColumnCount(attinmeta->tupdesc) && !ColumnIsDropped(attinmeta->tupdesc, columnIndex))
    {
        ListCell *cell = NULL;
        size_t count = list_length(l);

        elog(DEBUG1, "%s of %d: sizeof %ld in %ld dimension"
            , NameStr(*ColumnName(attinmeta->tupdesc, columnIndex))
            , ColumnType(attinmeta->tupdesc, columnIndex)
            , count
            , dim);

        appendStringInfoCharMacro(result, '{');

        foreach(cell, l)
        {
            List* ll = lfirst(cell);

            appendStringInfoCharMacro(result, '{');
            elog(DEBUG1, "count %d", list_length(ll));
            ArrayInnerTraversal(ll, list_length(ll), result);
            appendStringInfoCharMacro(result, '}');
            appendStringInfoCharMacro(result, cell == list_tail(l) ? ' ' : ',');
        }

        appendStringInfoCharMacro(result, '}');

        elog(DEBUG1, "Formed to: %s", StringInfoToCString(result));
        ctx->values[columnIndex] = InputFunctionCall(&attinmeta->attinfuncs[columnIndex]
            , StringInfoToCString(result)
            , attinmeta->attioparams[columnIndex]
            , attinmeta->atttypmods[columnIndex]);
        ctx->isnull[columnIndex] = false;
        pfree(result->data);
        pfree(result);
    }
#undef ArrayInnerTraversal
}

void json_ctx_next(AttInMetadata *attinmeta, json_ctx_t *json_ctx)
{
    StringInfo data = makeStringInfo();
    TupleDesc tupdesc = attinmeta->tupdesc;
    Index idx = tupdesc->natts;
    size_t elem_cnt = 0;
    List *mzs = NIL;
    List *peaks = NIL;
    int ions_cnt = 0;

    do
    {
        JsonbValue val = { 0 };
        JsonbIteratorToken state = JsonbIteratorNext(&json_ctx->it, &val, false);
        elog(DEBUG1, "json state %d", state);
        switch (state)
        {
            case WJB_KEY:
                idx = 0;
                appendBinaryStringInfo(data, val.val.string.val, val.val.string.len);
                while(idx < tupdesc->natts
                    && !TupleDescAttr(tupdesc, idx)->attisdropped
                    && strcmp(data->data, NameStr(TupleDescAttr(tupdesc, idx)->attname))
                    )
                    idx++;
                break;
            case WJB_ELEM:
                if(tupdesc->attrs[idx].atttypid == spectrumOid)
                {
                    StringInfo record = makeStringInfo();
                    char *tmp = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(val.val.numeric)));

                    elog(DEBUG1, "WJB_ELEM %s", tmp);

                    appendStringInfoString(record, tmp);
                    if(elem_cnt++ % 2 == 0)
                    {
                        mzs = lappend(mzs, record);
                    }
                    else
                    {
                        peaks = lappend(peaks, record);
                        ions_cnt--;
                    }
                }
                break;
            case WJB_VALUE:
                if(idx != tupdesc->natts)
                {
                    if(val.type == jbvNumeric)
                        appendStringInfoString(data, DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(val.val.numeric))));
                    else
                        appendBinaryStringInfo(data, val.val.string.val, val.val.string.len);

                    json_ctx->values[idx] = InputFunctionCall(&attinmeta->attinfuncs[idx], data->data, attinmeta->attioparams[idx], attinmeta->atttypmods[idx]);
                    json_ctx->isnull[idx] = false;
                    elog(DEBUG1, "WJB_VALUE %s", data->data);
                }
                idx = tupdesc->natts;
                break;
            case WJB_BEGIN_ARRAY:
                if( idx != tupdesc->natts 
                    && tupdesc->attrs[idx].atttypid == spectrumOid
                    && ions_cnt == 0)
                {
                    ions_cnt = json_ctx->it->nElems;
                    elog(DEBUG1, "WJB_BEGIN_ARRAY of %d", json_ctx->it->nElems);
                }
                if(idx == tupdesc->natts)
                    json_ctx->state = WJB_BEGIN_OBJECT;
                break;
            case WJB_END_ARRAY:
                if( idx != tupdesc->natts
                    && tupdesc->attrs[idx].atttypid == spectrumOid
                    && ions_cnt == 0)
                {
                    elog(DEBUG1, "WJB_END_ARRAY");

                    if(mzs && peaks)
                    {
                        List *spectrums = NIL;

                        spectrums = lappend(spectrums, mzs);
                        spectrums = lappend(spectrums, peaks);
                        parser_set_column_array(json_ctx, attinmeta, idx, spectrums, SPECTRUM_ARRAY_DIM);
                        list_free_deep(mzs);
                        list_free_deep(peaks);
                        list_free(spectrums);
                    }
                    idx = tupdesc->natts;
                }
                break;
            case WJB_BEGIN_OBJECT:
                json_ctx->state = json_ctx->state == WJB_DONE ? WJB_BEGIN_OBJECT : WJB_END_OBJECT;
                break;
            case WJB_END_OBJECT:
                json_ctx->state = json_ctx->state == WJB_END_OBJECT ? WJB_BEGIN_OBJECT : WJB_DONE; 
                break;
            case WJB_DONE:
            default:
                break;
        }
        resetStringInfo(data);
    } while (json_ctx->state != WJB_DONE && json_ctx->cnt);

    pfree(data->data);
    pfree(data);
}

PG_FUNCTION_INFO_V1(load_from_json);
Datum load_from_json(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx = NULL;
    json_ctx_t *json_ctx = NULL;
    TupleDesc tuple_desc = NULL;

    if(SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext = NULL;

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        if(get_call_result_type(fcinfo, NULL, &tuple_desc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                , errmsg("unsupported return type")));

        if(!OidIsValid(spectrumOid))
            spectrumOid = TypenameGetTypid("spectrum");

        funcctx->attinmeta = TupleDescGetAttInMetadata(tuple_desc);
        funcctx->user_fctx = (void*) json_ctx_create(PG_GETARG_JSONB_P(0), tuple_desc);
        elog(DEBUG1, "records: %ld", ((json_ctx_t*)funcctx->user_fctx)->cnt);
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    json_ctx = (json_ctx_t*) funcctx->user_fctx;
    tuple_desc = funcctx->attinmeta->tupdesc;

    PG_TRY();
    {
        for(Index i = 0; i < tuple_desc->natts; i++)
        {
            json_ctx->values[i] = (Datum) NULL;
            json_ctx->isnull[i] = true;
        }
        json_ctx_next(funcctx->attinmeta, json_ctx);
    }
    PG_CATCH();
    {
        json_ctx_free(json_ctx);
        PG_RE_THROW();
    }
    PG_END_TRY();

    if(json_ctx->cnt--)
    {
        HeapTuple tuple = heap_form_tuple(funcctx->attinmeta->tupdesc, json_ctx->values, json_ctx->isnull);
        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
    }

    json_ctx_free(json_ctx);
    SRF_RETURN_DONE(funcctx);
}
