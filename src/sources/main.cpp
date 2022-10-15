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

#include "hdrloader.h"

#include <iostream>

using namespace glm;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

// Settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 512;
#define RENDER_SCALE 1
#define MAX_BOUNCE 4

// Camera
Camera camera((float) SCR_WIDTH / (float) SCR_HEIGHT,
              glm::vec3(0.0f, 0.0f, 7.0f),
              glm::vec3(-87.78f, -14.0f, 0.0f));
static float cameraPosition[3] = {camera.Position.x, camera.Position.y, camera.Position.z};
static float cameraRotation[3] = {camera.Rotation.x, camera.Rotation.y, camera.Rotation.z};
static float cameraZoom = 25.0f;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timer
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fps = 0.0f;

// Screen FBO
RenderBuffer screenBuffer;

// Triangle Texture Buffer Data
GLuint trianglesTextureBuffer;

// BVH Node Texture Buffer Data
GLuint nodesTextureBuffer;

// HDR Map Data
GLuint hdrMap;
GLuint hdrCache;
int hdrResolution;

int main() {

    glfwInit();
    const char *glsl_version = "#version 330 core";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Ray Tracing Framework", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width * RENDER_SCALE, height * RENDER_SCALE);

    // stbi_set_flip_vertically_on_load(true);

    CPURandomInit();

    // Build and Compile Shaders
    // -------------------------
    const char *vertexShaderPath = "../../src/shaders/vertex_shader.glsl";
    Shader RayTracerShader(vertexShaderPath,"../../src/shaders/fragment_shader_ray_tracing.glsl");
    Shader ScreenShader(vertexShaderPath, "../../src/shaders/fragment_shader_screen.glsl");
    Shader ToneMappingShader(vertexShaderPath,"../../src/shaders/fragment_shader_tone_mapping.glsl");

    Screen screen;
    screen.InitScreenBind();
    screenBuffer.Init(width * RENDER_SCALE, height * RENDER_SCALE);

    RayTracerShader.use();

    std::vector<Triangle> triangles;


#pragma region Scene

    Material plane;
    plane.baseColor = vec3(0.73, 0.73, 0.73);
    plane.roughness = 0.1;
    plane.specular = 1.0;

    Material white;
    white.baseColor = vec3(0.73, 0.73, 0.73);
    white.roughness = 0.5;
    white.specular = 0.5;

    Material jade;
    jade.baseColor = vec3(0.55, 0.78, 0.55);
    jade.roughness = 0.1;
    jade.specular = 1.0;
    jade.subsurface = 1.0;

    Material golden;
    golden.baseColor = vec3(0.75, 0.7, 0.15);
    golden.roughness = 0.1;
    golden.specular = 1.0;
    golden.metallic = 1.0;

    // TODO GameObject

    Material current_material = jade;
    SetGlobalMaterialProperty(current_material);

    Model floor("../../resources/objects/floor.obj");
    getTriangle(floor.meshes, triangles, plane,
                getTransformMatrix(vec3(0), vec3(2.2, -2, 3), vec3(14, 7 ,7)), false);

    Model bunny("../../resources/objects/bunny_4000.obj");   // 4000 face
    getTriangle(bunny.meshes, triangles, current_material,
                getTransformMatrix(vec3(0), vec3(2.2, -2.5, 3), vec3(2)), false);

    // Model loong("../../resources/objects/loong.obj");        // 100000 face
    // getTriangle(loong.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0), vec3(2, -2.2, 3), vec3(3.5)), true);

    // camera.Rotation = glm::vec3(-90.0f, -14.0f, 0.0f);
    // Model dragon("../../resources/objects/dragon.obj");     // 831812 face
    // getTriangle(dragon.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0, 120, 0), vec3(-0.2, -2.5, 3), vec3(4)), false);

#pragma endregion

    int nTriangles = triangles.size();
    std::cout << "Scene loading completed: " << nTriangles << " triangle faces in total" << std::endl;

    // Build BVH Node Data
    // -------------------
    BVHNode testNode;
    testNode.left = 255;
    testNode.right = 128;
    testNode.n = 30;
    testNode.AA = vec3(1, 1, 0);
    testNode.BB = vec3(0, 1, 0);
    std::vector<BVHNode> nodes{testNode};
    // buildBVH(triangles, nodes, 0, triangles.size() - 1, 8);
    buildBVHwithSAH(triangles, nodes, 0, triangles.size() - 1, 8);
    int nNodes = nodes.size();
    std::cout << "BVH building completed: " << nNodes << " nodes in total" << std::endl;

    // Encode Triangle Data
    // --------------------
    std::vector<Triangle_encoded> triangles_encoded(nTriangles);
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
    }
    RayTracerShader.setInt("nTriangles", nTriangles);

    // Encode BVHNode and AABB
    // -----------------------
    std::vector<BVHNode_encoded> nodes_encoded(nNodes);
    for (int i = 0; i < nNodes; i++) {
        nodes_encoded[i].childs = vec3(nodes[i].left, nodes[i].right, 0);
        nodes_encoded[i].leafInfo = vec3(nodes[i].n, nodes[i].index, 0);
        nodes_encoded[i].AA = nodes[i].AA;
        nodes_encoded[i].BB = nodes[i].BB;
    }
    RayTracerShader.setInt("nNodes", nodes.size());

    // Triangle Texture Buffer
    // -----------------------
    GLuint tbo0;
    glGenBuffers(1, &tbo0);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo0);
    glBufferData(GL_TEXTURE_BUFFER, triangles_encoded.size() * sizeof(Triangle_encoded), &triangles_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &trianglesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, trianglesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo0);

    // BVHNode Texture Buffer
    // -----------------------
    GLuint tbo1;
    glGenBuffers(1, &tbo1);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo1);
    glBufferData(GL_TEXTURE_BUFFER, nodes_encoded.size() * sizeof(BVHNode_encoded), &nodes_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &nodesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, nodesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo1);

    // HDR Environment Map
    // -------------------
    HDRLoaderResult hdrRes;
    bool r = HDRLoader::load("../../resources/textures/hdr/peppermint_powerplant_4k.hdr", hdrRes);
    hdrMap = getTextureRGB32F(hdrRes.width, hdrRes.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, hdrRes.width, hdrRes.height, 0, GL_RGB, GL_FLOAT, hdrRes.cols);

    // HDR Important Sampling Cache
    // ----------------------------
    std::cout << "HDR Map Important Sample Cache, HDR Resolution: " << hdrRes.width << " x " << hdrRes.height << std::endl;
    float* cache = calculateHdrCache(hdrRes.cols, hdrRes.width, hdrRes.height);
    hdrCache = getTextureRGB32F(hdrRes.width, hdrRes.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, hdrRes.width, hdrRes.height, 0, GL_RGB, GL_FLOAT, cache);
    hdrResolution = hdrRes.width;

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

    camera.Refresh();

    // glEnable(GL_DEPTH_TEST);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Render Setting
    bool show_demo_window = false;
    bool enableImportantSample = true;
    bool enableEnvMap = true;
    bool enableToneMapping = true;
    bool enableGammaCorrection = true;
    int maxBounce = 4;
    int maxIterations = 1000;
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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwGetFramebufferSize(window, &width, &height);
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
#if __APPLE__
        ImGui::SetNextWindowSizeConstraints(ImVec2(10,10), ImVec2(width, height / 2.0 - 20));
