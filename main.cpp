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
#include <iostream>
#include <fstream>

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
int SPP = 2;
std::vector<PathPoint> paths;
float* result;
float maxResult = 0.0f;
const float gamma = 2.2f;
char* image;

bool debugOutput = false;
std::ofstream fout;

bool intersectionCheck(Ray r, float& t, IntersectionPoint& IP);
bool lightIntersectionCheck(Ray r, float& t, IntersectionPoint& IP);
IntersectionType intersectionTypeCheck(Ray r, float& t, IntersectionPoint& IP);
extern bool CUDAIntersectionCheck(Ray r, float& t, IntersectionPoint& IP);
vec3 pathTracing(IntersectionPoint IP, vec3 wo);
vec3 bidirectionalPathTracing(IntersectionPoint IP, vec3 wo);
vec3 BDPTCombine(std::vector<PathPoint>& cameraPath, std::vector<PathPoint>& lightPath, Light* light);

float ACESToneMapping(float color, float adapted_lum);
float ACESFilm(float x);

int main() {

	fout.open("log.txt");
	srand(time(0));

	cam = LoadCamera();
	lights = LoadLight();
	LoadOBJ();

	/*Ray r = cam->pixelRay(294, cam->Height() - 457);
	float t;
	IntersectionPoint IP;
	return CUDAIntersectionCheck(r, t, IP);*/

	vec3 color1, color2;
	Ray r = cam->pixelRay(518, cam->Height() - 722);
	float tmpT;
	IntersectionPoint tmpIP;
	IntersectionType IT = intersectionTypeCheck(r, tmpT, tmpIP);

	for (int i = 0; i < 10240; i++) {

		color1 += pathTracing(tmpIP, -r.direction());
		color2 += bidirectionalPathTracing(tmpIP, -r.direction());

	}

	color1 /= 10240.0f;
	color2 /= 10240.0f;

	std::cout << color1.x << " " << color1.y << " " << color1.z << std::endl;
	std::cout << color2.x << " " << color2.y << " " << color2.z << std::endl;

	return 0;

	result = new float[cam->Width() * cam->Height() * 3];
	image = new char[cam->Width() * cam->Height() * 3];

	double tmp_timing_start = omp_get_wtime();

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

				if (i == 519 && j == cam->Height() - 720) debugOutput = true;
				else debugOutput = false;

				vec3 color = vec3(0);
				float t;
				IntersectionPoint IP;

				IntersectionType IT = intersectionTypeCheck(r, t, IP);

				if (IT == HITOBJECT) color = bidirectionalPathTracing(IP, r.direction() * -1.0f);
				else if (IT == HITLIGHT) color = IP.mat->GetLightRadiance();
				else color = vec3(0);

				result[index] += color.x;
				result[index + 1] += color.y;
				result[index + 2] += color.z;

				if (color != vec3(0)) completePath += 1.0f;

			}

			for (int k = 0; k < 3; k++) {

				result[index + k] /= (1.0f * SPP);
				//result[index + k] /= completePath;
				result[index + k] = clamp(result[index + k], 0.0f, 1.0f);

			}

		}

		printf("%d%% is finished.\r", i * 100 / cam->Width());

		if (i == cam->Width() - 1) printf("\n\n");

	}

	double tmp_timing_finish = omp_get_wtime();
	double  tmp_timing_duration = tmp_timing_finish - tmp_timing_start;
	printf("Total time: %2.5f m           \n\n", tmp_timing_duration / 60);

	for (int i = 0; i < cam->Width(); i++)
		for (int j = 0; j < cam->Height(); j++)
			for (int k = 0; k < 3; k++)
				image[(i + (cam->Height() - j - 1) * cam->Width()) * 3 + k] = pow(result[(j + i * cam->Height()) * 3 + k], 1.0f / gamma) * 255.0f;


	stbi_write_jpg(("test/" + sceneName + "_" + "BDPT_RandomX" + "_" + std::to_string(cam->Width()) + "X" + std::to_string(cam->Height()) + "_" + std::to_string(SPP) + "SPP" + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 100);

	fout.close();

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
	
	if (length(IP.mat->GetLightRadiance()) > 0.0f) return IP.mat->GetLightRadiance();

	vec3 brdf;
	float cos;
	vec3 refractionLight = vec3(0);
	vec3 directLight = vec3(0);
	vec3 reflectionLight = vec3(0);
	float directP = 0.0f;
	float indirectP = 0.0f;
	float tmpT;
	IntersectionPoint tmpIP;
	Ray nextRay;
	vec3 MCresult = vec3(0);
	int paths = 0;

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

			if (IT == HITLIGHT && fabs(lightDistance - tmpT) < 1e-4) {

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
				if (IT == HITOBJECT) {

					vec3 wi = nextRay.direction();
					brdf = IP.mat->phongModelBRDF(wi, wo, IP.n, IP.uv);
					cos = max(dot(IP.n, wi), 0.0f);
					reflectionLight = brdf * cos * pathTracing(tmpIP, nextRay.direction() * -1.0f) / (p * RRThreshold);

				}

			}

		}

	}

	//printf("result: direct(%.2lf, %.2lf, %.2lf) cos(%.2lf) brdf(%.2lf, %.2lf,%.2lf) indirect(%.2lf, %.2lf,%.2lf)\n", directLight.x, directLight.y, directLight.z, lightG, lightbrdf.x, lightbrdf.y, lightbrdf.z, reflectionLight.x, reflectionLight.y, reflectionLight.z);

	return directLight + reflectionLight + refractionLight;

}

