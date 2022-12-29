#pragma once
#ifndef RAY
#define RAY
#include "glm/glm.hpp"
#include "material.h"

using namespace glm;

class Ray {

public:

	Ray() {

		o = vec3(0);
		d = vec3(0.0, 0.0, -1.0);

	}

	Ray(vec3 position, vec3 direction) {

		o = position;
		d = direction;
		d = normalize(d);

	}

	vec3 origin() { return o; }
	vec3 direction() { return d; }
	vec3 pointAt(float t) { return o + t * d; }

private:

	vec3 o;
	vec3 d;

};

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

#endif // !RAY
