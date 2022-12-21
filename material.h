#pragma once
#ifndef MATERIAL
#define MATERIAL
#include "glm/glm.hpp"
#include "stb_image.h"
#include <string.h>

using namespace glm;

class Material {

public:

    Material() {

        diffuse = vec3(0);
        texture = NULL;
        specular = vec3(0);
        transmittance = vec3(0);
        shiness = 1.0f;
        IOR = 1.0f;

    }

    Material(vec3 Diffuse, const char texturePath[], vec3 Specular, vec3 Transmittance, float Shiness, float Ior) {

        diffuse = Diffuse;
        if (strlen(texturePath) != 0) {

            texture = new Texture();
            texture->image = stbi_load(texturePath, &texture->width, &texture->height, &texture->channel, 0);

        }
        else texture = NULL;
        specular = Specular;
        transmittance = Transmittance;
        shiness = Shiness;
        IOR = Ior;

    }

    ~Material() {

        if (texture != NULL) {

            stbi_image_free(texture->image);
            delete(texture);

        }

    }

    vec3 sampleTexture(int x, int y) {

        if (texture == NULL) return vec3(1.0f);
        if (x < 0) x %= texture->width, x += texture->width;
        if (y < 0) y %= texture->height, y += texture->height;
        if (x > texture->width) x %= texture->width;
        if (y > texture->height) y %= texture->height;
        int index = (x + y * texture->width) * texture->channel;
        return vec3(texture->image[index] * 1.0f / 255.0f, texture->image[index + 1] * 1.0f / 255.0f, texture->image[index + 2] * 1.0f / 255.0f);

    }

private:

    vec3 diffuse;//-*Kd * : the diffuse reflectance of material, * map_Kd* is the texture file path.
    struct Texture {

        unsigned char* image;
        int width;
        int height;
        int channel;

    }*texture;
    vec3 specular;    //- *Ks * : the specular reflectance of material.
    vec3 transmittance;    //- *Tr * : the transmittance of material.
    float shiness;    //- *Ns * : shiness, the exponent of phong lobe.
    float IOR;    //- *Ni * : the * Index of Refraction(IOR) * of transparent object like glass and water.

};

#endif // !MATERIAL
