#pragma once
#ifndef LIGHT
#define LIGHT
#include "mesh.h"
#include <algorithm>

using namespace glm;

extern class Material;

class Light : public Mesh{

public:

	Light() {

		mesh.clear();
		radiance = vec3(1.0);
		A = 0;
		v1 = NULL;
		v2 = NULL;
		mat = NULL;

	}

	Light(vec3 r, std::string name) {

		mesh.clear();
		radiance = r;
		A = 0;
		v1 = NULL;
		v2 = NULL;
		mat = NULL;
		matName = name;

	}

	virtual void AABB(vec3& v1, vec3& v2) {

		assert(this->v1 != NULL);
		assert(this->v2 != NULL);
		v1 = this->v1[0];
		v2 = this->v2[0];

	}

	virtual float area() { return A; }

	void addMesh(Mesh* m) {

		mesh.push_back(m);
		A += m->area();

	}

	bool intersectChildren(int l, int r, int AABB_index, Ray ray, float& t, IntersectionPoint& IP) {

		if (l == r) return mesh[l]->intersect(ray, t, IP);

		vec3 v1 = this->v1[AABB_index], v2 = this->v2[AABB_index];

		vec3 d = ray.direction();
		vec3 o = ray.origin();

		float t0, t1;
		float tmin = 0, tmax = 0;
		bool firstT = false;

		if (fabs(d.x) > 1e-5) {

			t0 = (v1.x - o.x) / d.x;
			t1 = (v2.x - o.x) / d.x;
			if (t0 > t1) std::swap(t0, t1);
			if (!firstT) tmin = t0, tmax = t1, firstT = true;
			else {

				if (tmin < t0) tmin = t0;
				if (tmax > t1) tmax = t1;

			}

		}

		if (fabs(d.y) > 1e-5) {

			t0 = (v1.y - o.y) / d.y;
			t1 = (v2.y - o.y) / d.y;
			if (t0 > t1) std::swap(t0, t1);
			if (!firstT) tmin = t0, tmax = t1, firstT = true;
			else {

				if (tmin < t0) tmin = t0;
				if (tmax > t1) tmax = t1;

			}

			if (tmin > tmax) return false;

		}

		if (fabs(d.z) > 1e-5) {

			t0 = (v1.z - o.z) / d.z;
			t1 = (v2.z - o.z) / d.z;
			if (t0 > t1) std::swap(t0, t1);
			if (!firstT) tmin = t0, tmax = t1, firstT = true;
			else {

				if (tmin < t0) tmin = t0;
				if (tmax > t1) tmax = t1;

			}

			if (tmin > tmax + 1e-3) return false;

		}

		if (!firstT) return false;

		int mid = (l + r) / 2;
		if (!intersectChildren(l, mid, AABB_index * 2, ray, t, IP)) return intersectChildren(mid + 1, r, AABB_index * 2 + 1, ray, t, IP);

		float tmpT;
		IntersectionPoint tmpIP;
		if (!intersectChildren(mid + 1, r, AABB_index * 2 + 1, ray, tmpT, tmpIP)) return true;

		if (tmpT < t || (fabs(tmpT - t) < 1e-2 && length(tmpIP.mat->GetLightRadiance()) > length(IP.mat->GetLightRadiance()))) {

			t = tmpT;
			IP = tmpIP;

		}

		return true;

	}

	virtual bool intersect(Ray r, float& t, IntersectionPoint& IP) {

		return intersectChildren(0, mesh.size() - 1, 1, r, t, IP);
			
	}

	void updateBVH(int l, int r, int AABB_index, int cmpA) {

		if (l == r) {

			mesh[l]->AABB(v1[AABB_index], v2[AABB_index]);
			return;

		}

		if (cmpA == 0)	std::sort(mesh.begin() + l, mesh.begin() + r, cmpx);
		else if (cmpA == 1) std::sort(mesh.begin() + l, mesh.begin() + r, cmpy);
		else std::sort(mesh.begin() + l, mesh.begin() + r, cmpz);

		updateBVH(l, (l + r) / 2, AABB_index * 2, (cmpA + 1) % 3);
		updateBVH((l + r) / 2 + 1, r, AABB_index * 2 + 1, (cmpA + 1) % 3);

		v1[AABB_index] = v1[AABB_index * 2];
		v2[AABB_index] = v2[AABB_index * 2];

		if (v1[AABB_index].x > v1[AABB_index * 2 + 1].x) v1[AABB_index].x = v1[AABB_index * 2 + 1].x;
		if (v1[AABB_index].y > v1[AABB_index * 2 + 1].y) v1[AABB_index].y = v1[AABB_index * 2 + 1].y;
		if (v1[AABB_index].z > v1[AABB_index * 2 + 1].z) v1[AABB_index].z = v1[AABB_index * 2 + 1].z;

		if (v2[AABB_index].x < v2[AABB_index * 2 + 1].x) v2[AABB_index].x = v2[AABB_index * 2 + 1].x;
		if (v2[AABB_index].y < v2[AABB_index * 2 + 1].y) v2[AABB_index].y = v2[AABB_index * 2 + 1].y;
		if (v2[AABB_index].z < v2[AABB_index * 2 + 1].z) v2[AABB_index].z = v2[AABB_index * 2 + 1].z;

	}

	void buildBVH() {

		v1 = new vec3[mesh.size() * 3 + 1];
		v2 = new vec3[mesh.size() * 3 + 1];
		updateBVH(0, mesh.size() - 1, 1, 0);

	}

	virtual vec3 uniformSampling() {

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

		return m->uniformSampling();

	}

	vec3 randomLightRay(vec3 startPoint, Ray& r) {

		vec3 o = this->uniformSampling();
		vec3 d = normalize(o - startPoint);
		r = Ray(startPoint, d);
		return o;

	}

	vec3 Radiance() { return radiance; }
	void SetMat(Material* material) { mat = material; }
	Material* Mat() { return mat; }
	std::string Name() { return matName; }

private:

	std::vector<Mesh*> mesh;
	vec3* v1;
	vec3* v2;
	vec3 radiance;
	float A;
	Material* mat;
	std::string matName;

};

#endif // !LIGHT
