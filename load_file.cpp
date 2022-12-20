#include"tiny_obj_loader.h"
#include "tinyxml2.h"
#include "glm/glm.hpp"
#include "camera.h"
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

		printf("[Error]No camera attribute was found!");
		exit(-1);

	}

	if (xmlCam->Attribute("width") == NULL) {

		printf("[Error]No camera width attribute was found!");
		exit(-1);

	}

	if (xmlCam->Attribute("height") == NULL) {

		printf("[Error]No camera height attribute was found!");
		exit(-1);

	}

	if (xmlCam->Attribute("fovy") == NULL) {

		printf("[Error]No camera fovy attribute was found!");
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

		printf("[Error]Some camera attribute was not found!");
		exit(-1);

	}

	return new Camera(eye, lookat, fovy, width, height);

}

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

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}

				// Optional: vertex colors
				// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}


}