#else
        ImGui::SetNextWindowSizeConstraints(ImVec2(10,10), ImVec2(width, height - 20));
#endif
        ImGui::Begin("Inspector", nullptr, window_flags);
        ImGui::Text("RMB: rotate the camera");
        ImGui::Text("WASDQE: move the camera");
        ImGui::Separator();
        if (ImGui::Checkbox("Enable HDR EnvMap", &enableEnvMap)) {
            camera.LoopNum = 0;
        }
        if (ImGui::Checkbox("Enable Important Sampling", &enableImportantSample)) {
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
        ImGui::SameLine(); Helper("-1: No Limit");
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
            SaveFrame("../../screenshot/screenshot_bunny_" + to_string(camera.LoopNum) + "_spp.png", width, height);
        }
        ImGui::Separator();
        if (ImGui::ColorEdit4("Base Color", baseColor)) {
            current_material.baseColor = vec3(baseColor[0], baseColor[1], baseColor[2]);
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Subsurface", &subsurface, 0.0f, 1.0f)) {
            current_material.subsurface = subsurface;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
            current_material.metallic = metallic;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
            current_material.roughness = roughness;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f)) {
            current_material.specular = specular;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Specular Tine", &specularTint, 0.0f, 1.0f)) {
            current_material.specularTint = specularTint;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Anisotropic", &anisotropic, 0.0f, 1.0f)) {
            current_material.anisotropic = anisotropic;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::ColorEdit4("Emissive", emissive)) {
            current_material.emissive = vec3(emissive[0], emissive[1], emissive[2]);
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Sheen", &sheen, 0.0f, 1.0f)) {
            current_material.sheen = sheen;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Sheen Tint", &sheenTint, 0.0f, 1.0f)) {
            current_material.sheenTint = sheenTint;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Clearcoat", &clearcoat, 0.0f, 1.0f)) {
            current_material.clearcoat = clearcoat;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Clearcoat Gloss", &clearcoatGloss, 0.0f, 1.0f)) {
            current_material.clearcoatGloss = clearcoatGloss;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("IOR", &IOR, 0.0f, 1.5f)) {
            current_material.IOR = IOR;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }
        if (ImGui::SliderFloat("Transmission", &transmission, 0.0f, 1.0f)) {
            current_material.transmission = transmission;
            RefreshTriangleMaterial(triangles, triangles_encoded, current_material, tbo0, trianglesTextureBuffer);
            camera.LoopNum = 0;
        }

        ImGui::End();

        if (maxIterations == -1 || camera.LoopNum < maxIterations) { camera.LoopIncrease(); }

        {
            screenBuffer.setCurrentBuffer(camera.LoopNum);

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
            RayTracerShader.setBool("enableImportantSample", enableImportantSample);
            RayTracerShader.setBool("enableEnvMap", enableEnvMap);
            RayTracerShader.setInt("maxBounce", maxBounce);
            RayTracerShader.setInt("maxIterations", maxIterations);
            screen.DrawScreen();
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ScreenShader.use();
            screenBuffer.setCurrentAsTexture(camera.LoopNum);
            ScreenShader.setInt("screenTexture", 0);
            screen.DrawScreen();
        }

        if (enableToneMapping)
        {
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

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    // camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

