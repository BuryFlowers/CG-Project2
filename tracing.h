#pragma once
#ifndef TRACING
#define TRACING
#include "glm/glm.hpp"
#include "stb_image.h"
#include "camera.h"
#include <string.h>

using namespace glm;

//This header defines some structure of tracer situation information

extern class Material;

//The position, normal and uvs of a intersection point
struct IntersectionPoint {

    Material* mat;
    vec3 p;
    vec3 n;
    vec2 uv;

};

//The IP , incident direction and out direction of a point in the light path 
struct PathPoint {

    PathPoint(IntersectionPoint IP) {

        ip = IP;

    }
     
    IntersectionPoint ip;
    vec3 wi;
    vec3 wo;

};

enum IntersectionType { HITLIGHT, HITOBJECT, NOHIT };


#endif // !TRACING
