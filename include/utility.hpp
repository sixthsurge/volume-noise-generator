#ifndef UTILITY_INCLUDED
#define UTILITY_INCLUDED

#include <limits>

const auto uint32Max = std::numeric_limits<uint32_t>::max();

float linearStep(float low, float high, float x) { return clamp((x - low) / (high - low), 0.0f, 1.0f); }

float wrapMax(float x, float max) {
    return glm::mod(max + glm::mod(x, max), max);
}

float wrap(float x, float min, float max) {
    return min + wrapMax(x - min, max - min);
}

// Source: https://nullprogram.com/blog/2018/07/31/
uint lowbias32(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

vec3 xyz(vec4 v) { return vec3(v.x, v.y, v.z); }

#endif // UTILITY_INCLUDED
