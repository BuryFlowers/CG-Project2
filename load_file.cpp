#include"tiny_obj_loader.h"
#include "tinyxml2.h"
#include "glm/glm.hpp"
#include "camera.h"
#include "mesh.h"
#include "light.h"
#include "triangle.h"
#include "device_vector.cuh"
#include "device_triangle.cuh"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace glm;

extern std::string dataPath;
extern std::string sceneName;

extern int materialNum;
extern Material* materialList;
extern int triangleObjectNum;
extern int triangleNum;
extern TriangleObject* triangleObjects;

extern tinyxml2::XMLDocument xmlDoc;
extern float maxResult;

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
	return new Camera(eye, lookat, up, fovy, width, height);

}

extern int lightNum;
extern Light* lights;
extern float lightA;
Light* LoadLight() {

	std::string xmlPath = dataPath + sceneName + "/" + sceneName + ".xml";
	xmlDoc.LoadFile(xmlPath.c_str());

	lightNum = 0;
	tinyxml2::XMLElement* xmlLg = xmlDoc.FirstChildElement();
	while (xmlLg != NULL) {

		if (strcmp(xmlLg->Name(), "light") == 0) lightNum++;
		xmlLg = xmlLg->NextSiblingElement();

	}
	Light* lights= new Light[lightNum]();

	lightNum = 0;
	xmlLg = xmlDoc.FirstChildElement();
	while (xmlLg != NULL) {

		if (strcmp(xmlLg->Name(), "light") == 0) {

			std::string radianceStr = xmlLg->Attribute("radiance");
			if (radianceStr.empty()) {

				printf("[Error]Some light ridiance was not found!\n");
				exit(-1);

			}

			int l = 0, r = radianceStr.length() - 1;

			while (radianceStr[l] != ',' && l < r) l++;
			while (radianceStr[r] != ',' && r > 0) r--;

			vec3 radiance;
			radiance.x = strtod(radianceStr.substr(0, l).c_str(), NULL);
			radiance.y = strtod(radianceStr.substr(l + 1, r - l - 1).c_str(), NULL);
			radiance.z = strtod(radianceStr.substr(r + 1, radianceStr.length() - r - 1).c_str(), NULL);
			if (radiance.x > maxResult) maxResult = radiance.x;
			if (radiance.y > maxResult) maxResult = radiance.y;
			if (radiance.z > maxResult) maxResult = radiance.z;

			std::string materialName = xmlLg->Attribute("mtlname");
			if (materialName.empty()) {

				printf("[Error]Some light material was not found!\n");
				exit(-1);

			}

			lights[lightNum] = Light(radiance, materialName);

			lightNum++;

		}

		xmlLg = xmlLg->NextSiblingElement();

	}

	printf("[Success]Light configuration has been loaded!\n");
	return lights;

}

