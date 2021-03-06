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

#ifndef PGMS_H_
#define PGMS_H_

#include <postgres.h>
#include <funcapi.h>

#define NEGATIVE_SIGN           '-'
#define POSITIVE_SIGN           '+'

typedef struct
{
   float4 mz;
   float4 intenzity;
} spectrum_t;

extern Oid spectrumOid;

char* reverse_postfix_sign(char*);
void set_spectrum(AttInMetadata*, Datum*, bool*, spectrum_t*, int);
int spectrum_cmp(const void*, const void*);

#endif /* PGMS_H_ */
