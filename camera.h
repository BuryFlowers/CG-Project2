#pragma once
#ifndef CAMERA
#define CAMERA
#define PI 3.1415926535f
#include "glm/glm.hpp"
#include "ray.h"

using namespace glm;

class Camera {

public:

	Camera() {

		eye = vec3(0, 0, 0);
		lookat = vec3(0, -1, 0);
		worldUp = vec3(0, 0, 1);
		fovy = 60.0f;
		width = 1920;
		height = 1080;

		getBasis();

	}

	Camera(vec3 Eye, vec3 Lookat, vec3 Up, float Fovy, int Width, int Height) {

		eye = Eye;
		lookat = Lookat;
		worldUp = Up;
		fovy = Fovy;
		width = Width;
		height = Height;

		getBasis();

	}

	int Width() { return width; }
	int Height() { return height; }

	Ray pixelRay(int x, int y) {

		vec2 offset = vec2(rand() * 1.0f / RAND_MAX - 0.5f + x, rand() * 1.0f / RAND_MAX - 0.5f + y);
		vec3 p = origin - offset.x * 2.0f / width * left + offset.y * 2.0f / height * up;

		return Ray(eye, p - eye);

	}

private:

	vec3 eye;
	vec3 lookat;
	vec3 worldUp;
	vec3 front;
	vec3 left;
	vec3 up;
	vec3 origin;
	float fovy;
	int width;
	int height;

	void getBasis() {

		front = lookat - eye;
		front = normalize(front);
		left = cross(worldUp, front);
		left = normalize(left);
		up = cross(front, left);
		up = normalize(up);

		up *= tanf(fovy * 0.5f * PI / 180.0f);
		left *= length(up) * 1.0f * width / height;

		origin = eye + front - up + left;

	}

};

#endif // !CAMERA
