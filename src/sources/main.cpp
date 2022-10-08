#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "Screen.h"
#include "Utility.h"
#include "Triangle.h"
#include "BVH.h"

#include "hdrloader.h"

#include <iostream>

using namespace glm;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;
const unsigned int SCENE_WIDTH = 512;
const unsigned int SCENE_HEIGHT = 512;

// camera
Camera camera((float) SCR_WIDTH / (float) SCR_HEIGHT, glm::vec3(0.0f, 0.0f, 7.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fps;

// screen FBO
RenderBuffer screenBuffer;

// triangle data
GLuint trianglesTextureBuffer;

// bvh node data
GLuint nodesTextureBuffer;

// hdr map data
GLuint hdrMap;

int main() {

    glfwInit();
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tiny GL PathTracer", NULL, NULL);
    if (window == NULL) {
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

    // stbi_set_flip_vertically_on_load(true);

    // glEnable(GL_DEPTH_TEST);

    CPURandomInit();

    // build and compile shaders
    // -------------------------
    // Shader ourShader("../../src/shaders/1.model_loading.vs",
    //                  "../../src/shaders/1.model_loading.fs");

    // Shader ourShader("../../src/shaders/vertexShader.glsl",
    //                  "../../src/shaders/fragmentShader.glsl");

    Shader RayTracerShader("../../src/shaders/RayTracerVertexShader.glsl","../../src/shaders/RayTracerFragmentShader.glsl");
    Shader ScreenShader("../../src/shaders/ScreenVertexShader.glsl","../../src/shaders/ScreenFragmentShader.glsl");

    // load models
    // -----------
    Model sphere("../../resources/objects/sphere.obj");
    Model quad("../../resources/objects/quad.obj");
    Model bunny("../../resources/objects/bunny_4000.obj");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Screen screen;
    screen.InitScreenBind();
    screenBuffer.Init(width, height);

    RayTracerShader.use();

    std::vector<Triangle> triangles;


#pragma region scene
    // cornell box
    // -----------
    Material cornell_box_white;
    Material cornell_box_red;
    Material cornell_box_green;
    Material cornell_box_light;
    cornell_box_white.baseColor = vec3(0.73, 0.73, 0.73);
    cornell_box_red.baseColor = vec3(0.65, 0.05, 0.05);
    cornell_box_green.baseColor = vec3(0.12, 0.45, 0.15);
    cornell_box_light.baseColor = vec3(1, 1, 1);
    cornell_box_light.emissive = vec3(15, 15, 15);

    // 上
    getTriangle(quad.meshes, triangles, cornell_box_white,
                getTransformMatrix(vec3(0, 0, 0), vec3(0, 5.5, -5.5), vec3(11.1, 0.1, 11.1)), false);

    // 下
    getTriangle(quad.meshes, triangles, cornell_box_white,
                getTransformMatrix(vec3(0, 0, 0), vec3( 0 ,-5.5, -5.5), vec3(11.1, 0.1, 11.1)), false);

    // 后
    getTriangle(quad.meshes, triangles, cornell_box_white,
                getTransformMatrix(vec3(0, 0, 0), vec3( 0 ,0, -11), vec3(11.1, 11.1, 0.2)), false);

    // 左
    getTriangle(quad.meshes, triangles, cornell_box_green,
                getTransformMatrix(vec3(0, 0, 0), vec3( -5.5,0, -5.5), vec3(0.1, 11.1, 11.1)), false);

    // 右
    getTriangle(quad.meshes, triangles, cornell_box_red,
                getTransformMatrix(vec3(0, 0, 0), vec3( 5.5,0, -5.5), vec3(0.1, 11.1, 11.1)), false);

    // 灯
    getTriangle(quad.meshes, triangles, cornell_box_light,
                getTransformMatrix(vec3(0, 0, 0), vec3( 0 ,5.49, -5.5), vec3(2.6, 0.1, 2.1)), false);

    // cube
    getTriangle(quad.meshes, triangles, cornell_box_white,
                getTransformMatrix(vec3(0, 15, 0), vec3(-1.65, -2.2, -7.5), vec3(3.2, 6.6, 3.2)), false);
//    getTriangle(quad.meshes, triangles, cornell_box_white,
//                getTransformMatrix(vec3(0, -18, 0), vec3(1.65, -3.9, -4.65), vec3(3.2, 3.2, 3.2)), false);

    // sphere
    getTriangle(sphere.meshes, triangles, cornell_box_white,
                getTransformMatrix(vec3(0, 0, 0), vec3(1.65, -3.9, -5), vec3(3.2, 3.2, 3.2)), false);

    // bunny
//    getTriangle(bunny.meshes, triangles, cornell_box_white,
//                getTransformMatrix(vec3(0, 0, 0), vec3(2.5, -6.4, -5), vec3(3.2, 3.2, 3.2)), false);

#pragma endregion

    int nTriangles = triangles.size();
    std::cout << "Scene loading completed: " << nTriangles << " triangle faces in total" << std::endl;

    // build bvh node
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

    // 编码三角形数据
    std::vector<Triangle_encoded> triangles_encoded(nTriangles);
    RayTracerShader.setInt("nTriangles", nTriangles);
    for (int i = 0; i < nTriangles; i++) {
        Triangle &t = triangles[i];
        Material &m = t.material;
        // 顶点位置
        triangles_encoded[i].p1 = t.p1;
        triangles_encoded[i].p2 = t.p2;
        triangles_encoded[i].p3 = t.p3;
        // 顶点法线
        triangles_encoded[i].n1 = t.n1;
        triangles_encoded[i].n2 = t.n2;
        triangles_encoded[i].n3 = t.n3;
        // 材质
        triangles_encoded[i].emissive = m.emissive;
        triangles_encoded[i].baseColor = m.baseColor;
        triangles_encoded[i].param1 = vec3(m.subsurface, m.metallic, m.specular);
        triangles_encoded[i].param2 = vec3(m.specularTint, m.roughness, m.anisotropic);
        triangles_encoded[i].param3 = vec3(m.sheen, m.sheenTint, m.clearcoat);
        triangles_encoded[i].param4 = vec3(m.clearcoatGloss, m.IOR, m.transmission);
    }

    // 编码 BVHNode, aabb
    std::vector<BVHNode_encoded> nodes_encoded(nNodes);
    RayTracerShader.setInt("nNodes", nodes.size());
    for (int i = 0; i < nNodes; i++) {
        nodes_encoded[i].childs = vec3(nodes[i].left, nodes[i].right, 0);
        nodes_encoded[i].leafInfo = vec3(nodes[i].n, nodes[i].index, 0);
        nodes_encoded[i].AA = nodes[i].AA;
        nodes_encoded[i].BB = nodes[i].BB;
    }

    // 三角形数组
    GLuint tbo0;
    glGenBuffers(1, &tbo0);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo0);
    glBufferData(GL_TEXTURE_BUFFER, triangles_encoded.size() * sizeof(Triangle_encoded), &triangles_encoded[0],GL_STATIC_DRAW);
    glGenTextures(1, &trianglesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, trianglesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo0);

    // BVHNode 数组
    GLuint tbo1;
    glGenBuffers(1, &tbo1);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo1);
    glBufferData(GL_TEXTURE_BUFFER, nodes_encoded.size() * sizeof(BVHNode_encoded), &nodes_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &nodesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, nodesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo1);

    // HDR 全景图
    HDRLoaderResult hdrRes;
    bool r = HDRLoader::load("../../resources/textures/hdr/circus_arena_4k.hdr", hdrRes);
    hdrMap = getTextureRGB32F(hdrRes.width, hdrRes.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, hdrRes.width, hdrRes.height, 0, GL_RGB, GL_FLOAT, hdrRes.cols);
    // hdrMap = TextureFromFile("circus_arena_4k.hdr", "../../resources/textures/hdr", false, true);

    // 线框模式
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 渲染循环
    // -------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        fps = 1.0f / deltaTime;

        // input
        // -----
        processInput(window);

        // imgui
        // -----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

//        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
//        if (show_demo_window)
//            ImGui::ShowDemoWindow(&show_demo_window);
//
//        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
//        {
//            static float f = 0.0f;
//            static int counter = 0;
//
//            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
//
//            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
//            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
//            ImGui::Checkbox("Another Window", &show_another_window);
//
//            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
//            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
//
//            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
//                counter++;
//            ImGui::SameLine();
//            ImGui::Text("counter = %d", counter);
//
//            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//            ImGui::End();
//        }
//
//        // 3. Show another simple window.
//        if (show_another_window)
//        {
//            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
//            ImGui::Text("Hello from another window!");
//            if (ImGui::Button("Close Me"))
//                show_another_window = false;
//            ImGui::End();
//        }

        // render
        // ------
        camera.LoopIncrease();
//        std::cout << "\r";
//        std::cout << std::fixed << std::setprecision(2) << "FPS : " << fps << "    迭代次数: " << camera.LoopNum;

        {
            screenBuffer.setCurrentBuffer(camera.LoopNum);

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
            screen.DrawScreen();
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ScreenShader.use();
            screenBuffer.setCurrentAsTexture(camera.LoopNum);

            ScreenShader.setInt("screenTexture", 0);
            screen.DrawScreen();

            ImGui::Begin("Scene View");
            ImGui::Text("pointer = %p", 1);
            ImGui::Text("size = %d x %d", width/2, height/2);
            ImGui::Image((void*)(intptr_t)screenBuffer.getCurrentTexture(camera.LoopNum),
                         ImVec2(width/2, height/2), ImVec2(0, 1),ImVec2(1, 0));
            ImGui::End();
        }


       // // view/projection transformations
       // glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, 0.1f,
       //                                         100.0f);
       // glm::mat4 view = camera.GetViewMatrix();
       // ourShader.setMat4("projection", projection);
       // ourShader.setMat4("view", view);
       // float customColor = 0.5f;
       // ourShader.setFloat("customColor", customColor);
       //
       // // render the loaded model
       // glm::mat4 model = glm::mat4(1.0f);
       // model = glm::translate(model,
       //                        glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
       // model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));    // it's a bit too big for our scene, so scale it down
       // ourShader.setMat4("model", model);
       // ourModel.Draw(ourShader);
       //
       // screen.DrawScreen();

        ImGui::Render();
//        int display_w, display_h;
//        glfwGetFramebufferSize(window, &display_w, &display_h);
//        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    screenBuffer.Delete();
    screen.Delete();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
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

    // std::cout << "Camera Position: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << std::endl;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glfwGetFramebufferSize(window, &width, &height);
    camera.ProcessScreenRatio(width, height);
    screenBuffer.Resize(width, height);
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
    // std::cout << "Camera Zoom: " << camera.Zoom << std::endl;
}

