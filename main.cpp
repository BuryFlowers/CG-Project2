#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"
#include "tinyxml2.h"
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

int main() {

	LoadOBJ();
	cam = LoadCamera();
	lights = LoadLight();

}
