#pragma once
#ifndef TRACING
#define TRACING
#include "glm/glm.hpp"
#include "stb_image.h"
#include <string.h>

using namespace glm;

extern class Material;

struct IntersectionPoint {

    Material* mat;
    vec3 p;
    vec3 n;
    vec2 uv;

};

struct PathPoint {

    PathPoint(vec3 X, vec3 N, float F, float P) {

        x = X;
        n = N;
        f = F;
        p = P;

    }

    vec3 x;
    vec3 n;
    float f;
    float p;

};


#endif // !TRACING
