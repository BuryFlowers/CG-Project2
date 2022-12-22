#pragma once
#ifndef MESH
#define MESH
#include "glm/glm.hpp"
#include "ray.h"
#include "material.h"
#include <algorithm>

using namespace glm;

class Mesh {

public:

	virtual void AABB(vec3& v1, vec3& v2) = 0;
	virtual bool intersect(Ray r, float &t, IntersectionPoint &IP) = 0;

};

class Triangle : public Mesh {

public:

	Triangle() {

		mat = NULL;

	}
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

	virtual bool intersect(Ray r, float& t, IntersectionPoint& IP) {

		if (fabs(dot(r.direction(), planeNormal)) < 1e-5) return false;

		t = (v0N - dot(r.origin(), planeNormal)) / dot(r.direction(), planeNormal);
		if (t < 0) return false;

		vec3 p = r.pointAt(t);
		float w0, w1, w2;

		w0 = length(cross(v[1] - p, v[2] - p)) / A;
		w1 = length(cross(v[0] - p, v[2] - p)) / A;
		w2 = 1 - w0 - w1;

		if (w0 < 0 || w0 > 1.0f) return false;
		if (w1 < 0 || w1 > 1.0f) return false;
		if (w2 < 0 || w2 > 1.0f) return false;

		IP.mat = this->mat;
		IP.p = p;
		IP.n = w0 * n[0] + w1 * n[1] + w2 * n[2];
		IP.uv = w0 * uv[0] + w1 * uv[1] + w2 * uv[2];

		return true;

	}

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

bool cmpx(Mesh& a, Mesh& b) {

	vec3 av1, av2;
	vec3 bv1, bv2;
	a.AABB(av1, av2);
	b.AABB(bv1, bv2);
	if (av1.x == bv1.x) return av2.x < bv2.x;
	return av1.x < bv1.x;

}

bool cmpy(Mesh& a, Mesh& b) {

	vec3 av1, av2;
	vec3 bv1, bv2;
	a.AABB(av1, av2);
	b.AABB(bv1, bv2);
	if (av1.y == bv1.y) return av2.y < bv2.y;
	return av1.y < bv1.y;

}

bool cmpz(Mesh& a, Mesh& b) {

	vec3 av1, av2;
	vec3 bv1, bv2;
	a.AABB(av1, av2);
	b.AABB(bv1, bv2);
	if (av1.z == bv1.z) return av2.z < bv2.z;
	return av1.z < bv1.z;

}

//class BVH : public Mesh {
//
//public:
//
//	BVH() {
//
//		printf("[Error]You are trying to build an empty BVH");
//		exit(-5);
//
//	}
//
//	BVH(Mesh* meshes, int meshNum, int cmpA) {
//
//		meshList = meshes;
//		num = meshNum;
//		left = NULL;
//		right = NULL;
//
//		if (meshNum >= 2) {
//
//			if (cmpA == 0)	std::sort(meshes, meshes + meshNum, cmpx);
//			else if (cmpA == 1) std::sort(meshes, meshes + meshNum, cmpy);
//			else std::sort(meshes, meshes + meshNum, cmpz);
//
//			left = new BVH(meshes, meshNum / 2, (cmpA + 1) % 3);
//			right = new BVH(meshes + meshNum / 2, meshNum - meshNum / 2, (cmpA + 1) % 3);
//
//		}
//
//	}
//
//	virtual void AABB(vec3& v1, vec3& v2) {
//
//		if (left == NULL || right == NULL) {
//
//			meshList[0].AABB(v1, v2);
//			return;
//
//		}
//
//		vec3 v3, v4;
//		left->AABB(v1, v2);
//		right->AABB(v3, v4);
//
//		if (v1.x < v3.x) v1.x = v3.x;
//		if (v1.y < v3.y) v1.y = v3.y;
//		if (v1.z < v3.z) v1.z = v3.z;
//
//		if (v2.x > v4.x) v2.x = v4.x;
//		if (v2.y > v4.y) v2.y = v4.y;
//		if (v2.z > v4.z) v2.z = v4.z;
//
//	}
//
//	virtual bool intersect(Ray r, float& t, IntersectionPoint& IP) {
//
//		if (left == NULL || right == NULL) return meshList[0].intersect(r, t, IP);
//
//		vec3 v1, v2;
//		AABB(v1, v2);
//
//		vec3 d = r.direction();
//		vec3 o = r.origin();
//
//		float t0, t1;
//		float tmin, tmax;
//		bool firstT = false;
//
//		if (fabs(d.x) > 1e-5) {
//
//			t0 = (v1.x - o.x) / d.x;
//			t1 = (v2.x - o.x) / d.x;
//			if (t0 > t1) std::swap(t0, t1);
//			if (!firstT) tmin = t0, tmax = t1, firstT = true;
//			else {
//
//				if (tmin < t0) tmin = t0;
//				if (tmax > t1) tmax = t1;
//
//			}
//
//		}
//
//		if (fabs(d.y) > 1e-5) {
//
//			t0 = (v1.y - o.y) / d.y;
//			t1 = (v2.y - o.y) / d.y;
//			if (t0 > t1) std::swap(t0, t1);
//			if (!firstT) tmin = t0, tmax = t1, firstT = true;
//			else {
//
//				if (tmin < t0) tmin = t0;
//				if (tmax > t1) tmax = t1;
//
//			}
//
//			if (tmin > tmax) return false;
//
//		}
//
//		if (fabs(d.z) > 1e-5) {
//
//			t0 = (v1.z - o.z) / d.z;
//			t1 = (v2.z - o.z) / d.z;
//			if (t0 > t1) std::swap(t0, t1);
//			if (!firstT) tmin = t0, tmax = t1, firstT = true;
//			else {
//
//				if (tmin < t0) tmin = t0;
//				if (tmax > t1) tmax = t1;
//
//			}
//
//			if (tmin > tmax) return false;
//
//		}
//
//		if (firstT) return false;
//		if (!left->intersect(r, t, IP)) return right->intersect(r, t, IP);
//			
//
//		float tmpT;
//		IntersectionPoint tmpIP;
//		if (!right->intersect(r, tmpT, tmpIP)) return true;
//
//		if (tmpT < t) {
//
//			t = tmpT;
//			IP = tmpIP;
//
//		}
//
//		return true;
//
//	}
//
//private:
//
//	Mesh* meshList;
//	int num;
//	BVH* left;
//	BVH* right;
//
//};

#endif // !MESH