extern d_triangle* d_triangles;
extern d_triangle* h_triangles;
extern int* d_id;
extern float* d_T;
extern d_vec3* d_weight;
extern int* d_lock;
extern Material* h_mat;
extern int* h_id;
extern float* h_T;
extern d_vec3* h_weight;
extern int* h_lock;
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

		const char* Name = materials[i].name.c_str();
		vec3 Diffuse = vec3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		std::string texturePath;
		if (!materials[i].diffuse_texname.empty()) texturePath = dataPath + sceneName + "/" + materials[i].diffuse_texname;
		/*Texture* Tex = NULL;
		if (!materials[i].diffuse_texname.empty()) {

			Tex = new Texture();
			Tex->image = stbi_load((dataPath + sceneName + "/" + materials[i].diffuse_texname).c_str(), &Tex->width, &Tex->height, &Tex->channel, 0);

		}*/
		vec3 Specular = vec3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		vec3 Transmittance = vec3(materials[i].transmittance[0], materials[i].transmittance[1], materials[i].transmittance[2]);
		materialList[i] = Material(Name, Diffuse, texturePath, Specular, Transmittance, materials[i].shininess, materials[i].ior);

		for (int j = 0; j < lightNum; j++)
			if (strcmp(lights[j].Name().c_str(), materials[i].name.c_str()) == 0) lights[j].SetMat(&materialList[i]);

	}

	
	triangleNum = 0;
	triangleObjectNum = shapes.size();
	triangleObjects = new TriangleObject[triangleObjectNum]();

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

				uv[k].x = attrib.texcoords[2 * size_t(index.texcoord_index) + 0];
				uv[k].y = attrib.texcoords[2 * size_t(index.texcoord_index) + 1];

			}
			
			Triangle* tmpTriangle = new Triangle(v, n, uv, &materialList[shapes[i].mesh.material_ids[j]]);
			bool isLight = false;

			for (int k = 0; k < lightNum; k++) 
				if (lights[k].Mat() == &materialList[shapes[i].mesh.material_ids[j]]) {

					materialList[shapes[i].mesh.material_ids[j]].SetLightRadiance(lights[k].Radiance());
					lights[k].addMesh(tmpTriangle);
					isLight = true;
					break;

				}

			if (!isLight) triangleObjects[i].addTriangle(tmpTriangle), triangleNum++;

		}

		triangleObjects[i].buildBVH();

	}

	h_triangles = new d_triangle[triangleNum]();
	h_id = new int;
	*h_id = -1;
	h_T = new float();
	*h_T = 1e10;
	h_weight = new d_vec3();
	h_mat = new Material[triangleNum]();
	h_lock = new int;
	*h_lock = 1;

	if (cudaMalloc((void**)&d_lock, sizeof(int)) != cudaSuccess) {

		printf("[Error]Failed to malloc GPU memory for locker\n");
		exit(-1);

	}

	if (cudaMalloc((void**)&d_triangles, triangleNum * sizeof(d_triangle)) != cudaSuccess) {

		printf("[Error]Failed to malloc GPU memory for triangles\n");
		exit(-1);

	}

	if (cudaMalloc((void**)&d_id, sizeof(int)) != cudaSuccess) {

		printf("[Error]Failed to malloc GPU memory for flags\n");
		exit(-1);

	}

	if (cudaMalloc((void**)&d_T, sizeof(float)) != cudaSuccess) {

		printf("[Error]Failed to malloc GPU memory for Ts\n");
		exit(-1);

	}

	if (cudaMalloc((void**)&d_weight, sizeof(d_vec3)) != cudaSuccess) {

		printf("[Error]Failed to malloc GPU memory for weights\n");
		exit(-1);

	}

	triangleNum = 0;
	offset = 0;
	for (int i = 0; i < shapes.size(); i++) {

		for (int j = 0; j < shapes[i].mesh.num_face_vertices.size(); j++) {

			bool isLight = false;

			for (int k = 0; k < lightNum; k++)
				if (lights[k].Mat() == &materialList[shapes[i].mesh.material_ids[j]]) {

					isLight = true;
					break;

				}

			if (isLight) continue;

			for (int k = 0; k < 3; k++) {

				tinyobj::index_t index = shapes[i].mesh.indices[j * 3 + k];

				h_triangles[triangleNum].v[k] = d_vec3(attrib.vertices[3 * size_t(index.vertex_index) + 0], attrib.vertices[3 * size_t(index.vertex_index) + 1], attrib.vertices[3 * size_t(index.vertex_index) + 2]);
				h_triangles[triangleNum].n[k] = d_vec3(attrib.normals[3 * size_t(index.normal_index) + 0], attrib.normals[3 * size_t(index.normal_index) + 1], attrib.normals[3 * size_t(index.normal_index) + 2]);
				h_triangles[triangleNum].uv[k] = d_vec2(attrib.texcoords[2 * size_t(index.texcoord_index) + 0], attrib.texcoords[2 * size_t(index.texcoord_index) + 1]);


			}

			h_mat[triangleNum] = materialList[shapes[i].mesh.material_ids[j]];
			triangleNum++;

		}

	}

	if (cudaMemcpy(d_triangles, h_triangles, triangleNum * sizeof(d_triangle), cudaMemcpyHostToDevice) != cudaSuccess) {

		printf("[Error]Failed to copy triangles to GPU\n");
		exit(-1);

	}

	for (int i = 0; i < lightNum; i++) lights[i].buildBVH();
	for (int i = 0; i < lightNum; i++) lightA += lights[i].area();

	printf("[Success]Scene has been loaded!\n");

}