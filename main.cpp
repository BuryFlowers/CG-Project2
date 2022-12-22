#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"
#include "tinyxml2.h"
#include "glm/glm.hpp"
#include "camera.h"
#include "mesh.h"
#include <string>

using namespace glm;

std::string dataPath = "data/";
std::string sceneName = "cornell-box";

tinyxml2::XMLDocument xmlDoc;

Camera* LoadCamera();
Camera* cam;

int materialNum = 0;
Material* materialList = NULL;
int triangleNum = 0;
Mesh* triangleMesh = NULL;
void LoadOBJ();

int main() {

	cam = LoadCamera();
	LoadOBJ();

}
