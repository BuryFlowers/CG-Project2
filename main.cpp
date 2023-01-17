#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "tinyxml2.h"
#include "stb_image_write.h"
#include "glm/glm.hpp"
#include "camera.h"
#include "mesh.h"
#include "triangle.h"
#include "light.h"
#include "device_triangle.cuh"
#include "omp.h"
#include <string>
#include <time.h>

# define	TIMING_BEGIN \
	{double tmp_timing_start = omp_get_wtime();

# define	TIMING_END(message) \
	{double tmp_timing_finish = omp_get_wtime();\
	double  tmp_timing_duration = tmp_timing_finish - tmp_timing_start;\
	printf("%s: %2.5f ms           \n\n", (message), tmp_timing_duration * 1000);}}

using namespace glm;

std::string dataPath = "data/";
std::string sceneName = "cornell-box";

int materialNum = 0;
Material* materialList = NULL;
int triangleObjectNum;
int triangleNum;
TriangleObject* triangleObjects;
d_triangle* d_triangles;
d_triangle* h_triangles;
int* d_id;
float* d_T;
d_vec3* d_weight;
int* d_lock;
Material* h_mat;
int* h_id;
float* h_T;
d_vec3* h_weight;
int* h_lock;

Light* lights;
int lightNum;
float lightA = 0.0f;
tinyxml2::XMLDocument xmlDoc;
Camera* cam;
Camera* LoadCamera();
Light* LoadLight();
void LoadOBJ();

const float RRThreshold = 0.8f;
int SPP = 32;
std::vector<PathPoint> paths;
float* result;
float maxResult = 0.0f;
const float gamma = 2.2f;
char* image;

bool intersectionCheck(Ray r, float& t, IntersectionPoint& IP);
bool lightIntersectionCheck(Ray r, float& t, IntersectionPoint& IP);
IntersectionType intersectionTypeCheck(Ray r, float& t, IntersectionPoint& IP);
extern bool CUDAIntersectionCheck(Ray r, float& t, IntersectionPoint& IP);
vec3 pathTracing(IntersectionPoint IP, vec3 wo);
vec3 bidirectionalPathTracing(IntersectionPoint IP, vec3 wo);
vec3 MISCombine(std::vector<PathPoint>& cameraPath, vec3 cameraDirection, std::vector<PathPoint>& lightPath, Light* light);

float ACESToneMapping(float color, float adapted_lum);
float ACESFilm(float x);

int main() {

	srand(time(0));

	cam = LoadCamera();
	lights = LoadLight();
	LoadOBJ();

	/*Ray r = cam->pixelRay(cam->Width() / 2, cam->Height() / 2);
	float t;
	IntersectionPoint IP;
	return CUDAIntersectionCheck(r, t, IP);*/

	result = new float[cam->Width() * cam->Height() * 3];
	image = new char[cam->Width() * cam->Height() * 3];

#pragma omp parallel for
	for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++) {

			int index = (i * cam->Height() + j) * 3;
			float completePath = 0.0f;
			result[index] = 0.0f;
			result[index + 1] = 0.0f;
			result[index + 2] = 0.0f;

			for (int k = 0; k < SPP; k++) {

				Ray r = cam->pixelRay(i, j);

				if (i == 257 && j == cam->Height() - 553) {

					bool xx = true;

				}

				vec3 color = vec3(0);
				float t;
				IntersectionPoint IP;

				IntersectionType IT = intersectionTypeCheck(r, t, IP);

				if (IT == HITOBJECT) color = bidirectionalPathTracing(IP, r.direction() * -1.0f);
				else if (IT == HITLIGHT) color = IP.mat->GetLightRadiance();
				else color = vec3(0);

				/*if (!intersectionCheck(r, t, IP)) {

					if (lightIntersectionCheck(r, lightT, lightIP)) color = lightIP.mat->GetLightRadiance();
					else color = vec3(0);

				}
				else {

					if (lightIntersectionCheck(r, lightT, lightIP) && lightT - t < 1e-3) color = lightIP.mat->GetLightRadiance();
					else color = pathTracing(IP, r.direction() * -1.0f);

				}*/

				result[index] += color.x;
				result[index + 1] += color.y;
				result[index + 2] += color.z;

			}

			for (int k = 0; k < 3; k++) {

				result[index + k] /= (1.0f * SPP);
				result[index + k] = clamp(result[index + k], 0.0f, 1.0f);

			}

		}

		printf("%d%% is finished.\r", i * 100 / cam->Width());

	}

	for (int i = 0; i < cam->Width(); i++)
		for (int j = 0; j < cam->Height(); j++)
			for (int k = 0; k < 3; k++)
				image[(i + (cam->Height() - j - 1) * cam->Width()) * 3 + k] = pow(result[(j + i * cam->Height()) * 3 + k], 1.0f / gamma) * 255.0f;


	stbi_write_jpg((sceneName + "_" + "BDPT" + "_" + std::to_string(cam->Width()) + "X" + std::to_string(cam->Height()) + "_" + std::to_string(SPP) + "SPP" + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 100);

}

