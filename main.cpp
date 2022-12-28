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

int SPP = 1;

int main() {

	LoadOBJ();
	cam = LoadCamera();
	lights = LoadLight();

	float* result = new float[cam->Width() * cam->Height()];
	char* image = new char[cam->Width() * cam->Height() * 3];
	float maxT = 0;

	for (int i = 0; i < cam->Width(); i++)
		for (int j = 0; j < cam->Height(); j++) {

			if (i == 50 && j == 50) {

				bool xx = true;

			}

			Ray r = cam->pixelRay(i, j);
			float t = 1e10;
			IntersectionPoint IP;
			bool flag = false;
			for (int k = 0; k < SPP; k++) {

				for (int l = 0; l < triangleObjectNum; l++) {

					float tmpT;
					IntersectionPoint tmpIP;
					if (triangleObjects[l].intersect(r, tmpT, tmpIP) && tmpT < t) {

						t = tmpT;
						IP = tmpIP;
						flag = true;

					}


				}

			}

			if (flag == false) t = 0;
			result[(j + i * cam->Height())] = t;
			if (t > maxT) maxT = t;

		}

	for (int i = 0; i < cam->Width(); i++) {

		for (int j = 0; j < cam->Height(); j++) {

			float t = result[j + i * cam->Height()];
			int index = i + (cam->Height() - j - 1) * cam->Width();
			image[index * 3] = t / maxT * 255.0f;
			image[index * 3 + 1] = t / maxT * 255.0f;
			image[index * 3 + 2] = t / maxT * 255.0f;

		}

	}

	stbi_write_jpg((sceneName + ".jpg").c_str(), cam->Width(), cam->Height(), 3, image, 0);

}