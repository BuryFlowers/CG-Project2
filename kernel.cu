#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/extrema.h>
#include <cuda.h>
//#include <device_functions.h>
//#include "sm_20_atomic_functions.h"
#include "book.h"
#include "glm/glm.hpp"
#include "device_vector.cuh"
#include "device_triangle.cuh"
#include "ray.h"
#include "tracing.h"
#include "material.h"

__device__ float d_dot(d_vec3 a, d_vec3 b) {

	return a.x * b.x + a.y * b.y + a.z * b.z;

}

__device__ float d_fabs(float a) {

	if (a < 0) return a * -1.0f;
	return a;

}

__device__ d_vec3 d_cross(d_vec3 a, d_vec3 b) {

	return d_vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);

}

__global__ void IntersectionCheckKernel(d_vec3 start, d_vec3 direction, d_triangle* triangles, int num, int* id, float* resultT, d_vec3* weights, int* d_lock) {

	int index = blockIdx.x * blockDim.x + threadIdx.x;
	if (index < num) {

		d_triangle T = triangles[index];
		d_vec3 planeNormal;
		planeNormal = d_cross(T.v[1] - T.v[0], T.v[2] - T.v[0]);
		float area = planeNormal.length();
		planeNormal.normalize();

		if (d_fabs(d_dot(direction, planeNormal)) < 1e-5) return;
		if (area == 0) return;

		float t = (d_dot(T.v[1], planeNormal) - d_dot(start, planeNormal)) / d_dot(direction, planeNormal);
		//printf("ID:%d A:%.2lf N:%.2lf %.2lf %.2lf T:%.2lf\n", index, area, planeNormal.x, planeNormal.y, planeNormal.z, t);
		//printf("ID:%d\n", index);
		if (t < 1e-3) return;

		d_vec3 p = start + direction * t;
		d_vec3 t1 = d_cross(T.v[0] - p, T.v[1] - p);
		d_vec3 t2 = d_cross(T.v[1] - p, T.v[2] - p);
		d_vec3 t3 = d_cross(T.v[2] - p, T.v[0] - p);

		if (d_dot(t1, t2) < 0) return;
		if (d_dot(t1, t3) < 0) return;
		if (d_dot(t2, t3) < 0) return;

		float w0, w1, w2;

		w0 = t2.length() / area;
		w1 = t3.length() / area;
		w2 = 1 - w0 - w1;

		//printf("%d\n", *d_lock);
		while (true) {

			if (atomicExch(d_lock, 0) != 0) {

				//printf("%d in! %d\n", index, *d_lock);
				if (*id == -1) {

					*id = index;
					*resultT = t;
					*weights = d_vec3(w0, w1, w2);

				}
				else if (t < *resultT) {

					*id = index;
					*resultT = t;
					*weights = d_vec3(w0, w1, w2);

				}

				atomicAdd(d_lock, 1);
				//printf("%d out! %d\n", index, *d_lock);
				break;

			}

			//printf("%d wait! %d\n", index, *d_lock);

		}

	}

}

extern int triangleNum;
extern d_triangle* d_triangles;
extern d_triangle* h_triangles;
extern int* d_id;
extern float* d_T;
extern d_vec3* d_weight;
extern int* d_lock;
extern Material* h_mat;
extern int* h_id;
extern float* h_T;
extern d_vec3* h_weight;
extern int* h_lock;

bool CUDAIntersectionCheck(Ray r, float& t, IntersectionPoint& IP) {

	cudaEvent_t start, stop;
	// capture the start time
	HANDLE_ERROR(cudaEventCreate(&start));
	HANDLE_ERROR(cudaEventCreate(&stop));
	HANDLE_ERROR(cudaEventRecord(start, 0));

	*h_lock = 1;
	*h_T = 1e10;
	*h_id = -1;

	if (cudaMemcpy(d_id, h_id, sizeof(int), cudaMemcpyHostToDevice) != cudaSuccess) {

		printf("[Error]Failed to copy id to GPU\n");
		exit(-1);

	}

	if (cudaMemcpy(d_lock, h_lock, sizeof(int), cudaMemcpyHostToDevice) != cudaSuccess) {

		printf("[Error]Failed to copy locker to GPU\n");
		exit(-1);

	}

	if (cudaMemcpy(d_T, h_T, sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess) {

		printf("[Error]Failed to copy t to GPU\n");
		exit(-1);

	}

	int threads = 512;
	int blocks = (triangleNum + threads) / threads;
	d_vec3 origin = d_vec3(r.origin().x, r.origin().y, r.origin().z);
	d_vec3 direction = d_vec3(r.direction().x, r.direction().y, r.direction().z);

	IntersectionCheckKernel <<<blocks, threads >>> (origin, direction, d_triangles, triangleNum, d_id, d_T, d_weight, d_lock);

	if (cudaMemcpy(h_id, d_id, sizeof(int), cudaMemcpyDeviceToHost) != cudaSuccess) {

		printf("[Error]Failed to send flags from GPU!");
		exit(-1);

	}

	if (cudaMemcpy(h_T, d_T, sizeof(float), cudaMemcpyDeviceToHost) != cudaSuccess) {

		printf("[Error]Failed to send Ts from GPU!");
		exit(-1);

	}

	if (cudaMemcpy(h_weight, d_weight, sizeof(d_vec3), cudaMemcpyDeviceToHost) != cudaSuccess) {

		printf("[Error]Failed to send weights from GPU!");
		exit(-1);

	}

	// get stop time, and display the timing results
	HANDLE_ERROR(cudaEventRecord(stop, 0));
	HANDLE_ERROR(cudaEventSynchronize(stop));
	float   elapsedTime;
	HANDLE_ERROR(cudaEventElapsedTime(&elapsedTime,
		start, stop));
	printf("GPU Time to compute:  %3.1f ns\n", elapsedTime * 1e6);

	if (*h_id == -1) return false;

	vec3 weight = vec3(h_weight->x, h_weight->y, h_weight->z);
	d_triangle T = h_triangles[*h_id];
	d_vec3 normal;
	d_vec2 uv;
	IP.mat = &h_mat[*h_id];
	IP.p = r.pointAt(*h_T);
	normal = T.n[0] * weight.x + T.n[1] * weight.y + T.n[2] * weight.z;
	IP.n = vec3(normal.x, normal.y, normal.z);
	uv = T.uv[0] * weight.x + T.uv[1] * weight.y + T.uv[2] * weight.z;
	IP.uv = vec2(uv.x, uv.y);

	return true;

}