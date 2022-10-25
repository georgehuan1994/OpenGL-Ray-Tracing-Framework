//
// Created by George Huan on 2022/10/5.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

struct Material {
    glm::vec3   emissive        = glm::vec3(0, 0, 0);  // 作为光源时的发光颜色
    glm::vec3   baseColor       = glm::vec3(1, 1, 1);
    float       subsurface      = 0.0;
    float       metallic        = 0.0;
    float       specular        = 0.0;
    float       specularTint    = 0.0;
    float       roughness       = 0.0;
    float       anisotropic     = 0.0;
    float       sheen           = 0.0;
    float       sheenTint       = 0.0;
    float       clearcoat       = 0.0;
    float       clearcoatGloss  = 0.0;
    float       IOR             = 1.0;
    float       transmission    = 0.0;

    float       baseColorTexID          = -1.0;
    float       metallicRoughnessTexID  = -1.0;
    float       normalmapTexID          = -1.0;
    float       emissionmapTexID        = -1.0;
};

static float baseColor[4]   = {1.0f, 1.0, 1.0f, 1.0f};
static float emissive[3]    = {0.0f, 0.0f, 0.0f};
static float subsurface     = 0.0f;
static float metallic       = 0.0f;
static float specular       = 0.0f;
static float specularTint   = 0.0f;
static float roughness      = 0.0f;
static float anisotropic    = 0.0f;
static float sheen          = 0.0f;
static float sheenTint      = 0.0f;
static float clearcoat      = 0.0f;
static float clearcoatGloss = 0.0f;
static float IOR            = 1.5f;
static float transmission   = 0.0f;

void SetGlobalMaterialProperty(Material material) {
    baseColor[0]    = material.baseColor.x;
    baseColor[1]    = material.baseColor.y;
    baseColor[2]    = material.baseColor.z;

    emissive[0]     = material.emissive.x;
    emissive[1]     = material.emissive.y;
    emissive[2]     = material.emissive.z;

    subsurface      = material.subsurface;
    metallic        = material.metallic;
    specular        = material.specular;
    specularTint    = material.specularTint;
    roughness       = material.roughness;
    anisotropic     = material.anisotropic;
    sheen           = material.sheen;
    sheenTint       = material.sheenTint;
    clearcoat       = material.clearcoat;
    clearcoatGloss  = material.clearcoatGloss;
    IOR             = material.IOR;
    transmission    = material.transmission;
}

#endif //MATERIAL_H
