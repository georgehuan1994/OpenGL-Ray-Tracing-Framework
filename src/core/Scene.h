//
// Created by George on 2022/10/29.
//

#ifndef SCENE_H
#define SCENE_H

// Built-in Material
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

// Game Object
GameObject go_floor;
GameObject go_sphere;
GameObject go_bunny;
GameObject go_loong;
GameObject go_panther;

GameObject current_game_object;

void InitMaterial();
void InitMesh();
void InitHdrEnvMap();
void EncodedBVHandTriangles();

void InitScene() {

    InitMaterial();

    current_material = tear_glass;
    SetGlobalMaterialProperty(current_material);

    InitMesh();
    current_game_object = go_loong;

    nTriangles = triangles.size();
    std::cout << "Scene loading completed: " << nTriangles << " triangle faces in total" << std::endl;

    InitHdrEnvMap();

    EncodedBVHandTriangles();
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

    go_floor.active = true;
    go_loong.active = true;

    if(go_floor.active) {
        Model floor("../../resources/objects/floor.obj");
        getTriangle(floor.meshes, triangles, plane,
                    getTransformMatrix(vec3(0), vec3(2.2, -2, 3), vec3(14, 7, 7)), false);
    }

    if (go_bunny.active) {
        Model bunny("../../resources/objects/bunny_4000.obj");   // 4000 face
        getTriangle(bunny.meshes, triangles, current_material,
                    getTransformMatrix(vec3(0), vec3(2.2, -2.5, 3), vec3(2)), false);
    }

    if (go_sphere.active) {
        Model sphere("../../resources/objects/sphere2.obj");
        go_sphere.triangleIndex = getTriangle(sphere.meshes, triangles, current_material,
                    getTransformMatrix(vec3(0, 90, 0), vec3(1.8, -1, 3), vec3(2)), true);
    }

    if (go_loong.active) {
        Model loong("../../resources/objects/loong.obj");        // 100000 face
        go_loong.triangleIndex = getTriangle(loong.meshes, triangles, current_material,
                                             getTransformMatrix(vec3(0), vec3(2, -2, 3), vec3(3.5)), true);
    }

    if (go_panther.active) {
        Model panther("../../resources/objects/panther_100000.obj");     // 100000 face
        go_panther.triangleIndex = getTriangle(panther.meshes, triangles, current_material,
                    getTransformMatrix(vec3(0, -30, 0), vec3(0.8, -2.2, 5), vec3(4.5)), true);
    }

    // Model teapot("../../resources/objects/renderman/teapot.obj");
    // getTriangle(teapot.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0,0,0), vec3(2.6, -2.0, 3), vec3(2.5)), true);

    // camera.Rotation = glm::vec3(-90.0f, -14.0f, 0.0f);
    // Model dragon("../../resources/objects/dragon.obj");     // 831812 face
    // getTriangle(dragon.meshes, triangles, current_material,
    //             getTransformMatrix(vec3(0, 130, 0), vec3(-0.2, -1.8, 3), vec3(3)), true);

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

    const char *peppermint_powerplant_1k = "../../resources/textures/hdr/peppermint_powerplant_1k.hdr";
    const char *peppermint_powerplant_4k = "../../resources/textures/hdr/peppermint_powerplant_4k.hdr";
    const char *sunset_4k = "../../resources/textures/hdr/sunset_4k.hdr";

    HDRLoader::load(peppermint_powerplant_4k, hdrRes);

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

void EncodedBVHandTriangles() {
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

    nodes_prt = &nodes;
    nNodes = nodes.size();
    std::cout << "BVH building completed: " << nNodes << " nodes in total" << std::endl;

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
}

#endif //SCENE_H
