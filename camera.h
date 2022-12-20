#pragma once
#ifndef CAMERA
#define CAMERA
#include "glm/glm.hpp"

using namespace glm;

class Camera {

public:

	Camera() {

		eye = vec3(0, 0, 0);
		lookat = vec3(0, -1, 0);
		up = vec3(0, 0, 1);
		fovy = 60.0f;
		width = 1920;
		height = 1080;

	}

	Camera(vec3 Eye, vec3 Lookat, float Fovy, int Width, int Height) {

		eye = Eye;
		lookat = Lookat;
		fovy = Fovy;
		width = Width;
		height = Height;

	}

private:

	vec3 eye;
	vec3 lookat;
	vec3 up;
	float fovy;
	int width;
	int height;

};

#endif // !CAMERA
