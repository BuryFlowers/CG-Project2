#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"
#include "tinyxml2.h"
#include "glm/glm.hpp"
#include "camera.h"
#include <string>

using namespace glm;

std::string dataPath = "data/";
std::string sceneName = "cornell-box";

tinyxml2::XMLDocument xmlDoc;

Camera* LoadCamera();
Camera* cam;

void LoadOBJ();

int main() {

	cam = LoadCamera();
	LoadOBJ();

}
