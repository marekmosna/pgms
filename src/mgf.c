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

#include "mgf.h"
#include "pgms.h"

#include <utils/builtins.h>
#include <catalog/pg_type.h>
#include <catalog/namespace.h>
#include <storage/large_object.h>
#include <funcapi.h>
#include <plpgsql.h>
#include <nodes/pg_list.h>
#include <access/htup_details.h>
#include <libpq/libpq-fs.h>

#include <utils/syscache.h>

#define BEGIN_IONS_STR          "BEGIN IONS"
#define END_IONS_STR            "END IONS"
#define COMMENT_STR             "#;!/"
#define BUFFER_SIZE             (1 << 20) //1MB

PG_MODULE_MAGIC;

static Oid spectrumOid = InvalidOid;

typedef enum 
{
    BEGIN       = 0,
    BEGIN_IONS  = 1,
    END_IONS    = 2,
    END         = 3,
}
ParserState;

typedef struct Parser
{
    LargeObjectDesc     *file;
    Index               pos;
    size_t              size;
    Pointer             data;
    Datum               *values;
    bool                *isnull;
    AttInMetadata       *meta;
    ParserState         status;
} ParserData;

static bool is_comment(StringInfo str)
{
    Index idx = 0;
    size_t comments_lenght = sizeof(COMMENT_STR) - 1;
    Pointer pbegin = StringInfoToCString(str);
    Pointer pend = StringInfoToCString(str) + str->len;

    TrailLeftSpaces(pbegin, pend);
    while(idx < comments_lenght && *pbegin != COMMENT_STR[idx])
        idx++;

    return idx != comments_lenght;
}

static Index find_column_by_name(TupleDesc tuple, const char* name)
{
    Index idx = 0;

    while(idx < ColumnCount(tuple) && namestrcmp(ColumnName(tuple, idx), name))
        idx++;

    return idx;
}

static Index find_column_by_oid(TupleDesc tuple, Oid oid)
{
    Index idx = 0;

    while(idx < ColumnCount(tuple) && ColumnType(tuple, idx) != oid)
    {
        elog(DEBUG1, "cmpr %d not match %d", ColumnType(tuple, idx), oid);
        idx++;
    }

    return idx;
}

static List* split(Pointer data, const char* delimiter)
{
    List* result = NIL;

    elog(DEBUG1, "splitting %s", data);
    for(Pointer token = strtok(data, delimiter)
        ; PointerIsValid(token)
        ; token = strtok(NULL, delimiter))
    {
        StringInfo record = makeStringInfo();

        appendStringInfoString(record, token);
        result = lappend(result, record);
    }

    elog(DEBUG1, "splitted %d", list_length(result));
    return result;
}

static int read_line(struct Parser* parser, StringInfo buffer)
{
    size_t cnt = 0;
    bool white_only = true;

    do
    {
        char c = '\0';

        if(parser->pos == parser->size && PointerIsValid(parser->file))
        {
            parser->pos = 0;
            parser->size = inv_read(parser->file, parser->data, BUFFER_SIZE);
            elog(DEBUG1, "read %ld bytes", parser->size);
        }

        if(parser->pos == parser->size)
        {
            if(white_only)
                return EOF;
            else 
                break;
        }

        c = parser->data[parser->pos++];

        if(isgraph(c))
            white_only = false;
        else if (c == '\0')
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                , errmsg("Unsupported character: '\\0'")));

        if(c == '\n')
            break;

        appendStringInfoCharMacro(buffer, c);

        cnt++;
    }
    while(true);

    return white_only ? 0 : cnt;
}

static struct Parser* parser_init(AttInMetadata *meta, size_t size)
{
    struct Parser* parser = (struct Parser*) palloc(sizeof(ParserData));

    parser->pos = 0;
    parser->file = NULL;
    parser->size = size;
    parser->data = palloc(parser->size);
    parser->values = palloc(ColumnCount(meta->tupdesc) * sizeof(Datum));
    parser->isnull = palloc(ColumnCount(meta->tupdesc) * sizeof(bool));
    parser->meta = meta;
    parser->status = BEGIN;

    for(Index i = 0; i < ColumnCount(meta->tupdesc); i++)
    {
        parser->values[i] = (Datum) 0;
        parser->isnull[i] = true;
    }

