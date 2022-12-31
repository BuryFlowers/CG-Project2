#pragma once
#ifndef LIGHT
#define LIGHT
#include "mesh.h"
#include <algorithm>

using namespace glm;

extern class Material;

class Light {

public:

	Light() {

		mesh.clear();
		radiance = vec3(1.0);
		A = 0;

	}

	Light(vec3 r, Material* Mat) {

		mesh.clear();
		radiance = r;
		A = 0;
		mat = Mat;

	}

	void addMesh(Mesh* m) {

		mesh.push_back(m);
		A += m->area();

	}

	bool intersect(Ray r, float& t, IntersectionPoint& IP) {

		t = 1e10;
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

	float randomLightRay(vec3 point, Ray& r, IntersectionPoint& lightIP) {

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

		m->uniformSampling(lightIP);
		vec3 d = lightIP.p - point;
		d = normalize(d);
		r = Ray(point, d);
		//return length(direction) * length(direction) / ()
		return 1.0f / A;

	}

	float area() { return A; }
	vec3 Radiance() { return radiance; }
	Material* Mat() { return mat; }

private:

	std::vector<Mesh*> mesh;
	vec3 radiance;
	float A;
	Material* mat;

};

#endif // !LIGHT