float ACESToneMapping(float color, float adapted_lum) {

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

	/*double tmp_timing_start = omp_get_wtime();*/
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
	/*double tmp_timing_finish = omp_get_wtime();
	double tmp_timing_duration = tmp_timing_finish - tmp_timing_start;
	printf("CPU Time to compute : %.5f ns\n", tmp_timing_duration * 1e6);*/
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

IntersectionType intersectionTypeCheck(Ray r, float& t, IntersectionPoint& IP) {

	IntersectionPoint lightIP;
	float lightT;

	if (intersectionCheck(r, t, IP)) {

		if (!lightIntersectionCheck(r, lightT, lightIP)) return HITOBJECT;

		else if (lightT - t < 1e-4) {

			t = lightT;
			IP = lightIP;
			return HITLIGHT;

		}
		else return HITOBJECT;

	}

	else if (lightIntersectionCheck(r, lightT, lightIP)) {

		t = lightT;
		IP = lightIP;
		return HITLIGHT;

	}
	else return NOHIT;

}

vec3 pathTracing(IntersectionPoint IP, vec3 wo) {

	vec3 brdf;
	float cos;
	vec3 refractionLight = vec3(0);
	vec3 directLight = vec3(0);
	vec3 reflectionLight = vec3(0);
	float tmpT;
	IntersectionPoint tmpIP;
	Ray nextRay;

	if (IP.mat->isTransparent()) {

		float p = rand() * 1.0f / RAND_MAX;
		float refrectP = IP.mat->refractionRay(wo, IP, nextRay);

		if (p < refrectP) {

			vec3 reflectDirection = normalize(2 * dot(wo, IP.n) * IP.n - wo);
			nextRay = Ray(IP.p, reflectDirection);

			IntersectionType IT = intersectionTypeCheck(nextRay, tmpT, tmpIP);

			if (IT == HITLIGHT) IP.mat->Trans()* tmpIP.mat->GetLightRadiance();
			else if (IT == HITOBJECT) reflectionLight = IP.mat->Trans() * pathTracing(tmpIP, nextRay.direction() * -1.0f);

		}

		else {

			IntersectionType IT = intersectionTypeCheck(nextRay, tmpT, tmpIP);

			if (IT == HITLIGHT) refractionLight = IP.mat->Trans() * tmpIP.mat->GetLightRadiance();
			else if (IT == HITOBJECT) refractionLight = IP.mat->Trans() * pathTracing(tmpIP, nextRay.direction() * -1.0f);

		}

	}

	else {

		for (int i = 0; i < lightNum; i++) {

			Ray lightRay;
			vec3 lightPosition = lights[i].randomLightRay(IP.p, lightRay);
			float lightDistance = length(IP.p - lightPosition);

			IntersectionType IT = intersectionTypeCheck(lightRay, tmpT, tmpIP);

			if (IT == HITLIGHT) {

				vec3 wi = lightRay.direction();
				brdf = IP.mat->phongModelBRDF(wi, wo, IP.n, IP.uv);
				cos = max(dot(IP.n, wi), 0.0f);
				directLight += brdf * cos * lights[i].Radiance() * max(dot(tmpIP.n, wi * -1.0f), 0.0f) * lights[i].area() / (tmpT * tmpT);

			}

		}

		float RR = rand() * 1.0f / RAND_MAX;
		if (RR < RRThreshold) {

			float p = 1.0f;
			if (IP.mat->randomBRDFRay(wo, IP, nextRay, p)) {

				IntersectionType IT = intersectionTypeCheck(nextRay, tmpT, tmpIP);
				if (IT == HITLIGHT || IT == NOHIT) return directLight;
				vec3 wi = nextRay.direction();
				brdf = IP.mat->phongModelBRDF(wi, wo, IP.n, IP.uv);
				cos = max(dot(IP.n, wi), 0.0f);
				reflectionLight = brdf * cos * pathTracing(tmpIP, nextRay.direction() * -1.0f) / (p * RRThreshold);

			}

		}

	}

	return directLight + reflectionLight + refractionLight;

}

int randomLight() {

	float p = rand() * 1.0f / RAND_MAX * lightA;
	float a = 0.0f;
	for (int i = 0; i < lightNum; i++) {

		a += lights[i].area();
		if (a >= p) return i;

	}

}

vec3 MISCombine(std::vector<PathPoint>& cameraPath, vec3 cameraDirection, std::vector<PathPoint>& lightPath, Light* light) {

	float sumP = 0.0f;
	bool flag = false;
	vec3 MISResult = vec3(0);
	int* paths = new int[cameraPath.size() + lightPath.size()]();
	vec3* pathResult = new vec3[cameraPath.size() + lightPath.size()]();
	float* pathPDF = new float[cameraPath.size() + lightPath.size()]();

	for (int i = 0; i < cameraPath.size(); i++) {

		for (int j = 0; j < lightPath.size(); j++) {

			bool visible = false;
			Ray r = Ray(cameraPath[i].ip.p, lightPath[j].ip.p - cameraPath[i].ip.p);
			float tmpT;
			IntersectionPoint tmpIP;
			IntersectionType IT = intersectionTypeCheck(r, tmpT, tmpIP);
			if (IT == HITOBJECT && j != 0 && fabs(tmpT - length(lightPath[j].ip.p - cameraPath[i].ip.p)) < 1e-3 && tmpIP.mat == lightPath[j].ip.mat) visible = true;
			else if (IT == HITLIGHT && j == 0 && fabs(tmpT - length(lightPath[j].ip.p - cameraPath[i].ip.p)) < 1e-3 && tmpIP.mat == lightPath[j].ip.mat) visible = true;

			if (paths[i + j]++ == 0) pathResult[i + j] = vec3(0);

			if (visible) {

				flag = true;
				sumP += cameraPath[i].p * lightPath[j].p;
				vec3 currentResult = lightPath[0].ip.mat->GetLightRadiance();
				int k = 1;

				for (; k <= j; k++) {

					vec3 wi = lightPath[k - 1].ip.p - lightPath[k].ip.p;
					float len = length(wi) * length(wi);
					wi = normalize(wi);
					vec3 wo = k == j ? normalize(cameraPath[i].ip.p - lightPath[j].ip.p) : normalize(lightPath[k + 1].ip.p - lightPath[k].ip.p);
					vec3 BRDF = lightPath[k].ip.mat->phongModelBRDF(wi, wo, lightPath[k].ip.n, lightPath[k].ip.uv);
					float G = max(dot(lightPath[k - 1].ip.n, -wi), 0.0f) * max(dot(lightPath[k].ip.n, wi), 0.0f) / len;
					float COS = max(dot(lightPath[k].ip.n, wi), 0.0f)/* * max(dot(lightPath[k - 1].ip.n, -wi), 0.0f)*/;
					currentResult = currentResult * COS * BRDF;

				}

				k = i;

				if (j == 0) {

					vec3 wi = lightPath[0].ip.p - cameraPath[i].ip.p;
					float len = length(wi) * length(wi);
					wi = normalize(wi);
					vec3 wo = i == 0 ? cameraDirection : normalize(cameraPath[i - 1].ip.p - cameraPath[i].ip.p);
					vec3 BRDF = cameraPath[i].ip.mat->phongModelBRDF(wi, wo, cameraPath[i].ip.n, cameraPath[i].ip.uv);
					float G = max(dot(cameraPath[i].ip.n, wi), 0.0f) * max(dot(lightPath[0].ip.n, -wi), 0.0f) / len;
					currentResult = currentResult * G * BRDF * light->area();
					k--;

				}

				for (; k >= 0; k--) {

					vec3 wi = k == i ? lightPath[j].ip.p - cameraPath[k].ip.p : cameraPath[k + 1].ip.p - cameraPath[k].ip.p;
					float len = length(wi) * length(wi);
					wi = normalize(wi);
					vec3 wo = k == 0 ? cameraDirection : normalize(cameraPath[k - 1].ip.p - cameraPath[k].ip.p);
					vec3 BRDF = cameraPath[k].ip.mat->phongModelBRDF(wi, wo, cameraPath[k].ip.n, cameraPath[k].ip.uv);
					float G = max(dot(k == i ? lightPath[j].ip.n : cameraPath[k + 1].ip.n, -wi), 0.0f) * max(dot(cameraPath[k].ip.n, wi), 0.0f) / len;
					float COS = /*max(dot(k == i ? lightPath[j].ip.n : cameraPath[k + 1].ip.n, -wi), 0.0f) * */max(dot(cameraPath[k].ip.n, wi), 0.0f);
					currentResult = currentResult * COS * BRDF;

				}

				float p = 1.0f;
				for (int k = 0; k < i; k++) {

					p *= cameraPath[k].ip.mat->getDiffusePossibility(cameraPath[k].ip.n, normalize(cameraPath[k + 1].ip.p - cameraPath[k].ip.p), k == 0 ? cameraDirection : normalize(cameraPath[k - 1].ip.p - cameraPath[k].ip.p));

				}

				if (j != 0) {

					p *= cameraPath[i].ip.mat->getDiffusePossibility(cameraPath[i].ip.n, normalize(lightPath[j].ip.p - cameraPath[i].ip.p), i == 0 ? cameraDirection : normalize(cameraPath[i - 1].ip.p - cameraPath[i].ip.p));

					for (int k = j; k >= 2; k--) p *= lightPath[k].ip.mat->getDiffusePossibility(lightPath[k].ip.n, normalize(lightPath[k - 1].ip.p - lightPath[k].ip.p), normalize(k == j ? normalize(cameraPath[i].ip.p - lightPath[k].ip.p) : normalize(lightPath[k + 1].ip.p - lightPath[k].ip.p)));

				}

				if (j == 0) currentResult /= p;
				else currentResult /= p * lightPath[0].p;

				/*if (j != 0) currentResult /= cameraPath[i].p * lightPath[j].p;
				else currentResult /= cameraPath[i].p;*/
				//currentResult /= p;
				pathResult[i + j] += currentResult;
				pathPDF[i + j] += (cameraPath[i].p * lightPath[j].p);

				if (currentResult.x < 1.0f && currentResult.y > 1.0f && currentResult.z < 1.0f) {

					bool xx = true;

				}

			}

		}

	}

	if (!flag) return vec3(0);
	for (int i = 0; i < cameraPath.size() + lightPath.size(); i++)
		if (paths[i] != 0) MISResult += pathResult[i] / (1.0f * paths[i]);

	return MISResult;

}

vec3 bidirectionalPathTracing(IntersectionPoint IP, vec3 wo) {

	float RR = 0.0f;
	std::vector<PathPoint> cameraPath;
	cameraPath.clear();
	cameraPath.push_back(PathPoint(IP, 1.0f));
	Ray r;
	float t;
	float p;
	float tmpT;
	IntersectionPoint tmpIP = IP;
	IntersectionType IT;
	while (RR < RRThreshold) {

		if (!tmpIP.mat->randomBRDFRay(wo, tmpIP, r, p)) break;
		//p *= max(dot(r.direction(), tmpIP.n), 0.0f);
		IT = intersectionTypeCheck(r, tmpT, tmpIP);
		if (IT == HITLIGHT || IT == NOHIT) break;
		//p /= tmpT * tmpT;
		cameraPath.push_back(PathPoint(tmpIP, p * RRThreshold * cameraPath.back().p));
		RR = rand() * 1.0f / RAND_MAX;

	}

	Light* light = &lights[rand() % lightNum];
	Ray lightRay;
	std::vector<PathPoint> lightPath;
	lightPath.clear();

	light->uniformSampling(tmpIP);
	lightPath.push_back(PathPoint(tmpIP, 1.0f / light->area()));
	light->randomLightTracingRay(tmpIP, lightRay, p);
	//p *= max(dot(lightRay.direction(), tmpIP.n), 0.0f);
	lightPath.back().p /= max(dot(lightRay.direction(), tmpIP.n), 0.0f);
	IT = intersectionTypeCheck(lightRay, tmpT, tmpIP);
	//p /= tmpT * tmpT;
	lightPath.back().p *= tmpT * tmpT;
	RR = rand() * 1.0f / RAND_MAX;
	if (IT == HITOBJECT && RR < RRThreshold) {

		lightPath.push_back(PathPoint(tmpIP, p * RRThreshold * lightPath.back().p));
		vec3 wi = lightRay.direction() * -1.0f;
		vec3 wo;
		RR = rand() * 1.0f / RAND_MAX;
		while (RR < RRThreshold) {

			if (!tmpIP.mat->randomBRDFRay(wi, tmpIP, lightRay, p)) break;
			//p *= max(dot(lightRay.direction(), tmpIP.n), 0.0f);
			IT = intersectionTypeCheck(lightRay, tmpT, tmpIP);
			if (IT == HITLIGHT || IT == NOHIT) break;
			//p /= tmpT * tmpT;
			lightPath.push_back(PathPoint(tmpIP, p * RRThreshold * lightPath.back().p));
			RR = rand() * 1.0f / RAND_MAX;

		}

	}

	return MISCombine(cameraPath, wo, lightPath, light);

}