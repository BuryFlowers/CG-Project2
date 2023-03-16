#pragma once
#ifndef TRIANGLE
#define TRIANGLE

#include "glm/glm.hpp"
#include "ray.h"
#include "material.h"
#include "mesh.h"
#include "light.h"
#include <vector>

using namespace glm;

//A triangle mesh
class Triangle : public Mesh {

public:

	Triangle(vec3 V[3], vec3 N[3], vec2 UV[3], Material* MAT) {

		memcpy(v, V, sizeof(v));
		memcpy(n, N, sizeof(n));
		memcpy(uv, UV, sizeof(uv));
		planeNormal = cross(v[1] - v[0], v[2] - v[0]);
		A = length(planeNormal);
		planeNormal = normalize(planeNormal);
		v0N = dot(v[0], planeNormal);
		mat = MAT;

		v1 = v2 = v[0];

		if (v[1].x < v1.x) v1.x = v[1].x;
		if (v[1].y < v1.y) v1.y = v[1].y;
		if (v[1].z < v1.z) v1.z = v[1].z;
		if (v[1].x > v2.x) v2.x = v[1].x;
		if (v[1].y > v2.y) v2.y = v[1].y;
		if (v[1].z > v2.z) v2.z = v[1].z;

		if (v[2].x < v1.x) v1.x = v[2].x;
		if (v[2].y < v1.y) v1.y = v[2].y;
		if (v[2].z < v1.z) v1.z = v[2].z;
		if (v[2].x > v2.x) v2.x = v[2].x;
		if (v[2].y > v2.y) v2.y = v[2].y;
		if (v[2].z > v2.z) v2.z = v[2].z;

	}

	virtual void AABB(vec3& v1, vec3& v2) {

		v1 = this->v1;
		v2 = this->v2;

	}

	virtual float area() { return 0.5f * A; }

	virtual bool intersect(Ray r, float& t, IntersectionPoint& IP) {

		if (fabs(dot(r.direction(), planeNormal)) < 1e-5) return false;
		if (A == 0) return false;

		t = (v0N - dot(r.origin(), planeNormal)) / dot(r.direction(), planeNormal);
		//printf("A:%.2lf N:%.2lf %.2lf %.2lf T:%.2lf\n", A, planeNormal.x, planeNormal.y, planeNormal.z, t);
		if (t < 1e-3) return false;

		vec3 p = r.pointAt(t);
		vec3 t1 = cross(v[0] - p, v[1] - p);
		vec3 t2 = cross(v[1] - p, v[2] - p);
		if (dot(t1, t2) < 0) return false;
		vec3 t3 = cross(v[2] - p, v[0] - p);
		if (dot(t1, t3) < 0) return false;
		if (dot(t2, t3) < 0) return false;

		float w0, w1, w2;

		w0 = length(t2) / A;
		w1 = length(t3) / A;
		w2 = 1 - w0 - w1;

		IP.mat = this->mat;
		IP.p = p;
		IP.n = w0 * n[0] + w1 * n[1] + w2 * n[2];
		IP.uv = w0 * uv[0] + w1 * uv[1] + w2 * uv[2];

		return true;

	}

	virtual float uniformSampling(IntersectionPoint& IP) {

		float u1 = rand() * 1.0f / RAND_MAX;
		float u2 = rand() * 1.0f / RAND_MAX;
		IP.mat = this->mat;
		IP.p = (1 - u1) * sqrt(u2) * v[0] + u1 * sqrt(u2) * v[1] + (1 - sqrt(u2)) * v[2];
		IP.n = (1 - u1) * sqrt(u2) * n[0] + u1 * sqrt(u2) * n[1] + (1 - sqrt(u2)) * n[2];
		IP.uv = (1 - u1) * sqrt(u2) * uv[0] + u1 * sqrt(u2) * uv[1] + (1 - sqrt(u2)) * uv[2];

		return 1.0f / this->area();

	}

	Material* Mat() { return mat; }

private:

	vec3 v[3];
	vec3 n[3];
	vec2 uv[3];
	vec3 planeNormal;
	float v0N;
	float A;
	Material* mat;
	vec3 v1, v2;

};

//All triangles of an object, using BVH to accelerate
class TriangleObject : public Mesh {

public:

	TriangleObject() {

		triangles.clear();
		v1 = NULL;
		v2 = NULL;
		A = 0;

	}

