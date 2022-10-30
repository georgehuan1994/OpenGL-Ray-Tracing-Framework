//
// Created by George Huan on 2022/10/2.
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "Screen.h"
#include "Triangle.h"
#include "BVH.h"
#include "Utility.h"
#include "GameObeject.h"

#include "hdrloader.h"

#include "RenderSettings.h"
#include "Scene.h"

#include <iostream>

using namespace glm;

void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void OnGUI(vector<Triangle_encoded> &triangles_encoded, GLuint tbo0);

int main() {

#pragma region OpenGL Stuff
    glfwInit();
#ifdef __APPLE__
    const char *glsl_version = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    const char *glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Ray Tracing Framework", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width * RENDER_SCALE, height * RENDER_SCALE);
#pragma endregion

    // Init Random Seed
    // ----------------
    CPURandomInit();

    // Build and Compile Shaders
    // -------------------------
    Shader ScreenShader(vertexShaderPath, fragmentShaderScreenPath);
    Shader RayTracerShader(vertexShaderPath, fragmentShaderRayTracingPath);
    Shader ToneMappingShader(vertexShaderPath, fragmentShaderToneMapping);

#ifndef __APPLE__
    // Shader CompShader("../../src/shaders/compute_shader_test.glsl");
#endif

    // Bind VAO, VBO
    // -------------
    screen.InitScreenBind();

    // Init FBO
    // --------
    screenBuffer.Init(width * RENDER_SCALE, height * RENDER_SCALE);

#pragma region Scene/Triangle/BVH

    // Init Scene
    // ----------
    InitScene();

    // Build BVH Node Data
    // -------------------
    BVHNode bvhTestNode;
    bvhTestNode.left = 255;
    bvhTestNode.right = 128;
    bvhTestNode.n = 30;
    bvhTestNode.AA = vec3(1, 1, 0);
    bvhTestNode.BB = vec3(0, 1, 0);
    std::vector<BVHNode> nodes{bvhTestNode};
    // buildBVH(triangles, nodes, 0, triangles.size() - 1, 8);
    buildBVHwithSAH(triangles, nodes, 0, triangles.size() - 1, 8);

    nNodes = nodes.size();
    std::cout << "BVH building completed: " << nNodes << " nodes in total" << std::endl;

    // Encode Triangle Data
    // --------------------
    std::vector<Triangle_encoded> triangles_encoded(nTriangles);
    triangles_encoded_ptr = &triangles_encoded;
    for (int i = 0; i < nTriangles; i++) {
        Triangle &t = triangles[i];
        Material &m = t.material;
        // vertex position
        triangles_encoded[i].p1 = t.p1;
        triangles_encoded[i].p2 = t.p2;
        triangles_encoded[i].p3 = t.p3;
        // vertex normal
        triangles_encoded[i].n1 = t.n1;
        triangles_encoded[i].n2 = t.n2;
        triangles_encoded[i].n3 = t.n3;
        // material
        triangles_encoded[i].emissive = m.emissive;
        triangles_encoded[i].baseColor = m.baseColor;
        triangles_encoded[i].param1 = vec3(m.subsurface, m.metallic, m.specular);
        triangles_encoded[i].param2 = vec3(m.specularTint, m.roughness, m.anisotropic);
        triangles_encoded[i].param3 = vec3(m.sheen, m.sheenTint, m.clearcoat);
        triangles_encoded[i].param4 = vec3(m.clearcoatGloss, m.IOR, m.transmission);
        triangles_encoded[i].mediumColor = m.mediumColor;
        triangles_encoded[i].param5 = vec3(m.mediumType, m.mediumDensity, m.mediumAnisotropy);
    }

    // Encode BVHNode and AABB
    // -----------------------
    std::vector<BVHNode_encoded> nodes_encoded(nNodes);
    nodes_encoded_ptr = &nodes_encoded;
    for (int i = 0; i < nNodes; i++) {
        nodes_encoded[i].childs = vec3(nodes[i].left, nodes[i].right, 0);
        nodes_encoded[i].leafInfo = vec3(nodes[i].n, nodes[i].index, 0);
        nodes_encoded[i].AA = nodes[i].AA;
        nodes_encoded[i].BB = nodes[i].BB;
    }

    // Triangle Texture Buffer
    // -----------------------
    glGenBuffers(1, &tbo0);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo0);
    glBufferData(GL_TEXTURE_BUFFER, triangles_encoded_ptr->size() * sizeof(Triangle_encoded), &triangles_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &trianglesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, trianglesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo0);

    // BVHNode Texture Buffer
    // -----------------------
    glGenBuffers(1, &tbo1);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo1);
    glBufferData(GL_TEXTURE_BUFFER, nodes_encoded_ptr->size() * sizeof(BVHNode_encoded), &nodes_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &nodesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, nodesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo1);

