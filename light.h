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
		A = 0;

	}

	Light(vec3 r) {

		mesh.clear();
		radiance = r;
		A = 0;

	}

	void addMesh(Mesh* m) {

		mesh.push_back(m);
		A += m->area();

	}

	bool intersect(Ray r, float& t, IntersectionPoint& IP) {

		bool flag = false;
		for (int i = 0; i < mesh.size(); i++) {

			float tmpT;
			IntersectionPoint tmpIP;
			if (mesh[i]->intersect(r, tmpT, tmpIP) && tmpT < t) {

				flag = true;
				t = tmpT;
				IP = tmpIP;

			}

		}

		return flag;
			
	}

	float randomLightRay(vec3 point, vec3& d) {

		float p = rand() * 1.0f / RAND_MAX * A;
		float a = 0;
		Mesh* m = mesh[mesh.size() - 1];
		for (int i = 0; i < mesh.size(); i++) {

			a += mesh[i]->area();
			if (a >= p) {

				m = mesh[i];
				break;

			}

		}

		d = m->uniformSampling() - point;
		d = normalize(d);
		//return length(direction) * length(direction) / ()
		return 1.0f;

	}

	vec3 Radiance() { return radiance; }

private:

	std::vector<Mesh*> mesh;
	vec3 radiance;
	float A;

};

#endif // !LIGHT
