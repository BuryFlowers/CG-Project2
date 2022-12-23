#pragma once
#ifndef LIGHT
#define LIGHT
#include "mesh.h"
#include <algorithm>

using namespace glm;

class Light {

public:

	Light() {

		mesh.clear();
		radiance = vec3(1.0);

	}

	Light(vec3 r) {

		radiance = r;

	}

	void addMesh(Mesh* m) {

		mesh.push_back(m);

	}

private:

	std::vector<Mesh*> mesh;
	vec3 radiance;

};

#endif // !LIGHT