#ifndef __APPLE__
    // // dimensions of the image
    // glGenTextures(1, &tex_output);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, tex_output);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    // glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int work_grp_cnt[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

    printf("max global (total) work group counts x:%i y:%i z:%i\n",
           work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    int work_grp_size[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

    printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
           work_grp_size[0], work_grp_size[1], work_grp_size[2]);

    // glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    // printf("max local work group invocations %i\n", work_grp_inv);
#endif
#pragma endregion

    // glEnable(GL_DEPTH_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Setup Dear ImGui Context
    // ------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    // Setup Dear ImGui style
    // ----------------------
    ImGui::StyleColorsClassic();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    RayTracerShader.use();

    RayTracerShader.setInt("nTriangles", nTriangles);
    RayTracerShader.setInt("nNodes", nodes.size());

    RayTracerShader.setInt("hdrResolution", hdrResolution);
    RayTracerShader.setInt("historyTexture", 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_BUFFER, trianglesTextureBuffer);
    RayTracerShader.setInt("triangles", 1);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_BUFFER, nodesTextureBuffer);
    RayTracerShader.setInt("nodes", 2);

    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, hdrMap);
    RayTracerShader.setInt("hdrMap", 3);

    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, hdrCache);
    RayTracerShader.setInt("hdrCache", 4);

    camera.Refresh();
    for (int i = 0; i < 3; ++i) {
        cameraPosition[i] = camera.Position[i];
        cameraRotation[i] = camera.Rotation[i];
    }

    // Render Loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        fps = 1.0f / deltaTime;

        processInput(window);

        OnGUI(triangles_encoded, tbo0);

        if (maxIterations == -1 || camera.LoopNum < maxIterations) { camera.LoopIncrease(); }

        // Ray Tracer Shader
        {
            screenBuffer.setCurrentBuffer(camera.LoopNum);

            RayTracerShader.use();
            RayTracerShader.setVec3("camera.position", camera.Position);
            RayTracerShader.setVec3("camera.front", camera.Front);
            RayTracerShader.setVec3("camera.right", camera.Right);
            RayTracerShader.setVec3("camera.up", camera.Up);
            RayTracerShader.setFloat("camera.halfH", camera.halfH);
            RayTracerShader.setFloat("camera.halfW", camera.halfW);
            RayTracerShader.setVec3("camera.leftBottomCorner", camera.LeftBottomCorner);
            RayTracerShader.setInt("camera.loopNum", camera.LoopNum);
            RayTracerShader.setFloat("randOrigin", 674764.0f * (GetCPURandom() + 1.0f));
            RayTracerShader.setInt("screenWidth", width);
            RayTracerShader.setInt("screenHeight", height);
            RayTracerShader.setBool("enableMultiImportantSample", enableMultiImportantSample);
            RayTracerShader.setBool("enableEnvMap", enableEnvMap);
            RayTracerShader.setFloat("envIntensity", envIntensity);
            RayTracerShader.setFloat("envAngle", envAngle);
            RayTracerShader.setInt("maxBounce", maxBounce);
            RayTracerShader.setInt("maxIterations", maxIterations);
            RayTracerShader.setBool("enableBSDF", enableBSDF);
            screen.DrawScreen();
        }

        // Screen Shader
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ScreenShader.use();
            screenBuffer.setCurrentAsTexture(camera.LoopNum);
            ScreenShader.setInt("screenTexture", 0);
            screen.DrawScreen();
        }

        // ToneMapping Shader
        if (enableToneMapping) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0, 0, 0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ToneMappingShader.use();
            screenBuffer.setCurrentAsTexture(camera.LoopNum);
            ToneMappingShader.setBool("enableToneMapping", enableToneMapping);
            ToneMappingShader.setBool("enableGammaCorrection", enableGammaCorrection);
            ToneMappingShader.setInt("texPass0", 0);
            screen.DrawScreen();
        }

        // Compute Shader
