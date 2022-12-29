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

const float RRPThreshold = 0.5f;
int SPP = 4;
std::vector<PathPoint> paths;
float* result;
char* image;
bool intersctionCheck(Ray r, float& t, IntersectionPoint& IP);
void getPathTracingResult(int rgb, int x, int y);

int main() {

	srand(time(0));

	LoadOBJ();
	cam = LoadCamera();
	lights = LoadLight();

	result = new float[cam->Width() * cam->Height() * 3];
	image = new char[cam->Width() * cam->Height() * 3];
	float maxT = 0;

	for (int i = 0; i < cam->Width(); i++) {
		for (int j = 0; j < cam->Height(); j++)
			for (int rgb = 0; rgb < 3; rgb++) {

				for (int k = 0; k < SPP; k++) {

					Ray r = cam->pixelRay(i, j);
					paths.clear();
					float t = 1e10;
					IntersectionPoint IP;
					if (intersctionCheck(r, t, IP)) {

						float RRP = rand() * 1.0f / RAND_MAX;
						vec3 wi = r.direction() * -1.0f;
						float p = 1.0f;
						paths.push_back(PathPoint(IP.p, IP.n, 0, p));
						while (RRP < RRPThreshold) {

							p = IP.mat->randomBRDFRay(wi, r, IP, rgb);
							if (p == -1.0f) break;
							t = 1e10;
							if (!intersctionCheck(r, t, IP)) break;
							RRP = rand() * 1.0f / RAND_MAX;
							paths.push_back(PathPoint(IP.p, IP.n, 0, p));

						}


					}

					getPathTracingResult(rgb, i, j);

				}

				result[(i * cam->Height() + j) * 3 + rgb] /= 1.0f * SPP;

			}

		if (i % 64 == 0) printf("%d%% is finished.\n", i * 100 / cam->Width());

	}

	for (int i = 0; i < cam->Width(); i++) 
		for (int j = 0; j < cam->Height(); j++)
			for (int k = 0; k < 3; k ++) {

				image[(i + (cam->Height() - j - 1) * cam->Width()) * 3 + k] = result[(j + i * cam->Height()) * 3 + k] * 255.0f;

			}

	stbi_write_jpg((sceneName + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 0);

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

void getPathTracingResult(int rgb, int x, int y) {

	int num = paths.size();
	if (num == 0) return;
	bool flag = false;
	vec3 lightRadiance = vec3(0.0f);
	for (int i = 0; i < lightNum; i++) {

		Ray r1, r2;
		lights[i].randomLightRay(paths[num - 1].x, r1);
		r2 = Ray(paths[num - 1].x, r1.direction() * -1.0f);
		float t = 1e10;
		IntersectionPoint IP;
		if (intersctionCheck(r2, t, IP) && length(IP.p - r1.origin()) < 1e-3) {

			paths[num - 1].f += dot(paths[num - 1].n, r2.direction()) * lights[i].Radiance()[rgb];

		}

	}

	for (int i = num - 2; i > 0; i--) {

		vec3 direction = paths[i + 1].x - paths[i].x;
		direction = normalize(direction);
		paths[i].f += dot(paths[i].n, direction) * paths[i + 1].f / paths[i + 1].p / RRPThreshold;

	}

	result[(x * cam->Height() + y) * 3 + rgb] += paths[0].f;

}