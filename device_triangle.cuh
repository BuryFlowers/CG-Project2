#pragma once
#ifndef D_TRIANGLE
#define D_TRIANGLE
#include "device_vector.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/extrema.h>

struct d_triangle {

	d_vec3 v[3];
	d_vec3 n[3];
	d_vec2 uv[3];

	/*__device__ float intersect(d_vec3 start, d_vec3 direction) {

		d_vec3 BA, BC;
		BA = v[0] - v[1];
		BC = v[2] - v[1];
		d_vec3 planeNormal;
		planeNormal = d_cross(BA, BC);
		float area = planeNormal.length();

		if (d_fabs(d_dot(direction, planeNormal)) < 1e-5) return -1.0f;
		if (area == 0) return -1.0f;

		float t = (d_dot(v[0], planeNormal) - d_dot(start, planeNormal)) / d_dot(direction, planeNormal);
		if (t < 1e-3) return -1.0f;

		d_vec3 p = start + direction * t;
		d_vec3 t1 = d_cross(v[0] - p, v[1] - p);
		d_vec3 t2 = d_cross(v[1] - p, v[2] - p);
		d_vec3 t3 = d_cross(v[2] - p, v[0] - p);

		if (d_dot(t1, t2) < 0) return -1.0f;
		if (d_dot(t1, t3) < 0) return -1.0f;
		if (d_dot(t2, t3) < 0) return -1.0f;

		return t;

	}*/

};


#endif // !D_TRIANGLE