#ifndef __APPLE__
        // {
        //     CompShader.use();
        //     // glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        //     glDispatchCompute((GLuint)width, (GLuint)height, 1);
        //     glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //
        //     glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        //     glClear(GL_COLOR_BUFFER_BIT);
        //
        //     ScreenShader.use();
        //     glActiveTexture(GL_TEXTURE0);
        //     glBindTexture(GL_TEXTURE_2D, tex_output);
        //     screen.DrawScreen();
        // }
#endif


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    screenBuffer.Delete();
    screen.Delete();

    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        glfwGetFramebufferSize(window, &width, &height);
        SaveFrame("../../screenshot/screenshot_" + to_string(camera.LoopNum) + "_spp.png", width, height);
    }
    for (int i = 0; i < 3; ++i) {
        cameraPosition[i] = camera.Position[i];
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glfwGetFramebufferSize(window, &width, &height);
    camera.ProcessScreenRatio(width * RENDER_SCALE, height * RENDER_SCALE);
    screenBuffer.Resize(width * RENDER_SCALE, height * RENDER_SCALE);
    glViewport(0, 0, width * RENDER_SCALE, height * RENDER_SCALE);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.ProcessMouseMovement(xoffset, yoffset);
        for (int i = 0; i < 3; ++i) {
            cameraRotation[i] = camera.Rotation[i];
        }
    }
}

void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    // camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void setDirty() {
    RefreshTriangleMaterial(current_game_object.triangleIndex, triangles, *triangles_encoded_ptr, current_material, tbo0, trianglesTextureBuffer);
    camera.LoopNum = 0;
}

void OnGUI(vector<Triangle_encoded> &triangles_encoded, GLuint tbo0) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glfwGetFramebufferSize(window, &width, &height);
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
#if __APPLE__
    ImGui::SetNextWindowSizeConstraints(ImVec2(10,10), ImVec2(width, height / 2.0 - 20));
#else
    ImGui::SetNextWindowSizeConstraints(ImVec2(10, 10), ImVec2(width, height - 20));
