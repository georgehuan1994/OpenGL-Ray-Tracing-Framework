//
// Created by George on 2022/10/29.
//

#ifndef SCENE_H
#define SCENE_H

Material plane;
Material white;
Material jade;
Material golden;
Material copper;
Material glass;
Material brown_glass;
Material tear_glass;
Material tear_glass_emissive;

Material current_material;

GameObject go_sphere;
GameObject go_panther;
GameObject current_game_object;

void InitMaterial();
void InitMesh();
void InitHdrEnvMap();

void InitScene() {

    InitMaterial();

    current_material = copper;
    SetGlobalMaterialProperty(current_material);

    InitMesh();
    current_game_object = go_panther;

    nTriangles = triangles.size();
    std::cout << "Scene loading completed: " << nTriangles << " triangle faces in total" << std::endl;

    InitHdrEnvMap();

}

void InitMaterial() {
    plane.baseColor = vec3(0.73, 0.73, 0.73);
    plane.specular = 1.0;
    plane.IOR = 1.79;
    plane.metallic = 0.2;

    white.baseColor = vec3(0.73, 0.73, 0.73);
    white.roughness = 0.5;
    white.specular = 0.5;

    jade.baseColor = vec3(0.55, 0.78, 0.55);
    jade.specular = 1.0;
    jade.IOR = 1.79;
    jade.subsurface = 1.0;

    golden.baseColor = vec3(0.75, 0.7, 0.15);
    golden.roughness = 0.05;
    golden.specular = 1.0;
    golden.metallic = 1.0;

    copper.baseColor = vec3(238.0f / 255.0f, 158.0f / 255.0f, 137.0f / 255.0f);
    copper.roughness = 0.2;
    copper.specular = 1.0;
    copper.IOR = 1.21901;
    copper.metallic = 1.0;

    glass.baseColor = vec3(1);
    glass.specular = 1.0;
    glass.transmission = 1.0;
    glass.IOR = 1.5;
    glass.roughness = 0.02;

    brown_glass.baseColor = vec3(1);
    brown_glass.mediumType = 1;
    brown_glass.mediumColor = vec3(0.905, 0.63, 0.3);
    brown_glass.mediumDensity = 1; //0.75
    brown_glass.specular = 1.0;
    brown_glass.transmission = 0.957;
    brown_glass.IOR = 1.45;
    brown_glass.roughness = 0.1;

    tear_glass.baseColor = vec3(1);
    tear_glass.mediumColor = vec3(0.085, 0.917, 0.848);
    tear_glass.mediumDensity = 1;
    tear_glass.mediumType = 1;
    tear_glass.specular = 1.0;
    tear_glass.transmission = 0.917;
    tear_glass.IOR = 1.45;

    tear_glass_emissive.baseColor = vec3(1);
    tear_glass_emissive.mediumColor = vec3(0.085, 0.917, 0.848);
    tear_glass_emissive.mediumDensity = 0.25;
    tear_glass_emissive.mediumType = 3;
    tear_glass_emissive.specular = 1.0;
    tear_glass_emissive.transmission = 0.917;
    tear_glass_emissive.IOR = 1.45;
}

void InitMesh() {
    Model floor("../../resources/objects/floor.obj");
    getTriangle(floor.meshes, triangles, plane,
                getTransformMatrix(vec3(0), vec3(2.2, -2, 3), vec3(14, 7, 7)), false);

    // Model bunny("../../resources/objects/bunny_4000.obj");   // 4000 face
    // getTriangle(bunny.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0), vec3(2.2, -2.5, 3), vec3(2)), false);

    // Model teapot("../../resources/objects/renderman/teapot.obj");
    // getTriangle(teapot.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0,0,0), vec3(2.6, -2.0, 3), vec3(2.5)), true);

    // Model sphere("../../resources/objects/sphere2.obj");
    // go_sphere.triangleIndex = getTriangle(sphere.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0, 90, 0), vec3(1.8, -1, 3), vec3(2)), true);

    // Model loong("../../resources/objects/loong.obj");        // 100000 face
    // getTriangle(loong.meshes, triangles, current_material,
    //            getTransformMatrix(vec3(0), vec3(2, -2, 3), vec3(3.5)), true);

    // camera.Rotation = glm::vec3(-90.0f, -14.0f, 0.0f);
    // Model dragon("../../resources/objects/dragon.obj");     // 831812 face
    // getTriangle(dragon.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0, 130, 0), vec3(-0.2, -1.8, 3), vec3(3)), true);

    Model panther("../../resources/objects/panther_100000.obj");     // 100000 face
    go_panther.triangleIndex = getTriangle(panther.meshes, triangles, current_material,
                getTransformMatrix(vec3(0, -30, 0), vec3(0.8, -2.2, 5), vec3(4.5)), true);

    // Model boy_body("../../resources/objects/substance_boy/body.obj");
    // getTriangle(boy_body.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0, -85, 0), vec3(1.8, -1.25, 3.5), vec3(0.8)), true);
    //
    // Model boy_head("../../resources/objects/substance_boy/head.obj");
    // getTriangle(boy_head.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0, -85, 0), vec3(1.8, -0.33, 3.6), vec3(0.8)), true);
}

void InitHdrEnvMap() {
    // HDR Environment Map
    // -------------------
    // bool r = HDRLoader::load("../../resources/textures/hdr/peppermint_powerplant_1k.hdr", hdrRes);
    bool r = HDRLoader::load("../../resources/textures/hdr/peppermint_powerplant_4k.hdr", hdrRes);
    // bool r = HDRLoader::load("../../resources/textures/hdr/sunset.hdr", hdrRes);

    hdrMap = getTextureRGB32F(hdrRes.width, hdrRes.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, hdrRes.width, hdrRes.height, 0, GL_RGB, GL_FLOAT, hdrRes.cols);

    // HDR Important Sampling Cache
    // ----------------------------
    std::cout << "HDR Map Important Sample Cache, HDR Resolution: " << hdrRes.width << " x " << hdrRes.height << std::endl;
    float *cache = calculateHdrCache(hdrRes.cols, hdrRes.width, hdrRes.height);
    hdrCache = getTextureRGB32F(hdrRes.width, hdrRes.height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, hdrRes.width, hdrRes.height, 0, GL_RGB, GL_FLOAT, cache);
    hdrResolution = hdrRes.width;
}

#endif //SCENE_H