//int randomLight() {
//
//	float p = rand() * 1.0f / RAND_MAX * lightA;
//	float a = 0.0f;
//	for (int i = 0; i < lightNum; i++) {
//
//		a += lights[i].area();
//		if (a >= p) return i;
//
//	}
//
//}

float GetPathPossibility(std::vector<PathPoint>& path, int mid, float firstP) {

	if (path.size() == 2) return firstP;

	/*float p = firstP;
	float sumP = 0.0f;
	int count = 0;*/

	////Light Tracing
	/*for (int i = 1; i < path.size() - 1; i++) p *= path[i].ip.mat->getDiffusePossibility(path[i].ip.n, path[i].wo) * RRThreshold;
	return p;
	count++;
	sumP += p;
	p = 1.0f;*/

	//Path Tracing
	/*for (int i = path.size() - 1; i > 1; i--) p *= path[i].ip.mat->getDiffusePossibility(path[i].ip.n, path[i].wi) * RRThreshold;
	return p;
	count++;
	sumP += p;
	p = 1.0f;*/

	//MIS Bidirectional Path Tracing
	/*for (int x = 0; x <= path.size() - 1; x++) {

		for (int i = 1; i < x; i++) p *= path[i].ip.mat->getDiffusePossibility(path[i].ip.n, path[i].wo) * RRThreshold;
		for (int i = path.size() - 1; i > x + 1; i--) p *= path[i].ip.mat->getDiffusePossibility(path[i].ip.n, path[i].wi) * RRThreshold;
		count++;
		sumP += p;
		p = firstP;

	}

	return sumP / (1.0f * count);*/

	//don't use MIS
	float p = firstP;
	int x;
	if (path.size() > 3) x = rand() % (path.size() - 3) + 1;
	else x = 1;
	for (int i = 1; i < mid; i++) p *= path[i].ip.mat->getDiffusePossibility(path[i].ip.n, path[i].wo) * RRThreshold;
	for (int i = path.size() - 1; i > mid + 1; i--) p *= path[i].ip.mat->getDiffusePossibility(path[i].ip.n, path[i].wi) * RRThreshold;

	return p;

}

void DebugLog(std::vector<PathPoint>& cameraPath, std::vector<PathPoint>& lightPath, Light* light) {

	std::ofstream fout;
	fout.open("log.txt");
	

	fout.close();

}