#endif

    ImGui::Begin("Inspector", nullptr, window_flags);
    ImGui::Text("RMB: rotate the camera");
    ImGui::Text("WASDQE: move the camera");
    ImGui::Separator();
    if (ImGui::Checkbox("Enable HDR EnvMap", &enableEnvMap)) {
        camera.LoopNum = 0;
    }
    if (enableEnvMap) {
        if (ImGui::SliderFloat("Env Intensity", &envIntensity, 0, 10)) {
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Env Angle", &envAngle, -1, 1)) {
            camera.LoopNum = 0;
        }
    }
    if (ImGui::Checkbox("Enable Multi-Important Sampling", &enableMultiImportantSample)) {
        camera.LoopNum = 0;
    }
    if (ImGui::SliderInt("Max Bounce", &maxBounce, 1, MAX_BOUNCE)) {
        camera.LoopNum = 0;
    }
    ImGui::Separator();
    ImGui::Text("Screen Buffer Size: (%d x %d)", width, height);
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    if (ImGui::SliderInt("Max Iterations", &maxIterations, -1, 3000)) {
        camera.LoopNum = 0;
    }
    ImGui::SameLine();
    Helper("-1: No Limit");
    ImGui::Text("Iterations: %d / %d", camera.LoopNum, maxIterations);
    ImGui::Separator();
    if (ImGui::InputFloat3("Camera Position", cameraPosition)) {
        camera.Position = vec3(cameraPosition[0], cameraPosition[1], cameraPosition[2]);
        camera.Refresh();
    }
    if (ImGui::InputFloat3("Camera Rotation", cameraRotation)) {
        camera.Rotation = vec3(cameraRotation[0], cameraRotation[1], cameraRotation[2]);
        camera.Refresh();
    }
    if (ImGui::SliderFloat("Camera Zoom", &cameraZoom, 1.0f, 45.0f)) {
        camera.Zoom = cameraZoom;
        camera.Refresh();
    }
    ImGui::Separator();
    ImGui::Checkbox("Enable ToneMapping", &enableToneMapping);
    ImGui::Checkbox("Enable Gamma Correction", &enableGammaCorrection);
    ImGui::Separator();

    // ImGui::Checkbox("Demo Window", &show_demo_window);
    // if (show_demo_window)
    //     ImGui::ShowDemoWindow(&show_demo_window);

    if (ImGui::Button("Save Image")) {
        SaveFrame("../../screenshot/screenshot_" + to_string(camera.LoopNum) + "_spp.png", width, height);
    }
    ImGui::Separator();
    if (ImGui::Checkbox("Enable BSDF Properties", &enableBSDF)) {
        camera.LoopNum = 0;
    }
    if (ImGui::ColorEdit3("Base Color", baseColor)) {
        current_material.baseColor = vec3(baseColor[0], baseColor[1], baseColor[2]);
        setDirty();
    }
    if (ImGui::SliderFloat("Subsurface", &subsurface, 0.0f, 1.0f)) {
        current_material.subsurface = subsurface;
        setDirty();
    }
    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
        current_material.metallic = metallic;
        setDirty();
    }
    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
        current_material.roughness = roughness;
        setDirty();
    }
    if (!enableBSDF) {
        if (ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f)) {
            current_material.specular = specular;
            setDirty();
        }
    }
    if (ImGui::SliderFloat("Specular Tint", &specularTint, 0.0f, 1.0f)) {
        current_material.specularTint = specularTint;
        setDirty();
    }
    if (ImGui::SliderFloat("Anisotropic", &anisotropic, 0.0f, 1.0f)) {
        current_material.anisotropic = anisotropic;
        setDirty();
    }
    if (ImGui::ColorEdit3("Emissive", emissive)) {
        current_material.emissive = vec3(emissive[0], emissive[1], emissive[2]);
        setDirty();
    }
    if (ImGui::SliderFloat("Sheen", &sheen, 0.0f, 1.0f)) {
        current_material.sheen = sheen;
        setDirty();
    }
    if (ImGui::SliderFloat("Sheen Tint", &sheenTint, 0.0f, 1.0f)) {
        current_material.sheenTint = sheenTint;
        setDirty();
    }
    if (ImGui::SliderFloat("Clearcoat", &clearcoat, 0.0f, 1.0f)) {
        current_material.clearcoat = clearcoat;
        setDirty();
    }
    if (ImGui::SliderFloat("Clearcoat Gloss", &clearcoatGloss, 0.0f, 1.0f)) {
        current_material.clearcoatGloss = clearcoatGloss;
        setDirty();
    }
    if (enableBSDF) {
        if (ImGui::SliderFloat("IOR", &IOR, 0.001f, 2.45f)) {
            current_material.IOR = IOR;
            setDirty();
        }
        if (ImGui::SliderFloat("Transmission", &transmission, 0.0f, 1.0f)) {
            current_material.transmission = transmission;
            setDirty();
        }
        if (ImGui::Combo("Medium Type", &mediumType, "None\0Absorb\0Scatter\0Emissive\0\0")) {
            current_material.mediumType = (float) mediumType;
            setDirty();
        }
        if (ImGui::ColorEdit3("Medium Color", mediumColor)) {
            current_material.mediumColor = vec3(mediumColor[0], mediumColor[1], mediumColor[2]);
            setDirty();
        }
        if (ImGui::SliderFloat("Medium Density", &mediumDensity, 0, 1)) {
            current_material.mediumDensity = mediumDensity;
            setDirty();
        }
        if (ImGui::SliderFloat("Medium Anisotropy", &mediumAnisotropy, 0, 1)) {
            current_material.mediumAnisotropy = mediumAnisotropy;
            setDirty();
        }
    }

    ImGui::End();
}

