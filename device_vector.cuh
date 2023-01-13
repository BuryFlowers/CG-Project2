#pragma once
#ifndef D_VECTOR
#define D_VECTOR

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/extrema.h>
#include "glm/glm.hpp"

struct d_vec2 {

	float x, y;

	__host__ __device__ d_vec2() {

		x = 0.0f;
		y = 0.0f;

	}

	__host__ __device__ d_vec2(float X, float Y) {

		x = X;
		y = Y;

	}

	__device__ float length() {

		return sqrtf(x * x + y * y);

	}

	__device__ void normalize() {

		float len = this->length();
		x /= len;
		y /= len;

	}

	__host__ __device__ d_vec2 operator * (float x) {

		return d_vec2(this->x * x, this->y * x);

	}

	__host__ __device__ d_vec2 operator + (const d_vec2 v) {

		return d_vec2(this->x + v.x, this->y + v.y);

	}

};

struct d_vec3 {

	float x, y, z;

	__host__ __device__ d_vec3() {

		x = 0.0f;
		y = 0.0f;
		z = 0.0f;

	}

	__host__ __device__ d_vec3(float X, float Y, float Z) {

		x = X;
		y = Y;
		z = Z;

	}

	__host__ __device__ d_vec3 operator + (const d_vec3 v) {

		return d_vec3(this->x + v.x, this->y + v.y, this->z + v.z);

	}

	__host__ __device__ d_vec3 operator - (const d_vec3 v) {

		return d_vec3(this->x - v.x, this->y - v.y, this->z - v.z);

	}

	__host__ __device__ d_vec3 operator * (float x) {

		return d_vec3(this->x * x, this->y * x, this->z * x);

	}

	__host__ __device__ d_vec3 operator / (float x) {

		return d_vec3(this->x / x, this->y / x, this->z / x);

	}

	__device__ float length() {

		return sqrtf(x * x + y * y + z * z);

	}

	__device__ void normalize() {

		float len = this->length();
		x /= len;
		y /= len;
		z /= len;

	}

};

__device__ float d_dot(d_vec3 a, d_vec3 b);

__device__ float d_fabs(float a);

__device__ d_vec3 d_cross(d_vec3 a, d_vec3 b);

#endif // !D_VECTOR