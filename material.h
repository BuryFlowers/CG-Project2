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
    //std::string image;
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

    Material(const char* Name, vec3 Diffuse, std::string TexturePath, vec3 Specular, vec3 Transmittance, float Shiness, float Ior) {

        name = Name;
        diffuse = Diffuse;
        texture = NULL;
        texturePath = TexturePath;
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
            double dp = pow(max((double)dot(reflectDirection, wi), 0.0), shiness) * (shiness + 1) / (2.0 * PI);
            p = (float)dp;
            //if (p < 1e-2) return this->randomBRDFRay(wo, IP, r, p);
            return true;

        }

        else return false;

    }

    float getDiffusePossibility(vec3 normal, vec3 generatedDirection) {

        float costheta = dot(generatedDirection, normal);
        if (costheta < 0) return 0;
        return costheta / PI;

    }

    float getSpecularPossibility(vec3 normal, vec3 wi, vec3 wo) {

        vec3 y = normal;
        vec3 z = normalize(cross(y, wo));
        vec3 x = normalize(cross(y, z));
        float costheta = dot(wi, normal);
        if (costheta < 0) return 0;
        vec3 reflectDirection = normalize(2 * dot(wo, y) * y - wo);
        float cosalpha = dot(reflectDirection, wi);
        if (cosalpha < 0) return 0;
        return pow(cosalpha, shiness) * (shiness + 1) / (2.0f * PI);

    }

    bool isTransparent() { return IOR > 1.0f; }

    float refractionRay(vec3 wo, IntersectionPoint IP, Ray& r) {

        float ior = IOR;
        if (dot(wo, IP.n) < 0) ior = 1.0f / IOR, IP.n *= -1.0f;
        float coso = fabs(dot(wo, IP.n));
        if (1.0f - (1.0f - coso * coso) < 0) return 1.0f;
        float cosi = sqrt(1.0f - (1.0f - coso * coso) / (ior * ior));
        vec3 wi = normalize(-IP.n * cosi + (IP.n * coso - wo) / ior);
        r = Ray(IP.p, wi);

        float RV = (ior * coso - cosi) / (ior * coso + cosi);
        float RH = (coso - ior * cosi) / (coso + ior * cosi);

        return 0.5f * (RV * RV + RH * RH);

    }

    //bool refrectionRay(vec3 wo, IntersectionPoint IP, Ray& r) {

    //    int rgb = rand() % 3;
    //    float p = rand() * 1.0f / RAND_MAX;

    //    //if (p <= transmittance[rgb]) return false;

    //    if (transmittance.x >= 1.0f && transmittance.y >= 1.0f && transmittance.z >= 1.0f) return false;

    //    float ior = IOR;
    //    if (dot(wo, IP.n) < 0) ior = 1.0f / IOR, IP.n *= -1.0f;
    //    float coso = fabs(dot(wo, IP.n));
    //    if (1.0f - (1.0f - coso * coso) < 0) return false;
    //    float cosi = sqrt(1.0f - (1.0f - coso * coso) / (ior * ior));

    //    vec3 wi = normalize(- IP.n * cosi + (IP.n * coso - wo) / ior);
    //    r = Ray(IP.p, wi);

    //    return true;

    //}

    vec3 Trans() { return transmittance; }
    const char* Name() { return name.c_str(); }

    vec3 sampleTexture(float x, float y) {

        if (texturePath.empty()) return vec3(1.0f);
        if (texture == NULL) {

            texture = new Texture();
            texture->image = stbi_load(texturePath.c_str(), &texture->width, &texture->height, &texture->channel, 0);

        }

        int i = x * texture->width;
        int j = y * texture->height;

        if (i < 0) i %= texture->width, i += texture->width;
        if (j < 0) j %= texture->height, j += texture->height;
        if (i >= texture->width) i %= texture->width;
        if (j >= texture->height) j %= texture->height;
      
        int index = (i + (texture->height - j - 1) * texture->width) * texture->channel;
        vec3 color = vec3(texture->image[index] * 1.0f / 255.0f, texture->image[index + 1] * 1.0f / 255.0f, texture->image[index + 2] * 1.0f / 255.0f);
        //printf("%d %.2lf\n", texture->image[index], color.x);

        //if (name == "Wood"/* && i == 312 && j == 883*/) {

        //    printf("i:%d j:%d\n", i, j);
        //    printf("%d %d %d\n", texture->image[index], texture->image[index + 1], texture->image[index + 2]);
        //    printf("%.2lf %.2lf %.2lf\n\n", color.x, color.y, color.z);

        //}

        return vec3(texture->image[index] * 1.0f / 255.0f, texture->image[index + 1] * 1.0f / 255.0f, texture->image[index + 2] * 1.0f / 255.0f);

    }

private:

    std::string name;
    vec3 diffuse;//-*Kd * : the diffuse reflectance of material, * map_Kd* is the texture file path.
    std::string texturePath;
    Texture* texture;
    vec3 specular;    //- *Ks * : the specular reflectance of material.
    vec3 transmittance;    //- *Tr * : the transmittance of material.
    float shiness;    //- *Ns * : shiness, the exponent of phong lobe.
    float IOR;    //- *Ni * : the * Index of Refraction(IOR) * of transparent object like glass and water.
    vec3 radiance;

};

#endif // !MATERIAL
