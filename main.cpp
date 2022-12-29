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

const float RLPThreshold = 0.5f;
int SPP = 1;
std::vector<PathPoint> paths;
bool intersctionCheck(Ray r, float& t, IntersectionPoint& IP);

int main() {

	srand(time(0));

	LoadOBJ();
	cam = LoadCamera();
	lights = LoadLight();

	float* result = new float[cam->Width() * cam->Height()];
	char* image = new char[cam->Width() * cam->Height() * 3];
	float maxT = 0;

	for (int i = 0; i < cam->Width(); i++)
		for (int j = 0; j < cam->Height(); j++)
			for (int rgb = 0; rgb < 3; rgb++) {

				Ray r = cam->pixelRay(i, j);
				paths.clear();
				float t = 1e10;
				IntersectionPoint IP;
				for (int k = 0; k < SPP; k++) 
					if (intersctionCheck(r, t, IP)) {

						int currentPP = 0;
						float RLP = rand() * 1.0f / RAND_MAX;
						vec3 wi = r.direction() * -1.0f;
						float p = 1.0f;
						while (RLP < RLPThreshold) {

							paths.push_back(PathPoint(IP.p, IP.n, 0, p));
							p = IP.mat->randomBRDFRay(wi, r, IP, rgb);
							if (p == -1.0f) break;
							t = 1e10;
							if (!intersctionCheck(r, t, IP)) break;
							RLP = rand() * 1.0f / RAND_MAX;

						}

						
					}


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

vec3 getPathTracingResult(int rgb, int x, int y) {

	vec3 result = vec3(1.0f);

	int num = paths.size();
	vec3 currentResult = vec3(1.0f);

	bool flag = false;
	vec3 d;
	vec3 lightRadiance = vec3(0.0f);
	for (int i = 0; i < lightNum; i++) {

		lights[i].randomLightRay(paths[num - 1].x, d);
		Ray r = Ray(paths[num - 1].x, -d);
		float t = 1e10;
		IntersectionPoint IP;
		if (intersctionCheck(r, t, IP)) paths[num - 1].f += dot(paths[num - 1].n, r.direction()) * lights[i].Radiance()[rgb];


	}

	for (int i = num - 2; i > 0; i--) {

		vec3 direction = paths[i + 1].x - paths[i].x;
		paths[i].f += dot(paths[i].n, direction) * paths[i + 1].f / paths[i + 1].p;

	}

	result[x][y][rgb] += paths[0].f;

}