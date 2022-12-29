#pragma once
#ifndef RAY
#define RAY
#include "glm/glm.hpp"

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

#endif // !RAY
