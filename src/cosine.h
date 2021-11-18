#ifndef COSINE_H
#define COSINE_H

#include <math.h>


static inline float calc_score(const float intensity1, const float intensity2, const float mz1,  const float mz2, const float intensity_power, const float mz_power)
{
    if(mz_power == 0 && intensity_power == 1)
        return intensity1 * intensity2;
    else if(mz_power == 0)
        return powf(intensity1 * intensity2, intensity_power);
    else
        return powf(mz1 * mz2, mz_power) * powf(intensity1 * intensity2, intensity_power);
}


static inline float calc_norm(const float const *restrict intensities, const float const *restrict mz, const uint len, const float intensity_power, const float mz_power)
{
    float result = 0;

    for(uint i = 0; i < len; i++)
    {
        if(mz_power == 0 && intensity_power == 1)
            result += intensities[i] * intensities[i];
        else if(mz_power == 0)
            result += powf(intensities[i], 2 * intensity_power);
        else
            result += powf(mz[i], 2 * mz_power) * powf(intensities[i], 2 * intensity_power);
    }

    return result;
}


static inline float calc_simple_norm(float *restrict spec_intensities, int len)
{
    float result = 0;

    for(int i = 0; i < len; i++)
        result += spec_intensities[i] * spec_intensities[i];

    return result;
}

#endif /* COSINE_H */
