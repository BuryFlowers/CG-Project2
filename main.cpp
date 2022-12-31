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
std::string sceneName = "cornell-box";

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

const float RRPThreshold = 0.8f;
int SPP = 100;
std::vector<PathPoint> paths;
float* result;
float maxResult = 0.0f;
const float gamma = 0.5f;
char* image;
bool intersctionCheck(Ray r, float& t, IntersectionPoint& IP, int& hitLight);
bool getPathTracingResult(int x, int y, int lastLight);

float ACESToneMapping(float color, float adapted_lum);
float ACESFilm(float x);

int main() {

	srand(time(0));

	LoadOBJ();
	cam = LoadCamera();
	lights = LoadLight();

	result = new float[cam->Width() * cam->Height() * 3];
	image = new char[cam->Width() * cam->Height() * 3];
	float maxT = 0;

	for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++){

			if (i == 53 && j == 109) {

				bool xx = true;

			}
			int index = (i * cam->Height() + j) * 3;
			float completePath = 0.0f;
			result[index] = 0.0f;
			result[index + 1] = 0.0f;
			result[index + 2] = 0.0f;

			for (int k = 0; k < SPP; k++) {

				Ray r = cam->pixelRay(i, j);
				paths.clear();
				float t;
				IntersectionPoint IP;
				int hitLight = -1;
				if (intersctionCheck(r, t, IP, hitLight)) {

					float RRP = rand() * 1.0f / RAND_MAX;
					float p = 1.0f;
					int currentPath = 0;
					paths.push_back(PathPoint(IP, r.direction() * -1.0f, t, vec3(0), p));
						
					while (RRP < RRPThreshold && hitLight == -1) {

						if (!IP.mat->randomBRDFRay(paths[currentPath].wi, IP, r, p)) break;
						if (!intersctionCheck(r, t, IP, hitLight)) break;
						RRP = rand() * 1.0f / RAND_MAX;
						paths.push_back(PathPoint(IP, r.direction() * -1.0f, t, vec3(0), p / RRPThreshold));

					}

				}

				else break;

				if (getPathTracingResult(i, j, hitLight)) completePath++;

			}

			if (completePath != 0.0f) {

				result[index] /= completePath;
				result[index + 1] /= completePath;
				result[index + 2] /= completePath;

			}
			/*result[index] /= (1.0f * SPP);
			result[index + 1] /= (1.0f * SPP);
			result[index + 2] /= (1.0f * SPP);*/
			result[index] = clamp(result[index], 0.0f, 1.0f);
			result[index + 1] = clamp(result[index + 1], 0.0f, 1.0f);
			result[index + 2] = clamp(result[index + 2], 0.0f, 1.0f);
			//if (result[(i * cam->Height() + j) * 3 + rgb] > maxResult) maxResult = result[(i * cam->Height() + j) * 3 + rgb];

		}

		if (i % 16 == 0) printf("%d%% is finished.\n", i * 100 / cam->Width());

	}

	/*for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++)
			printf("%.0lf %.0lf %.0lf   ", result[(j + i * cam->Height()) * 3], result[(j + i * cam->Height()) * 3 + 1], result[(j + i * cam->Height()) * 3 + 2]);
		printf("%\n");
	}*/

	//float adaptedLum = 0.0f;
	//for (int i = 0; i < cam->Width(); i++)
	//	for (int j = 0; j < cam->Height(); j++) adaptedLum += result[(j + i * cam->Height()) * 3] * 0.299f + result[(j + i * cam->Height()) * 3 + 1] * 0.587f + result[(j + i * cam->Height()) * 3 + 2] * 0.114f;
	//adaptedLum /= 1.0f * cam->Width() * cam->Height();

	//for (int i = 0; i < cam->Width(); i++)
	//	for (int j = 0; j < cam->Height(); j++)
	//		for (int k = 0; k < 3; k++) result[(j + i * cam->Height()) * 3 + k] = ACESToneMapping(result[(j + i * cam->Height()) * 3 + k], adaptedLum);
	
	for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++)
			printf("%.2lf %.2lf %.2lf   ", result[(j + i * cam->Height()) * 3], result[(j + i * cam->Height()) * 3 + 1], result[(j + i * cam->Height()) * 3 + 2]);
		printf("%\n");
	}

	for (int i = 0; i < cam->Width(); i++) 
		for (int j = 0; j < cam->Height(); j++)
			for (int k = 0; k < 3; k++) 
				image[(i + (cam->Height() - j - 1) * cam->Width()) * 3 + k] = pow(result[(j + i * cam->Height()) * 3 + k], gamma) * 255.0f;


	stbi_write_jpg((sceneName + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 100);

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

bool intersctionCheck(Ray r, float& t, IntersectionPoint& IP, int& hitLight) {

	t = 1e10;
	bool flag = false;
	hitLight = -1;
	for (int l = 0; l < triangleObjectNum; l++) {

		float tmpT;
		IntersectionPoint tmpIP;
		if (triangleObjects[l].intersect(r, tmpT, tmpIP) && tmpT < t) {

			t = tmpT;
			IP = tmpIP;
			flag = true;

		}

	}

	if (!flag) return false;

	float lightT = 1e10;
	IntersectionPoint lightIP;
	for (int l = 0; l < lightNum; l++) if (lights[l].intersect(r, lightT, lightIP) && fabs(lightT - t) < 1e-5) {

		hitLight = l;
		break;

	}

	return true;

}

bool getPathTracingResult(int x, int y, int lastIsLight) {

	int last = paths.size() - 1;
	if (last < 0) return false;

	if (last == 0 && lastIsLight != -1) {

		bool xx = true;

	}

	bool flag = false;
	if (lastIsLight != -1) paths[last].f = paths[last].ip.mat->GetLightRadiance() * max(dot(paths[last].ip.n, paths[last].wi), 0.0f) * lights[lastIsLight].area() / (paths[last].t * paths[last].t);
	else for (int i = 0; i < lightNum; i++) {

			Ray r;
			IntersectionPoint lightIP;
			float p = lights[i].randomLightRay(paths[last].ip.p, r, lightIP);
			float t;
			IntersectionPoint IP;
			int hitLight;
			intersctionCheck(r, t, IP, hitLight);
			//float len = length(IP.p - lightIP.p);
			if (hitLight == i) {

				vec3 L = lights[i].Radiance() * max(dot(lightIP.n, r.direction() * -1.0f), 0.0f) / (t * t * p);
				vec3 brdf = paths[last].ip.mat->phongModelBRDF(paths[last].wi, r.direction(), paths[last].ip.n, paths[last].ip.uv);
				float cos = max(dot(paths[last].ip.n, r.direction()), 0.0f);
				paths[last].f +=  L * brdf * cos;
				flag = true;

			}

	}

	for (int i = last - 1; i >= 0; i--) {

		vec3 wo = paths[i + 1].wi * -1.0f;
		wo = normalize(wo);
		vec3 L = paths[i + 1].f / paths[i + 1].p;
		vec3 brdf = paths[i].ip.mat->phongModelBRDF(paths[i].wi, wo, paths[i].ip.n, paths[i].ip.uv);
		float cos = max(dot(paths[i].ip.n, wo), 0.0f);
		paths[i].f += paths[i].ip.mat->GetLightRadiance() * max(dot(paths[i].ip.n, paths[i].wi), 0.0f) / (paths[i].t * paths[i].t) +  L * brdf * cos;

	}

	int index = (x * cam->Height() + y) * 3;
	result[index] += paths[0].f.x;
	result[index + 1] += paths[0].f.y;
	result[index + 2] += paths[0].f.z;

	if (paths[0].f.x >= 1.0f && paths[0].f.y >= 1.0f && paths[0].f.z >= 1.0f) {

		bool xx = true;

	}

	return flag;

}