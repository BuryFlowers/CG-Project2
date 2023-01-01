#pragma once
#ifndef MATERIAL
#define MATERIAL
#include "glm/glm.hpp"
#include "stb_image.h"
#include "tracing.h"
#include <string.h>

using namespace glm;

struct Texture {

    unsigned char* image;
    int width;
    int height;
    int channel;

};

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

    Material(const char* Name, vec3 Diffuse, Texture* Tex, vec3 Specular, vec3 Transmittance, float Shiness, float Ior) {

        name = Name;
        diffuse = Diffuse;
        texture = Tex;
        specular = Specular;
        transmittance = Transmittance;
        shiness = Shiness;
        IOR = Ior;
        radiance = vec3(0);

    }

    /*~Material() {

        if (texture != NULL) {

            stbi_image_free(texture->image);
            delete(texture);

        }

    }*/

    void SetLightRadiance(vec3 R) { radiance = R; }
    vec3 GetLightRadiance() { return radiance; }

    vec3 phongModelBRDF(vec3 wi, vec3 wo, vec3 normal, vec2 uv) {

        if (dot(wo, normal) < 0 || dot(wi, normal) < 0) return vec3(0);
        vec3 reflectDirection = normalize(2 * dot(wi, normal) * normal - wi);
        vec3 d = diffuse * sampleTexture(uv.x, uv.y) * (1.0f / PI);
        float p = pow(max(dot(reflectDirection, wo), 0.0f), shiness);
        vec3 s = specular * (shiness + 2) * pow(max(dot(reflectDirection, wo), 0.0f), shiness) / (2.0f * PI);
        //vec3 d = diffuse * (1.0f / PI);
        //if (texture != NULL) d = sampleTexture((int)uv.x, (int)uv.y)
        vec3 result = diffuse * sampleTexture(uv.x, uv.y) * (1.0f / PI) + specular * (shiness + 2) * pow(max(dot(reflectDirection, wo), 0.0f), shiness) / (2.0f * PI);

        return result;

    }

    bool randomBRDFRay(vec3 wo, IntersectionPoint IP, Ray& r, float& p) {

        if (dot(wo, IP.n) < 0) return false;
        int rgb = rand() % 3;
        float u1 = rand() * 1.0f / RAND_MAX;
        while (u1 == 0.0f || u1 == 1.0f) u1 = rand() * 1.0f / RAND_MAX;
        float u2 = rand() * 1.0f / RAND_MAX;
        while (u2 == 0.0f || u2 == 1.0f) u2 = rand() * 1.0f / RAND_MAX;

        vec3 y = IP.n;
        vec3 z = normalize(cross(y, wo));
        vec3 x = normalize(cross(y, z));

        //float sampleDiffuse = rand() * 1.0f / RAND_MAX;
        float sampleDiffuse = 0.0f;
        vec3 kd = diffuse * sampleTexture(IP.uv.x, IP.uv.y);
        vec3 ks = specular;
        if (sampleDiffuse <= kd[rgb]) {

            vec2 w = vec2(acosf(sqrt(u1)), 2 * PI * u2);
            vec3 wi = x * cosf(w.y) * sinf(w.x) + y * cosf(w.x) + z * sinf(w.y) * sinf(w.x);
   
            r = Ray(IP.p, wi);
            p = cosf(w.x) / PI;
            return true;

        }

        else if (sampleDiffuse <= kd[rgb] + ks[rgb]) {

            vec3 reflectDirection = normalize(2 * dot(wo, y) * y - wo);
            vec2 w = vec2(acosf(pow(u1, 1.0f / (shiness + 1.0f))), 2 * PI * u2);
            vec3 wi = x * cosf(w.y) * sinf(w.x) + y * cosf(w.x) + z * sinf(w.y) * sinf(w.x);

            r = Ray(IP.p, wi);
            p = pow(max(dot(reflectDirection, wi), 0.0f), shiness) * (shiness + 1) / (2.0f * PI);
            if (p == 0.0f) return this->randomBRDFRay(wo, IP, r, p);
            return true;

        }

        else return false;

    }

    const char* Name() { return name.c_str(); }

    vec3 sampleTexture(float x, float y) {

        if (texture == NULL) return vec3(1.0f);
        while (x < 0) x += 1.0f;
        while (y < 0) y += 1.0f;
        if (x > 1.0f) x -= 1.0f;
        if (y > 1.0f) y -= 1.0f;

        int i = x * texture->width;
        int j = x * texture->height;
        int index = (i + (texture->height - j - 1) * texture->width) * texture->channel;
        vec3 color = vec3(texture->image[index] * 1.0f / 255.0f, texture->image[index + 1] * 1.0f / 255.0f, texture->image[index + 2] * 1.0f / 255.0f);

        return vec3(texture->image[index] * 1.0f / 255.0f, texture->image[index + 1] * 1.0f / 255.0f, texture->image[index + 2] * 1.0f / 255.0f);

    }

private:

    std::string name;
    vec3 diffuse;//-*Kd * : the diffuse reflectance of material, * map_Kd* is the texture file path.
    Texture* texture;
    vec3 specular;    //- *Ks * : the specular reflectance of material.
    vec3 transmittance;    //- *Tr * : the transmittance of material.
    float shiness;    //- *Ns * : shiness, the exponent of phong lobe.
    float IOR;    //- *Ni * : the * Index of Refraction(IOR) * of transparent object like glass and water.
    vec3 radiance;

};

#endif // !MATERIAL
