#include"tiny_obj_loader.h"
#include "tinyxml2.h"
#include "glm/glm.hpp"
#include "camera.h"
#include "mesh.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace glm;

extern tinyxml2::XMLDocument xmlDoc;
extern std::string dataPath;
extern std::string sceneName;

Camera* LoadCamera() {

	std::string xmlPath = dataPath + sceneName + "/" + sceneName + ".xml";
	xmlDoc.LoadFile(xmlPath.c_str());
	tinyxml2::XMLElement* xmlCam = xmlDoc.FirstChildElement();
	while (strcmp(xmlCam->Name(), "camera") != 0 && xmlCam != NULL) xmlCam = xmlCam->NextSiblingElement();

	if (xmlCam == NULL) {

		printf("[Error]No camera attribute was found!\n");
		exit(-1);

	}

	if (xmlCam->Attribute("width") == NULL) {

		printf("[Error]No camera width attribute was found!");
		exit(-1);

	}

	if (xmlCam->Attribute("height") == NULL) {

		printf("[Error]No camera height attribute was found!\n");
		exit(-1);

	}

	if (xmlCam->Attribute("fovy") == NULL) {

		printf("[Error]No camera fovy attribute was found!\n");
		exit(-1);

	}

	int width = atoi(xmlCam->Attribute("width"));
	int height = atoi(xmlCam->Attribute("height"));
	float fovy = strtod(xmlCam->Attribute("fovy"), NULL);
	vec3 eye;
	vec3 lookat;
	vec3 up;
	int count = 0;

	xmlCam = xmlCam->FirstChildElement();
	while (xmlCam != NULL) {

		if (strcmp(xmlCam->Name(), "eye") == 0) {

			eye.x = strtod(xmlCam->Attribute("x"), NULL);
			eye.y = strtod(xmlCam->Attribute("y"), NULL);
			eye.z = strtod(xmlCam->Attribute("z"), NULL);
			count++;

		}
		
		else if (strcmp(xmlCam->Name(), "lookat") == 0) {

			lookat.x = strtod(xmlCam->Attribute("x"), NULL);
			lookat.y = strtod(xmlCam->Attribute("y"), NULL);
			lookat.z = strtod(xmlCam->Attribute("z"), NULL);
			count++;

		}

		else if (strcmp(xmlCam->Name(), "up") == 0) {

			up.x = strtod(xmlCam->Attribute("x"), NULL);
			up.y = strtod(xmlCam->Attribute("y"), NULL);
			up.z = strtod(xmlCam->Attribute("z"), NULL);
			count++;

		}

		xmlCam = xmlCam->NextSiblingElement();

	}

	if (count < 3) {

		printf("[Error]Some camera attribute was not found!\n");
		exit(-1);

	}

	printf("[Success]Camera configuration has been loaded!\n");
	return new Camera(eye, lookat, fovy, width, height);

}

extern int materialNum;
extern Material* materialList;
extern int triangleNum;
extern Mesh* triangleMesh;
void LoadOBJ() {

	std::string objPath = dataPath + sceneName + "/" + sceneName + ".obj";
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.mtl_search_path = dataPath + sceneName + "/";

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(objPath, readerConfig)) {
		if (!reader.Error().empty()) {
			std::cerr << "[Error]TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "[Warning]TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	materialNum = materials.size();
	materialList = new Material[materialNum]();
	for (int i = 0; i < materialNum; i++) {

		const unsigned char* Name = (unsigned char*)materials[i].name.c_str();
		vec3 Diffuse = vec3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		const char* texturePath = (char*)materials[i].diffuse_texname.c_str();
		vec3 Specular = vec3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		vec3 Transmittance = vec3(materials[i].transmittance[0], materials[i].transmittance[1], materials[i].transmittance[2]);
		materialList[i] = Material(Name, Diffuse, texturePath, Specular, Transmittance, materials[i].shininess, materials[i].ior);

	}

	for (int i = 0; i < shapes.size(); i++) triangleNum += shapes[i].mesh.num_face_vertices.size();
	triangleMesh = new Triangle[triangleNum]();
	
	int offset = 0;
	for (int i = 0; i < shapes.size(); i++) {

		for (int j = 0; j < shapes[i].mesh.num_face_vertices.size(); j++) {

			vec3 v[3], n[3];
			vec2 uv[3];
			for (int k = 0; k < 3; k++) {

				tinyobj::index_t index = shapes[i].mesh.indices[j * 3 + k];

				v[k].x = attrib.vertices[3 * size_t(index.vertex_index) + 0];
				v[k].y = attrib.vertices[3 * size_t(index.vertex_index) + 1];
				v[k].z = attrib.vertices[3 * size_t(index.vertex_index) + 2];

				n[k].x = attrib.normals[3 * size_t(index.normal_index) + 0];
				n[k].y = attrib.normals[3 * size_t(index.normal_index) + 1];
				n[k].z = attrib.normals[3 * size_t(index.normal_index) + 2];

				uv[k].x = attrib.normals[3 * size_t(index.texcoord_index) + 0];
				uv[k].y = attrib.normals[3 * size_t(index.texcoord_index) + 1];

			}

			triangleMesh[offset++] = Triangle(v, n, uv, &materialList[shapes[i].mesh.material_ids[j]]);

		}

	}

	printf("[Success]Scene has been loaded!\n");

}