	virtual void AABB(vec3& v1, vec3& v2) {

		assert(this->v1 != NULL);
		assert(this->v2 != NULL);
		v1 = this->v1[0];
		v2 = this->v2[0];

	}

	virtual float area() { return A; }

	//Search child nodes to get intersection result
	bool intersectChildren(int l, int r, int AABB_index, Ray ray, float& t, IntersectionPoint& IP) {

		if (l == r) return triangles[l]->intersect(ray, t, IP);

		vec3 v1 = this->v1[AABB_index], v2 = this->v2[AABB_index];

		vec3 d = ray.direction();
		vec3 o = ray.origin();

		float t0, t1;
		float tmin = 0, tmax = 0;
		bool firstT = true;

		if (fabs(d.x) > 1e-5) {

			t0 = (v1.x - o.x) / d.x;
			t1 = (v2.x - o.x) / d.x;
			if (t0 > t1) std::swap(t0, t1);
			if (firstT) tmin = t0, tmax = t1, firstT = false;
			else {

				if (tmin < t0) tmin = t0;
				if (tmax > t1) tmax = t1;

			}

		}

		if (fabs(d.y) > 1e-5) {

			t0 = (v1.y - o.y) / d.y;
			t1 = (v2.y - o.y) / d.y;
			if (t0 > t1) std::swap(t0, t1);
			if (firstT) tmin = t0, tmax = t1, firstT = false;
			else {

				if (tmin < t0) tmin = t0;
				if (tmax > t1) tmax = t1;

			}

			if (tmin > tmax + 1e-3) return false;


		}

		if (fabs(d.z) > 1e-5) {

			t0 = (v1.z - o.z) / d.z;
			t1 = (v2.z - o.z) / d.z;
			if (t0 > t1) std::swap(t0, t1);
			if (firstT) tmin = t0, tmax = t1, firstT = false;
			else {

				if (tmin < t0) tmin = t0;
				if (tmax > t1) tmax = t1;

			}

			if (tmin > tmax + 1e-3) return false;

		}

		if (firstT) return false;

		int mid = (l + r) / 2;
		if (!intersectChildren(l, mid, AABB_index * 2, ray, t, IP)) return intersectChildren(mid + 1, r, AABB_index * 2 + 1, ray, t, IP);

		float tmpT;
		IntersectionPoint tmpIP;
		if (!intersectChildren(mid + 1, r, AABB_index * 2 + 1, ray, tmpT, tmpIP)) return true;

		if (tmpT < t) {

			t = tmpT;
			IP = tmpIP;

		}

		return true;

	}

	virtual bool intersect(Ray r, float& t, IntersectionPoint& IP) {

		return intersectChildren(0, triangles.size() - 1, 1, r, t, IP);

	}

	virtual float uniformSampling(IntersectionPoint& IP) {

		float p = rand() * 1.0f / RAND_MAX * A;
		float a = 0;
		Triangle* t = triangles[triangles.size() - 1];
		for (int i = 0; i < triangles.size(); i++) {

			a += triangles[i]->area();
			if (a >= p) {

				t = triangles[i];
				break;

			}

		}

		t->uniformSampling(IP);
		return 1.0f / A;

	}

	void addTriangle(Triangle* triangle) {

		triangles.push_back(triangle);
		A += triangle->area();

	}

	//Build the complete BVH tree
	void updateBVH(int l, int r, int AABB_index, int cmpA) {

		if (l == r) {

			triangles[l]->AABB(v1[AABB_index], v2[AABB_index]);
			return;

		}

		if (cmpA == 0)	std::sort(triangles.begin() + l, triangles.begin() + r, cmpx);
		else if (cmpA == 1) std::sort(triangles.begin() + l, triangles.begin() + r, cmpy);
		else std::sort(triangles.begin() + l, triangles.begin() + r, cmpz);

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

	//The port of building BVH after adding all triangles to this object
	void buildBVH() {

		v1 = new vec3[triangles.size() * 3 + 1];
		v2 = new vec3[triangles.size() * 3 + 1];
		updateBVH(0, triangles.size() - 1, 1, 0);

	}

private:

	std::vector<Triangle*> triangles;
	vec3* v1;
	vec3* v2;
	float A;

};

#endif // !TRIANGLE