    return parser;
}

struct Parser* parser_init_lob(struct LargeObjectDesc* in, struct AttInMetadata *meta)
{
    struct Parser* parser = parser_init(meta, BUFFER_SIZE);

    parser->file = in;
    parser->size = 0;

    return parser;
}

struct Parser* parser_init_varchar(VarChar* in, struct AttInMetadata *meta)
{
    struct Parser* parser = parser_init(meta, VARSIZE(in) - VARHDRSZ);

    memcpy(parser->data, VARDATA(in), parser->size);

    return parser;
}

dummyret parser_close(struct Parser* parser)
{
    pfree(parser->data);

    if(parser->file)
    {
        close_lo_relation(true);
        inv_close(parser->file);
    }

    pfree(parser->values);
    pfree(parser->isnull);
    pfree(parser);
}

static dummyret parser_set_column_value(struct Parser* parser, Index columnIndex, StringInfo value)
{
    if(columnIndex < ColumnCount(parser->meta->tupdesc) && !ColumnIsDropped(parser->meta->tupdesc, columnIndex))
    {
        elog(DEBUG1, "%s of %d: %s"
            , NameStr(*ColumnName(parser->meta->tupdesc, columnIndex))
            , ColumnType(parser->meta->tupdesc, columnIndex)
            , StringInfoToCString(value));

        if(IsColumnNumericValue(parser->meta->tupdesc, columnIndex))
        { // treatment for numeric suffix sign notation
            Pointer pbegin = StringInfoToCString(value);
            Pointer psign = pbegin + value->len - 1;

            NormalizeNumericSignSuffix(pbegin, psign);
        }

        parser->values[columnIndex] = InputFunctionCall(&parser->meta->attinfuncs[columnIndex]
            , StringInfoToCString(value)
            , parser->meta->attioparams[columnIndex]
            , parser->meta->atttypmods[columnIndex]);
        parser->isnull[columnIndex] = false;
    }
}

static dummyret parser_set_column_array(struct Parser* parser, Index columnIndex, List *l, size_t dim)
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

    if(columnIndex < ColumnCount(parser->meta->tupdesc) && !ColumnIsDropped(parser->meta->tupdesc, columnIndex))
    {
        size_t count = list_length(l);
        elog(DEBUG1, "%s of %d: sizeof %ld in %ld dimension"
            , NameStr(*ColumnName(parser->meta->tupdesc, columnIndex))
            , ColumnType(parser->meta->tupdesc, columnIndex)
            , count
            , dim);

        appendStringInfoCharMacro(result, '{');
        if(dim > 1)
        {
            ListCell *cell = NULL;

            foreach(cell, l)
            {
                List* ll = lfirst(cell);

                appendStringInfoCharMacro(result, '{');
                ArrayInnerTraversal(ll, list_length(ll), result);
                appendStringInfoCharMacro(result, '}');
                appendStringInfoCharMacro(result, cell == list_tail(l) ? ' ' : ',');
            }
        }
        else
            ArrayInnerTraversal(l, list_length(l), result);

        appendStringInfoCharMacro(result, '}');

        elog(DEBUG1, "Formed to: %s", StringInfoToCString(result));
        parser->values[columnIndex] = InputFunctionCall(&parser->meta->attinfuncs[columnIndex]
            , StringInfoToCString(result)
            , parser->meta->attioparams[columnIndex]
            , parser->meta->atttypmods[columnIndex]);
        parser->isnull[columnIndex] = false;
        pfree(result->data);
        pfree(result);
    }
    else
        elog(DEBUG1, "Column: %d not exist", columnIndex);

#undef ArrayInnerTraversal
}

static dummyret parser_set_parameter(struct Parser* parser, const char* name, const char* s)
{
    Index idx = 0;
    StringInfo value = makeStringInfo();

    appendStringInfoString(value, s);
    idx = find_column_by_name(parser->meta->tupdesc, name);

    if(IsColumnNumericArray(parser->meta->tupdesc, idx))
    {
        List *l = split(StringInfoToCString(value), " ");
        parser_set_column_array(parser, idx, l, 1);
        list_free_deep(l);
    }
    else
        parser_set_column_value(parser, idx, value);
    pfree(value->data);
    pfree(value);
}

