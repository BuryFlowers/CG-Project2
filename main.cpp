#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "tinyxml2.h"
#include "stb_image_write.h"
#include "glm/glm.hpp"
#include "camera.h"
#include "mesh.h"
#include "triangle.h"
#include "light.h"
#include <string>
#include <time.h>

using namespace glm;

std::string dataPath = "data/";
std::string sceneName = "staircase";

int materialNum = 0;
Material* materialList = NULL;
int triangleObjectNum;
TriangleObject* triangleObjects;
void LoadOBJ();

tinyxml2::XMLDocument xmlDoc;
Camera* LoadCamera();
Camera* cam;
Light* LoadLight();
Light* lights;
int lightNum;

const float RRThreshold = 0.8f;
int SPP = 10;
std::vector<PathPoint> paths;
float* result;
float maxResult = 0.0f;
const float gamma = 2.2f;
char* image;
bool intersectionCheck(Ray r, float& t, IntersectionPoint& IP);
bool lightIntersectionCheck(Ray r, float& t, IntersectionPoint& IP);
vec3 pathTracing(IntersectionPoint IP, vec3 wo);

float ACESToneMapping(float color, float adapted_lum);
float ACESFilm(float x);

int main() {

	srand(time(0));

	cam = LoadCamera();
	lights = LoadLight();
	LoadOBJ();

	result = new float[cam->Width() * cam->Height() * 3];
	image = new char[cam->Width() * cam->Height() * 3];

	for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++){

			int index = (i * cam->Height() + j) * 3;
			float completePath = 0.0f;
			result[index] = 0.0f;
			result[index + 1] = 0.0f;
			result[index + 2] = 0.0f;

			for (int k = 0; k < SPP; k++) {

				Ray r = cam->pixelRay(i, j);

				vec3 color = vec3(0);
				float t, lightT;
				IntersectionPoint IP, lightIP;
				if (!intersectionCheck(r, t, IP)) {

					if (lightIntersectionCheck(r, lightT, lightIP)) color = lightIP.mat->GetLightRadiance();
					else color = vec3(0);

				}
				else {

					if (lightIntersectionCheck(r, lightT, lightIP) && lightT - t < 1e-3) color = lightIP.mat->GetLightRadiance();
					else color = pathTracing(IP, r.direction() * -1.0f);

				}

				result[index] += color.x;
				result[index + 1] += color.y;
				result[index + 2] += color.z;

			}

			for (int k = 0; k < 3; k++) {

				result[index + k] /= (1.0f * SPP);
				result[index + k] = clamp(result[index + k], 0.0f, 1.0f);

			}

		}

		if (i % 16 == 0) printf("%d%% is finished.\n", i * 100 / cam->Width());

	}

	for (int i = 0; i < cam->Width(); i++) 
		for (int j = 0; j < cam->Height(); j++)
			for (int k = 0; k < 3; k++) 
				image[(i + (cam->Height() - j - 1) * cam->Width()) * 3 + k] = pow(result[(j + i * cam->Height()) * 3 + k], 1.0f / gamma) * 255.0f;


	stbi_write_jpg((sceneName + "_" + std::to_string(cam->Width()) + "X" + std::to_string(cam->Height()) + "_" + std::to_string(SPP) + "SPP" + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 100);

}

float ACESToneMapping(float color, float adapted_lum){

	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);

}

float ACESFilm(float x) {

	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);

}

bool intersectionCheck(Ray r, float& t, IntersectionPoint& IP) {

	t = 1e10;
	bool flag = false;
	for (int l = 0; l < triangleObjectNum; l++) {

		float tmpT;
		IntersectionPoint tmpIP;
		if (triangleObjects[l].intersect(r, tmpT, tmpIP) && tmpT < t) {

			t = tmpT;
			IP = tmpIP;
			flag = true;

		}

	}

	return flag;

}

bool lightIntersectionCheck(Ray r, float& t, IntersectionPoint& IP) {

	t = 1e10;
	bool flag = false;
	for (int l = 0; l < lightNum; l++) {

		float tmpT;
		IntersectionPoint tmpIP;
		if (lights[l].intersect(r, tmpT, tmpIP) && tmpT < t) {

			t = tmpT;
			IP = tmpIP;
			flag = true;

		}

	}

	return flag;

}



vec3 pathTracing(IntersectionPoint IP, vec3 wo) {

	vec3 brdf;
	float cos;

	vec3 directLight = vec3(0);
	float lightT;
	IntersectionPoint lightIP;
	for (int i = 0; i < lightNum; i++) {
			
			Ray lightRay;
			vec3 lightPosition = lights[i].randomLightRay(IP.p, lightRay);
			float lightDistance = length(IP.p - lightPosition);
			bool hitLight = false;
			if (lightIntersectionCheck(lightRay, lightT, lightIP) && fabs(lightT - lightDistance) < 1e-3) {

				float tmpT;
				IntersectionPoint tmpIP;
				if (!intersectionCheck(lightRay, tmpT, tmpIP)) hitLight = true;
				else if (lightDistance - tmpT < 1e-3) hitLight = true;

			}

			if (hitLight) {

				vec3 wi = lightRay.direction();
				brdf = IP.mat->phongModelBRDF(wi, wo, IP.n, IP.uv);
				cos = max(dot(IP.n, wi), 0.0f);
				directLight += brdf * cos * lights[i].Radiance() * max(dot(lightIP.n, wi * -1.0f), 0.0f) * lights[i].area() / (lightT * lightT);

			}

	}

	float tmpT;
	IntersectionPoint tmpIP;
	vec3 indirectLight = vec3(0);
	float RR = rand() * 1.0f / RAND_MAX;
	if (RR < RRThreshold) {

		Ray nextRay;
		float p = 1.0f;
		if (IP.mat->randomBRDFRay(wo, IP, nextRay, p)) {

			if (!intersectionCheck(nextRay, tmpT, tmpIP)) return directLight;
			vec3 wi = nextRay.direction();
			brdf = IP.mat->phongModelBRDF(wi, wo, IP.n, IP.uv);
			cos = max(dot(IP.n, wi), 0.0f);
			indirectLight = brdf * cos * pathTracing(tmpIP, nextRay.direction() * -1.0f) / (p * RRThreshold);

		}

	}

	return directLight + indirectLight;

}