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

#ifndef PGMS_H_
#define PGMS_H_

#include <postgres.h>
#include <funcapi.h>

extern Oid spectrumOid;

#define SPECTRUM_ARRAY_DIM      2

#define ColumnName(td, idx)         ((Name)(&(TupleDescAttr((td), (idx))->attname)))
#define ColumnIsDropped(td, idx)    ((bool)TupleDescAttr((td), (idx))->attisdropped)
#define ColumnCount(td)             ((size_t)(td)->natts)
#define ColumnType(td, idx)         ((Oid)TupleDescAttr((td), (idx))->atttypid)
#define StringInfoToCString(si)     ((Pointer)((si)->data + (si)->cursor))

#define IsColumnNumericValue(td, idx)                       \
            (ColumnType((td), (idx)) == INT4OID ||          \
            ColumnType((td), (idx)) == NUMERICOID ||        \
            ColumnType((td), (idx)) == INT8OID ||           \
            ColumnType((td), (idx)) == FLOAT4OID ||         \
            ColumnType((td), (idx)) == FLOAT8OID)

#define IsColumnNumericArray(td, idx)                       \
            (ColumnType((td), (idx)) == FLOAT4ARRAYOID ||   \
            ColumnType((td), (idx)) == FLOAT8ARRAYOID ||    \
            ColumnType((td), (idx)) == INT4ARRAYOID ||      \
            ColumnType((td), (idx)) == INT8ARRAYOID ||      \
            ColumnType((td), (idx)) == NUMERICARRAYOID)

#define TrailSpaces(b, e, dir)    do {                      \
            while(likely((b) != (e))                        \
                && isspace((uint8) *(b))) (b) += (dir);     \
            } while(false)

#define TrailLeftSpaces(b, e)   TrailSpaces((b), (e), 1)
#define TrailRightSpaces(b, e)  TrailSpaces((e), (b), -1)

#define NormalizeNumericSignSuffix(b, e)  do {              \
            TrailLeftSpaces((b), (e));                      \
            TrailRightSpaces((b), (e));                     \
            if(likely((e) != (b)))                          \
            {                                               \
                if(*(e) == '+')                             \
                    *(e) = '\0';                            \
                else if(*(e) == '-')                        \
                {                                           \
                    while((e) != (b))                       \
                    {                                       \
                        char c = *((e) - 1);                \
                        *(e) = c;                           \
                        *(--(e)) = '-';                     \
                    }                                       \
                }                                           \
            }                                               \
        }while(false)

#endif /* PGMS_H_ */
