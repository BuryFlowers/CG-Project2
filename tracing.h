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

    PathPoint(IntersectionPoint IP, vec3 Wi, float T, vec3 F, float P) {

        ip = IP;
        wi = Wi;
        t = T;
        f = F;
        p = P;
  
    }
     
    IntersectionPoint ip;
    vec3 wi;
    float t;
    vec3 f;
    float p;

};

#endif // !TRACING