vec3 BDPTCombine(std::vector<PathPoint>& cameraPath, std::vector<PathPoint>& lightPath, Light* light) {

	bool flag = false;
	vec3 MISResult = vec3(0);
	int* paths = new int[cameraPath.size() + lightPath.size()]();
	vec3* pathResult = new vec3[cameraPath.size() + lightPath.size()]();
	float* pathPDF = new float[cameraPath.size() + lightPath.size()]();
	int pathCount = 0;
	float sumP = 0.0f;
	//float p = -1.0f;

	//if (debugOutput) DebugLog(cameraPath, lightPath, light);

	for (int i = cameraPath.size() - 1; i >= 0; i--) {

		for (int j = lightPath.size() - 1; j >= 0; j--) {

			bool visible = false;
			int points = i + j;

			Ray r = Ray(cameraPath[i].ip.p, lightPath[j].ip.p - cameraPath[i].ip.p);
			float tmpT;
			IntersectionPoint tmpIP;
			IntersectionType IT = intersectionTypeCheck(r, tmpT, tmpIP);
			//float len = length(lightPath[j].ip.p - cameraPath[i].ip.p);
			if (IT == HITOBJECT && j != 0 && fabs(tmpT - length(lightPath[j].ip.p - cameraPath[i].ip.p)) < 1e-4 && tmpIP.mat == lightPath[j].ip.mat) visible = true;
			else if (IT == HITLIGHT && j == 0 && fabs(tmpT - length(lightPath[j].ip.p - cameraPath[i].ip.p)) < 1e-4 && tmpIP.mat == lightPath[j].ip.mat) visible = true;

			pathCount++;
			if (paths[points]++ == 0) pathResult[points] = vec3(0);

			if (visible) {

				float tmpT;
				IntersectionPoint tmpIP;
				flag = true;
				std::vector<PathPoint> path;
				path.clear();

				for (int k = 0; k <= j; k++) path.push_back(lightPath[k]);
				for (int k = i; k >= 0; k--) path.push_back(cameraPath[k]);

				path[j].wo = normalize(path[j + 1].ip.p - path[j].ip.p);
				path[j + 1].wi = -path[j].wo;

				float len = pow(length(path[0].ip.p - path[1].ip.p), 2);
				vec3 wi = path[1].wi;
				vec3 wo = path[1].wo;
				vec3 BRDF = path[1].ip.mat->phongModelBRDF(wi, wo, path[1].ip.n, path[1].ip.uv);
				float G = max(dot(path[1].ip.n, wi), 0.0f) * max(dot(path[0].ip.n, -wi), 0.0f) / len;
				float COS = max(dot(path[1].ip.n, wi), 0.0f);
				vec3 currentResult = light->Radiance() * COS * BRDF/* / (path[0].ip.mat->getDiffusePossibility(path[0].ip.n, path[0].wo) * RRThreshold)*/;

				for (int k = 2; k < path.size(); k++) {

					if (length(currentResult) == 0.0f) break;
					vec3 wi = path[k].wi;
					vec3 wo = path[k].wo;
					vec3 BRDF = path[k].ip.mat->phongModelBRDF(wi, wo, path[k].ip.n, path[k].ip.uv);
					float COS = max(dot(path[k].ip.n, wi), 0.0f);
					currentResult = currentResult * COS * BRDF;

				}

				if (length(currentResult) == 0.0f) continue;

				/*std::vector<float> Ps;
				Ps.clear();*/
				int validPathCount = 0;
				float p = GetPathPossibility(path, j, len / (max(dot(path[0].ip.n, -wi), 0.0f) * light->area()));
				
				//if (p == -1.0f) p = GetPathPossibility(path, j);

				//for (int k = 0; k < path.size() - 1; k++) {

				//	p += GetPathPossibility(path, k);
				//	//Ps.push_back(tmp);

				//}

				//vec3 tmpR = currentResult;
				if (p != 0.0f) {

					currentResult /= p /*/ (path.size() - 1)*/;
					//pathCount++;

				}
				else currentResult = vec3(0);

				/*if (currentResult.x > 1.0f && currentResult.y > 1.0f && currentResult.z > 1.0f) {

					bool xx = true;
					p = GetPathPossibility(path, 0);

				}*/

				/*float tmp = GetPathPossibility(path, 0);
				if (tmp != 0.0f) currentResult /= tmp;*/

				//MISResult += currentResult;
				pathResult[points] += currentResult;
				if (debugOutput /*&& (currentResult.x > 10.0f || currentResult.y > 10.0f || currentResult.z > 10.0f)*/){

					fout << "BDPT:\n\n" << std::endl;

					for (int i = 0; i < path.size(); i++) {

						fout << "path[" << i << "]: " << path[i].ip.p.x << " " << path[i].ip.p.y << " " << path[i].ip.p.z << std::endl;

					}

					fout << "result: " << currentResult.x << " " << currentResult.y << " " << currentResult.z << std::endl;
					fout << "pdf: " << p << std::endl;
					//fout.close();

				}

			}

		}

	}

	if (!flag) return vec3(0);
	for (int i = 0; i < cameraPath.size() + lightPath.size() - 1; i++)
		if (paths[i] != 0) {

			MISResult += pathResult[i] / (1.0f * paths[i]);
			//printf("result: %d (%.2lf, %.2lf, %.2lf)\n", i, pathResult[i].x / (1.0f * paths[i]), pathResult[i].y / (1.0f * paths[i]), pathResult[i].z / (1.0f * paths[i]));

		}

	delete[](paths);
	delete[](pathResult);
	delete[](pathPDF);

	return MISResult/*  / (1.0f * pathCount)*/;

}

