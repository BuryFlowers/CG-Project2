#pragma once
#ifndef TRACING
#define TRACING
#include "glm/glm.hpp"
#include "stb_image.h"
#include "camera.h"
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

    PathPoint(IntersectionPoint IP) {

        ip = IP;

    }
     
    IntersectionPoint ip;
    vec3 wi;
    vec3 wo;
    float cameraP;
    float lightP;

};

enum IntersectionType { HITLIGHT, HITOBJECT, NOHIT };


#endif // !TRACING
