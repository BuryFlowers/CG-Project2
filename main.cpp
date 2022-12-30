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
int SPP = 20;
std::vector<PathPoint> paths;
float* result;
float maxResult = 0.0f;
const float gamma = 0.8f;
char* image;
bool intersctionCheck(Ray r, float& t, IntersectionPoint& IP);
bool getPathTracingResult(int rgb, int x, int y);

int main() {

	srand(time(0));

	LoadOBJ();
	cam = LoadCamera();
	lights = LoadLight();

	result = new float[cam->Width() * cam->Height() * 3];
	image = new char[cam->Width() * cam->Height() * 3];
	float maxT = 0;

	Ray r = Ray(vec3(278.0f, 540.799988f, 219.103943), vec3(0, -1, 0));
	float t = 1e10;
	IntersectionPoint IP;
	bool flag = intersctionCheck(r, t, IP);

	for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++)
			for (int rgb = 0; rgb < 3; rgb++) {

				result[(i * cam->Height() + j) * 3 + rgb] = 0.0f;
				float completePaths = 0;
				for (int k = 0; k < SPP;) {

					Ray r = cam->pixelRay(i, j);
					paths.clear();
					float t = 1e10;
					IntersectionPoint IP;
					if (intersctionCheck(r, t, IP)) {

						float RRP = rand() * 1.0f / RAND_MAX;
						float p = 1.0f;
						int currentPath = 0;
						paths.push_back(PathPoint(IP.p, IP.n, r.direction() * -1.0f, IP.uv, 0, p, IP.mat));
						while (RRP < RRPThreshold && length(IP.mat->GetLightRadiance()) < 1e-2) {

							p = IP.mat->randomBRDFRay(paths[currentPath].wi, r, IP, rgb);
							if (p == -1.0f) break;
							t = 1e10;
							if (!intersctionCheck(r, t, IP)) break;
							RRP = rand() * 1.0f / RAND_MAX;
							paths.push_back(PathPoint(IP.p, IP.n, r.direction() * -1.0f, IP.uv, 0, p, IP.mat));

						}

					}

					else break;

					if (getPathTracingResult(rgb, i, j)) k++;

				}

				if (completePaths != 0) result[(i * cam->Height() + j) * 3 + rgb] /= SPP;
				//result[(i * cam->Height() + j) * 3 + rgb] = clamp(result[(i * cam->Height() + j) * 3 + rgb], 0.0f, 1.0f);
				//if (result[(i * cam->Height() + j) * 3 + rgb] > maxResult) maxResult = result[(i * cam->Height() + j) * 3 + rgb];

			}

		if (i % 16 == 0) printf("%d%% is finished.\n", i * 100 / cam->Width());

	}

	for (int i = 0; i < cam->Width(); i++) {

		for (int j = 0; j < cam->Height(); j++)
			for (int k = 0; k < 3; k++) {

				//printf("%.0lf ", result[(j + i * cam->Height()) * 3 + k]);
				image[(i + (cam->Height() - j - 1) * cam->Width()) * 3 + k] = pow(result[(j + i * cam->Height()) * 3 + k] / maxResult, gamma) * 255.0f;

				//if (k == 2) printf("  ");

			}

		//printf("\n");

	}

	stbi_write_jpg((sceneName + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 100);

}

bool intersctionCheck(Ray r, float& t, IntersectionPoint& IP) {

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

bool getPathTracingResult(int rgb, int x, int y) {

	int last = paths.size() - 1;
	if (last < 0) return false;

	bool flag = false;
	vec3 lightRadiance = vec3(0.0f);
	if (length(paths[last].mat->GetLightRadiance()[rgb]) < 1e-2)
		for (int i = 0; i < lightNum; i++)
			for (int j = 0; j < SPP; j++) {

				Ray r;
				vec3 lightPosition;
				lights[i].randomLightRay(paths[last].x, r, lightPosition);
				float t = 1e10;
				IntersectionPoint IP;
				bool isIntersected = intersctionCheck(r, t, IP);
				float len = length(IP.p - lightPosition);
				if (isIntersected && len < 1e-2) {

					float tmp = paths[last].mat->phongModelBRDF(paths[last].wi, r.direction(), paths[last].n, paths[last].uv)[rgb];
					paths[last].f += max(dot(paths[last].n, r.direction()), 0.0f) * lights[i].Radiance()[rgb] * tmp;
					flag = true;
					break;

				}

			}

	else paths[last].f += paths[last].mat->GetLightRadiance()[rgb];

	for (int i = last - 1; i >= 0; i--) {

		vec3 wo = paths[i + 1].wi * -1.0f;
		wo = normalize(wo);
		paths[i].f += paths[i].mat->GetLightRadiance()[rgb] + max(dot(paths[i].n, wo), 0.0f) * paths[i].mat->phongModelBRDF(paths[i].wi, wo, paths[i].n, paths[i].uv)[rgb] * paths[i + 1].f / paths[i + 1].p / RRPThreshold;

	}

	result[(x * cam->Height() + y) * 3 + rgb] += paths[0].f;

	return flag;

}