#pragma once
#ifndef MATERIAL
#define MATERIAL
#include "glm/glm.hpp"
#include "stb_image.h"
#include "tracing.h"
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
        radiance = vec3(0);

    }

    Material(const char* Name, vec3 Diffuse, const char* texturePath, vec3 Specular, vec3 Transmittance, float Shiness, float Ior) {

        name = Name;
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
        radiance = vec3(0);

    }

    ~Material() {

        if (texture != NULL) {

            stbi_image_free(texture->image);
            delete(texture);

        }

    }

    /*vec3 phongModel(vec3 wi, vec3 wo, vec3 cameraDirection, vec3 normal, vec2 uv, vec3 lightRadiance) {

        vec3 result = diffuse * sampleTexture((int)uv.x, (int)uv.y) * dot(wo, normal) + specular * pow(dot(wi, cameraDirection), shiness);

        return result * lightRadiance;

    }*/

    void SetLightRadiance(vec3 R) { radiance = R; }
    vec3 GetLightRadiance() { return radiance; }

    vec3 phongModelBRDF(vec3 wi, vec3 wo, vec3 normal, vec2 uv) {

        vec3 reflectDirection = 2 * dot(wi, normal) * normal - wi;
        vec3 h = wi + reflectDirection;
        h = normalize(h);
        vec3 result = diffuse * sampleTexture((int)uv.x, (int)uv.y) * (1.0f / PI) + specular * (shiness + 2) / (2.0f * PI) * pow(max(dot(reflectDirection, wo), 0.0f), shiness);

        return result;

    }

    float randomBRDFRay(vec3 wi, Ray& r, IntersectionPoint IP, int rgb) {

        float u1 = rand() * 1.0f / RAND_MAX;
        float u2 = rand() * 1.0f / RAND_MAX;

        vec3 y = IP.n;
        vec3 z = cross(y, wi);
        z = normalize(z);
        vec3 x = cross(y, z);
        x = normalize(x);

        float p = rand() * 1.0f / RAND_MAX;
        vec3 kd = diffuse * sampleTexture((int)IP.uv.x, (int)IP.uv.y);
        vec3 ks = specular;
        if (p < kd[rgb]) {

            vec2 w = vec2(acosf(sqrt(u1)), 2 * PI * u2);
            vec3 direction;
            direction.x = cosf(w.y) * sinf(w.x);
            direction.y = cosf(w.x);
            direction.z = sinf(w.y) * sinf(w.x);
            direction = normalize(direction);
            //vec3 wo = x * cosf(w.y) * sinf(w.x) + y * sinf(w.y) * sinf(w.x) + z * cosf(w.x);
            vec3 wo = (x + y + z) * direction;
            wo = normalize(wo);
            r = Ray(IP.p, wo);
            return cosf(w.x) / PI;

        }

        else if (p < kd[rgb] + ks[rgb]) {

            vec3 reflectDirection = 2 * dot(wi, y) * y - wi;
            vec2 w = vec2(acosf(pow(u1, 1.0f / (shiness + 1.0f))), 2 * PI * u2);
            vec3 direction;
            direction.x = cosf(w.y) * sinf(w.x);
            direction.y = cosf(w.x);
            direction.z = sinf(w.y) * sinf(w.x);
            direction = normalize(direction);
            //vec3 wo = x * cosf(w.y) * sinf(w.x) + y * sinf(w.y) * sinf(w.x) + z * cosf(w.x);
            vec3 wo = (x + y + z) * direction;
            wo = normalize(wo);
            r = Ray(IP.p, wo);
            return pow(max(dot(reflectDirection, wo), 0.0f), shiness) * (shiness + 1) / (2.0f * PI);

        }

        else return -1.0f;

    }

    const char* Name() { return name.c_str(); }

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

    std::string name;
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
    vec3 radiance;

};

#endif // !MATERIAL
