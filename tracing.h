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

    PathPoint(vec3 X, vec3 N, vec3 Wi, vec2 UV, float F, float P, Material* Mat) {

        x = X;
        n = N;
        wi = Wi;
        uv = UV;
        f = F;
        p = P;
        mat = Mat;

    }
     
    vec3 x;
    vec3 n;
    vec3 wi;
    vec2 uv;
    float f;
    float p;
    Material* mat;

};


#endif // !TRACING