dummyret parser_globals(struct Parser* parser)
{
    StringInfo line = makeStringInfo();

    Assert(parser);

    while(true)
    {
        int result = read_line(parser, line);
        bool bcomment = false;
        Pointer  eq = NULL;

        if(result == EOF)
            break;
        else if(result == 0)
            continue;

        bcomment = is_comment(line);

        if(!bcomment && parser->status == BEGIN && !strcmp (BEGIN_IONS_STR,StringInfoToCString(line)))
        {
            parser->status = BEGIN_IONS;
            break;
        }
        else if(!bcomment && parser->status == BEGIN && PointerIsValid(eq = strchr(StringInfoToCString(line), '=')))
        {
            *eq++ = '\0';
            parser_set_parameter(parser, StringInfoToCString(line), eq);
        }
        resetStringInfo(line);
    }

    pfree(line->data);
    pfree(line);
}

bool parser_next(struct Parser* parser)
{
    StringInfo line = makeStringInfo();
    List *mzs = NULL;
    List *peaks = NULL;

    elog(DEBUG1, "parser_next");
    do
    {
        int result = 0;
        Pointer  eq = NULL;

        result = read_line(parser, line);

        if(result == EOF)
        {
            parser->status = END;
            break;
        }
        else if(result == 0)
        {
            resetStringInfo(line);
            continue;
        }

        if(is_comment(line))
        {
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                , errmsg("Missplaced comment")));
        }
        else if(parser->status == BEGIN && !strcmp (BEGIN_IONS_STR,StringInfoToCString(line)))
        {
            elog(DEBUG1, BEGIN_IONS_STR);
            parser->status = BEGIN_IONS;
        }
        else if(parser->status == BEGIN_IONS && !strcmp (END_IONS_STR,StringInfoToCString(line)))
        {
            Index idx = 0;
            List *spectrums = NULL;

            if(mzs && peaks)
            {
                spectrums = lappend(spectrums, mzs);
                spectrums = lappend(spectrums, peaks);

                if(!OidIsValid(spectrumOid))
                    spectrumOid = TypenameGetTypid("spectrum");
                
                idx = find_column_by_oid(parser->meta->tupdesc, spectrumOid);
                parser_set_column_array(parser, idx, spectrums, SPECTRUM_ARRAY_DIM);
                list_free_deep(spectrums);
            }
            parser->status = END_IONS;
        }
        else if(parser->status == BEGIN_IONS && PointerIsValid(eq = strchr(StringInfoToCString(line), '=')))
        {
            *eq++ = '\0';
            parser_set_parameter(parser, StringInfoToCString(line), eq);
        }
        else if(parser->status == BEGIN_IONS)
        {
            StringInfo value = makeStringInfo();
            List *l = NIL;

            appendStringInfoString(value, StringInfoToCString(line));
            l = split(StringInfoToCString(value), " ");

            if(list_length(l) == SPECTRUM_ARRAY_DIM)
            {
                mzs = lappend(mzs, list_nth(l, 0));
                peaks = lappend(peaks, list_nth(l, 1));
            }
            else 
                ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                    , errmsg("Unknown format mass spectrum format: %s", StringInfoToCString(line))));

            pfree(value->data);
            pfree(value);
        }
        else
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED)
                , errmsg("Unknown format line: %s", StringInfoToCString(line))));

        resetStringInfo(line);
    }
    while(parser->status < END_IONS);

    parser->status = parser->status != END ? BEGIN : END;
    pfree(line);
    return parser->status != END;
}

Datum parser_get_tuple(struct Parser* parser)
{
    return HeapTupleGetDatum(
        heap_form_tuple(parser->meta->tupdesc, parser->values, parser->isnull)
    );
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

        SRF_RETURN_NEXT(funcctx, parser_get_tuple(parser));

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
        SRF_RETURN_NEXT(funcctx, parser_get_tuple(parser));

    parser_close(parser);
    SRF_RETURN_DONE(funcctx);
}

void _PG_init()
{
    Oid spaceid = LookupExplicitNamespace("pgms", true);
    if(OidIsValid(spaceid))
    {
#if PG_VERSION_NUM >= 120000
        spectrumOid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
#else
        spectrumOid = GetSysCacheOid2(TYPENAMENSP, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
#endif
    }
}
