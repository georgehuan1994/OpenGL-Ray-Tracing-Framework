//
// Created by George on 2022/10/29.
//

#ifndef RENDERSETTINGS_H
#define RENDERSETTINGS_H

const unsigned int SCR_WIDTH    = 1024;
const unsigned int SCR_HEIGHT   = 512;

#define RENDER_SCALE            1
#define MAX_BOUNCE              8

GLFWwindow *window = nullptr;
int width, height;

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

// Screen VAO, VBO
Screen screen;

// Screen FBO
RenderBuffer screenBuffer;

// Triangle Count
int nTriangles;

// Triangle Data
std::vector<Triangle> triangles;

// Triangle Texture Buffer Data
GLuint trianglesTextureBuffer;

std::vector<Triangle_encoded> *triangles_encoded_ptr;
std::vector<BVHNode_encoded> *nodes_encoded_ptr;

std::vector<BVHNode> *nodes_prt;

int nNodes;

// BVH Node Texture Buffer Data
GLuint nodesTextureBuffer;

// HDR Map Data
GLuint hdrMap;
GLuint hdrCache;
HDRLoaderResult hdrRes;
int hdrResolution;


GLuint tbo0;
GLuint tbo1;

// Compute Shader Output Image
GLuint tex_output;

// Shader Path
const char *vertexShaderPath                = "../../src/shaders/vertex_shader.glsl";
const char *fragmentShaderRayTracingPath    = "../../src/shaders/fragment_shader_ray_tracing.glsl";
const char *fragmentShaderScreenPath        = "../../src/shaders/fragment_shader_screen.glsl";
const char *fragmentShaderToneMapping       = "../../src/shaders/fragment_shader_tone_mapping.glsl";

// Render Setting
bool    show_demo_window                    = false;
bool    enableMultiImportantSample          = true;
bool    enableEnvMap                        = true;
bool    enableToneMapping                   = true;
bool    enableGammaCorrection               = true;
bool    enableBSDF                          = true;
float   envIntensity                        = 1;
float   envAngle                            = 0; //0.33;
int     maxBounce                           = 8;
int     maxIterations                       = 3000;

#endif //RENDERSETTINGS_H
