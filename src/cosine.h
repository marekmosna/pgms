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

static inline float4 calc_score(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power)
{
    if(float4_eq(mz_power, 0.0))
    {
        if(float4_eq(intensity_power, 1.0))
            return float4_mul(intensity1, intensity2);
        else
            return powf(float4_mul(intensity1, intensity2), intensity_power);
    }
    else
        return powf(float4_mul(mz1, mz2), mz_power) * powf(float4_mul(intensity1, intensity2), intensity_power);
}


static inline float4 calc_norm(const float4 *restrict intensities,
    const float4 *restrict mz,
    const size_t len,
    const float4 intensity_power,
    const float4 mz_power)
{
    float4 result = 0.0f;

    for(size_t i = 0; i < len; i++)
    {
        if(float4_eq(mz_power, 0.0))
        {
            if(float4_eq(intensity_power, 1.0))
                result = float4_pl(result, float4_mul(intensities[i], intensities[i]));
            else
                result = float4_pl(result, powf(intensities[i], 2 * intensity_power));
        }
        else
            result = float4_pl(result, powf(mz[i], 2 * mz_power) * powf(intensities[i], 2 * intensity_power));
    }

    return result;
}

#endif /* COSINE_H */
