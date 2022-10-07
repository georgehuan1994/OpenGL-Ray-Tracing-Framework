//
// Created by George Huan on 2022/10/6.
//

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Mesh.h"
#include "Shader.h"
#include "Material.h"

#include <vector>

struct Triangle {
    glm::vec3 p1, p2, p3;   // 顶点坐标
    glm::vec3 n1, n2, n3;   // 顶点法线
    Material material;      // 材质
};

struct Triangle_encoded {
    glm::vec3 p1, p2, p3;    // 顶点坐标
    glm::vec3 n1, n2, n3;    // 顶点法线
    glm::vec3 emissive;      // 自发光参数
    glm::vec3 baseColor;     // 颜色
    glm::vec3 param1;        // (subsurface, metallic, specular)
    glm::vec3 param2;        // (specularTint, roughness, anisotropic)
    glm::vec3 param3;        // (sheen, sheenTint, clearcoat)
    glm::vec3 param4;        // (clearcoatGloss, IOR, transmission)
};

void getTriangle(std::vector<Mesh> &data, std::vector<Triangle> &triangles, Material material, mat4 trans,
                 bool smoothNormal) {
    // 顶点位置，索引
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<GLuint> indices;

    // 计算 AABB 盒，归一化模型大小
    float maxx = -11451419.19;
    float maxy = -11451419.19;
    float maxz = -11451419.19;
    float minx = 11451419.19;
    float miny = 11451419.19;
    float minz = 11451419.19;

    for (int i = 0; i < data.size(); ++i) {
        for (int j = 0; j < data[i].vertices.size(); ++j) {
            vertices.push_back(data[i].vertices[j].Position);
            normals.push_back(data[i].vertices[j].Normal);
            maxx = glm::max(maxx, data[i].vertices[j].Position.x);
            maxy = glm::max(maxx, data[i].vertices[j].Position.y);
            maxz = glm::max(maxx, data[i].vertices[j].Position.z);
            minx = glm::min(minx, data[i].vertices[j].Position.x);
            miny = glm::min(minx, data[i].vertices[j].Position.y);
            minz = glm::min(minx, data[i].vertices[j].Position.z);
//            std::cout << j << " Normal: " << data[i].vertices[j].Position.x << ", " << data[i].vertices[j].Position.y << ", "<< data[i].vertices[j].Position.z << std::endl;
        }
        for (int k = 0; k < data[i].indices.size(); ++k) {
            indices.push_back(data[i].indices[k]);
//            std::cout << k << " Position: " << data[i].indices[k] << std::endl;
        }
    }

    // 模型大小归一化
    float lenx = maxx - minx;
    float leny = maxy - miny;
    float lenz = maxz - minz;
    float maxaxis = glm::max(lenx, glm::max(leny, lenz));
    for (auto &v: vertices) {
        v.x /= maxaxis;
        v.y /= maxaxis;
        v.z /= maxaxis;
    }

    // 通过矩阵进行坐标变换
    for (auto &v: vertices) {
        vec4 vv = vec4(v.x, v.y, v.z, 1);
        vv = trans * vv;
        v = vec3(vv.x, vv.y, vv.z);
    }

    // 构建 Triangle 对象数组
    int offset = triangles.size();  // 增量更新
    triangles.resize(offset + indices.size() / 3);
    for (int i = 0; i < indices.size(); i += 3) {
        Triangle &t = triangles[offset + i / 3];

        // 传顶点属性
        t.p1 = vertices[indices[i]];
        t.p2 = vertices[indices[i + 1]];
        t.p3 = vertices[indices[i + 2]];

        // 传顶点法线
        if (!smoothNormal) {
            vec3 n = normalize(cross(t.p2 - t.p1, t.p3 - t.p1));
            t.n1 = n;
            t.n2 = n;
            t.n3 = n;
        } else {
            t.n1 = normalize(normals[indices[i]]);
            t.n2 = normalize(normals[indices[i + 1]]);
            t.n3 = normalize(normals[indices[i + 2]]);
        }

        // 传材质
        t.material = material;
    }
}

#endif //TRIANGLE_H