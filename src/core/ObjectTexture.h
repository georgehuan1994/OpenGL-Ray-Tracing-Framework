//
// Created by George Huan on 2022/10/4.
//

#ifndef OBJECTTEXTURE_H
#define OBJECTTEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Mesh.h"
#include "Shader.h"
#include "Material.h"

#include <vector>

class ObjectTexture {
public:
    GLuint ID_vTex;
    GLuint ID_fTex;
    int dataSize_v, dataSize_f;

    void setTex(Shader &shader) {

        shader.setInt("meshVertexNum", dataSize_v);
        shader.setInt("meshFaceNum", dataSize_f);

        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, ID_vTex);

        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, ID_fTex);

        shader.setInt("texMeshVertex", 1);
        shader.setInt("texMeshFaceIndex", 2);
    }
};


void getTexture(std::vector<Mesh> &data, Shader &shader, ObjectTexture &objTex) {
    int dataSize_v = 0, dataSize_f = 0;
    for (int i = 0; i < data.size(); i++) {
        dataSize_v += data[i].vertices.size();
        dataSize_f += data[i].indices.size();
    }
    std::cout << "dataSize_t = " << dataSize_v << std::endl;
    std::cout << "dataSize_f = " << dataSize_f << std::endl;


    float vert_x_f = sqrtf(dataSize_v);
    int vert_x = ceilf(vert_x_f);
    int vert_y = ceilf((float) dataSize_v / (float) vert_x);
    std::cout << "vert_x = " << vert_x << " vert_y = " << vert_y << std::endl;

    float face_x_f = sqrtf(dataSize_f);
    int face_x = ceilf(face_x_f);
    int face_y = ceilf((float) dataSize_f / (float) face_x);
    std::cout << "face_x = " << face_x << " face_y = " << face_y << std::endl;

    float *vertexArray = new float[(3 + 3 + 2) * (vert_x * vert_y)];
    float *faceArray = new float[(1) * (face_x * face_y)];
    int v_index = 0;
    int f_index = 0;
    for (int i = 0; i < data.size(); i++) {
        for (int j = 0; j < data[i].vertices.size(); j++) {
            vertexArray[v_index * (8) + 0] = 0.04 * data[i].vertices[j].Position.x;
            vertexArray[v_index * (8) + 1] = 0.04 * data[i].vertices[j].Position.y - 0.2;
            vertexArray[v_index * (8) + 2] = 0.04 * data[i].vertices[j].Position.z;

            vertexArray[v_index * (8) + 3] = data[i].vertices[j].Normal.x;
            vertexArray[v_index * (8) + 4] = data[i].vertices[j].Normal.y;
            vertexArray[v_index * (8) + 5] = data[i].vertices[j].Normal.z;

            vertexArray[v_index * (8) + 6] = data[i].vertices[j].TexCoords.x;
            vertexArray[v_index * (8) + 7] = data[i].vertices[j].TexCoords.y;

            v_index++;
        }
        for (int j = 0; j < data[i].indices.size(); j++) {
            faceArray[f_index] = data[i].indices[j];

            f_index++;
        }
    }

    for (int i = 0; i < 8 * 3; i++) {
        //std::cout << vertexArray[i] << " " << std::endl;
        std::cout << faceArray[i] << " " << std::endl;
    }

    objTex.dataSize_v = dataSize_v;
    objTex.dataSize_f = dataSize_f;

    shader.use();

    glGenTextures(1, &objTex.ID_vTex);
    glBindTexture(GL_TEXTURE_2D, objTex.ID_vTex);//dataSize_v * 8
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, vert_x * 8, vert_y, 0, GL_RED, GL_FLOAT, vertexArray);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    shader.setInt("texMeshVertex", 1);

    glGenTextures(1, &objTex.ID_fTex);
    glBindTexture(GL_TEXTURE_2D, objTex.ID_fTex); //dataSize_f
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, face_x, face_y, 0, GL_RED, GL_FLOAT, faceArray);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    shader.setInt("texMeshFaceIndex", 2);

    delete[] vertexArray;
    delete[] faceArray;

}

#endif //OBJECTTEXTURE_H
