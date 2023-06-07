/* 
 * This file is part of the PGMS distribution (https://github.com/genesissoftware-tech/pgms or https://ip-147-251-124-124.flt.cloud.muni.cz/chemdb/pgms).
 * Copyright (c) 2022 Marek Mosna (marek.mosna@genesissoftware.eu).
 * Copyright (c) 2022 Jakub Galgonek (jakub.galgonek@uochb.cas.cz) as initial contributor
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

#ifndef COSINE_H
#define COSINE_H

#include <utils/float.h>

typedef float4 (*calc_score_func_t)(const float4,
    const float4, 
    const float4,
    const float4,
    const float4,
    const float4);

calc_score_func_t determine_calc_score(const float4 mz_power,const float4 intenzity_power);

float4 calc_norm(const float4 *restrict intensities,
    const float4 *restrict mz,
    const size_t len,
    const float4 intensity_power,
    const float4 mz_power);

#endif /* COSINE_H */
