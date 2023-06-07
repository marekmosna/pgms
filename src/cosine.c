#include <postgres.h>
#include "cosine.h"

static float4 calc_score_simple(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power);
static float4 calc_score_full(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power);
static float4 calc_score_no_mz_power(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power);
static float4 calc_score_ident_intenzity_power(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power);

calc_score_func_t determine_calc_score(const float4 mz_power,const float4 intensity_power)
{
    if(float4_eq(mz_power, 0.0))
    {
        if(float4_eq(intensity_power, 1.0))
            return calc_score_simple;
        else
            return calc_score_no_mz_power;
    }
    else
    {
        if(float4_eq(intensity_power, 1.0))
            return calc_score_ident_intenzity_power;
        else
            return calc_score_full;
    }
}

static float4 calc_score_simple(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power)
{
    return float4_mul(intensity1, intensity2);
}

static float4 calc_score_full(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power)
{
    return float4_mul(
        powf(float4_mul(mz1, mz2), mz_power),
        powf(float4_mul(intensity1, intensity2), intensity_power)
    );
}

static float4 calc_score_no_mz_power(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power)
{
    return powf(float4_mul(intensity1, intensity2), intensity_power);
}

static float4 calc_score_ident_intenzity_power(const float4 intensity1,
    const float4 intensity2, 
    const float4 mz1,
    const float4 mz2,
    const float4 intensity_power,
    const float4 mz_power)
{
    return float4_mul(
        powf(float4_mul(mz1, mz2), mz_power),
        float4_mul(intensity1, intensity2)
    );
}

float4 calc_norm(const float4 *restrict intensities,
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
                result = float4_pl(result, powf(intensities[i], float4_mul(2.0f, intensity_power)));
        }
        else
        {
            if(float4_eq(intensity_power, 1.0))
                result = float4_pl(result, 
                    float4_mul(powf(mz[i], float4_mul(2.0f, mz_power)),
                    float4_mul(intensities[i], intensities[i]))
                );
            else
                result = float4_pl(result,
                    float4_mul(powf(mz[i], float4_mul(2.0f, mz_power)),
                    powf(intensities[i], float4_mul(2.0f, intensity_power)))
                );
        }
    }

    return result;
}