vec3 bidirectionalPathTracing(IntersectionPoint IP, vec3 wo) {

	float tmpT;
	IntersectionPoint tmpIP = IP;
	IntersectionType IT;
	float RR = 0.0f;
	std::vector<PathPoint> cameraPath;
	std::vector<PathPoint> lightPath;
	vec3 finalResult = vec3(0);

	Ray cameraRay;
	float cameraP;
	cameraPath.clear();
	cameraPath.push_back(PathPoint(IP));
	cameraPath.back().wo = wo;
	RR = rand() * 1.0f / RAND_MAX;
	while (RR < RRThreshold) {

		if (!tmpIP.mat->randomBRDFRay(cameraPath.back().wo, tmpIP, cameraRay, cameraP)) break;
		IT = intersectionTypeCheck(cameraRay, tmpT, tmpIP);
		if (IT == NOHIT || IT == HITLIGHT) break;
		/*else if (IT == HITLIGHT) {

			lightPath.clear();
			lightPath.push_back(PathPoint(tmpIP));
			for (int i = 0; i < lightNum; i++)
				if (light[i].Mat() == tmpIP.mat) return MISCombine(cameraPath, lightPath, &light[i]);

		}*/
		cameraPath.back().wi = cameraRay.direction();
		cameraPath.push_back(PathPoint(tmpIP));
		cameraPath.back().wo = -cameraRay.direction();
		RR = rand() * 1.0f / RAND_MAX;

	}

	for (int i = 0; i < lightNum; i++) {

		Light* light = &lights[i];
		Ray lightRay;
		float lightP;
		lightPath.clear();
		light->uniformSampling(tmpIP);
		lightPath.push_back(PathPoint(tmpIP));
		light->randomLightTracingRay(tmpIP, lightRay, lightP);
		IT = intersectionTypeCheck(lightRay, tmpT, tmpIP);
		RR = rand() * 1.0f / RAND_MAX;
		while (IT == HITOBJECT && RR < RRThreshold) {

			lightPath.back().wo = lightRay.direction();
			lightPath.push_back(tmpIP);
			lightPath.back().wi = -lightRay.direction();
			if (!tmpIP.mat->randomBRDFRay(lightPath.back().wi, tmpIP, lightRay, lightP)) break;
			IT = intersectionTypeCheck(lightRay, tmpT, tmpIP);
			RR = rand() * 1.0f / RAND_MAX;

		}

		finalResult += BDPTCombine(cameraPath, lightPath, light);

	}

	return finalResult;

}