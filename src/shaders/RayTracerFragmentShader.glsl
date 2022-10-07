#version 330 core

#define PI              3.1415926
#define INF             114514.0
#define SIZE_TRIANGLE   12
#define SIZE_BVHNODE    4

in vec2 TexCoords;
out vec4 FragColor;

uniform int screenWidth;
uniform int screenHeight;
uniform sampler2D historyTexture;
uniform sampler2D hdrMap;

uniform samplerBuffer triangles;
uniform int nTriangles;

uniform samplerBuffer nodes;
uniform int nNodes;

// 三角面参数
// --------
struct Triangle {
    vec3 p1, p2, p3;
    vec3 n1, n2, n3;
};


// BVH 树节点
// ---------
struct BVHNode {
    int left;           // 左子树
    int right;          // 右子树
    int n;              // 包含三角形数目
    int index;          // 三角形索引
    vec3 AA, BB;        // 碰撞盒
};

// 材质参数
// -------
struct Material {
    vec3 emissive;
    vec3 baseColor;
    float subsurface;
    float metallic;
    float specular;
    float specularTint;
    float roughness;
    float anisotropic;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    float IOR;
    float transmission;
};

// 相机参数，用于构建射线方向
// ---------------------
struct Camera {
    vec3 position;
    vec3 front;
    vec3 right;
    vec3 up;
    float halfH;
    float halfW;
    vec3 leftBottomCorner;
    int loopNum;
};

// 射线参数
// -------
struct Ray {
    vec3 origin;
    vec3 direction;
};

// 碰撞信息
// -------
struct HitRecord {
    bool isHit;             // 是否命中
    bool isInside;          // 是否从内部命中
    float distance;         // 射线距离
    vec3 hitPoint;          // 命中点
    vec3 normal;            // 法线
    vec3 viewDir;           // 视线
    Material material;      // 材质
};

uint wseed;
float rand(void);

// 随机种
// -----
float randcore(uint seed) {
    seed = (seed ^ uint(61)) ^ (seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> uint(4));
    seed *= uint(0x27d4eb2d);
    wseed = seed ^ (seed >> uint(15));
    return float(wseed) * (1.0 / 4294967296.0);
}

// 随机数生成
// --------
float rand() { return randcore(wseed); }

uniform float randOrigin;
uniform Camera camera;

// 获取第 i 下标的三角形
// ------------------
Triangle getTriangle(int i) {

    // vec3 -> 12 字节对齐
    int offset = i * SIZE_TRIANGLE;
    Triangle t;

    // 顶点坐标
    t.p1 = texelFetch(triangles, offset + 0).xyz;
    t.p2 = texelFetch(triangles, offset + 1).xyz;
    t.p3 = texelFetch(triangles, offset + 2).xyz;
    // 法线
    t.n1 = texelFetch(triangles, offset + 3).xyz;
    t.n2 = texelFetch(triangles, offset + 4).xyz;
    t.n3 = texelFetch(triangles, offset + 5).xyz;

    return t;
}

// 获取第 i 个下标的三角形法线
// -----------------------
vec3 getTriangleNormal(Triangle trian) {
    return normalize(cross(trian.p3-trian.p1, trian.p2-trian.p1));
}

// 获取第 i 下标的三角形的材质
// -----------------------
Material getMaterial(int i) {

    Material m;

    int offset = i * SIZE_TRIANGLE;
    vec3 param1 = texelFetch(triangles, offset + 8).xyz;
    vec3 param2 = texelFetch(triangles, offset + 9).xyz;
    vec3 param3 = texelFetch(triangles, offset + 10).xyz;
    vec3 param4 = texelFetch(triangles, offset + 11).xyz;

    m.emissive = texelFetch(triangles, offset + 6).xyz;
    m.baseColor = texelFetch(triangles, offset + 7).xyz;
    m.subsurface = param1.x;
    m.metallic = param1.y;
    m.specular = param1.z;
    m.specularTint = param2.x;
    m.roughness = param2.y;
    m.anisotropic = param2.z;
    m.sheen = param3.x;
    m.sheenTint = param3.y;
    m.clearcoat = param3.z;
    m.clearcoatGloss = param4.x;
    m.IOR = param4.y;
    m.transmission = param4.z;

    return m;
}

// 获取第 i 下标的 BVHNode 对象
// -------------------------
BVHNode getBVHNode(int i) {
    BVHNode node;

    // 左右子树
    int offset = i * SIZE_BVHNODE;
    ivec3 childs = ivec3(texelFetch(nodes, offset + 0).xyz);
    ivec3 leafInfo = ivec3(texelFetch(nodes, offset + 1).xyz);
    node.left = int(childs.x);
    node.right = int(childs.y);
    node.n = int(leafInfo.x);
    node.index = int(leafInfo.y);

    // 包围盒
    node.AA = texelFetch(nodes, offset + 2).xyz;
    node.BB = texelFetch(nodes, offset + 3).xyz;

    return node;
}

// 将三维向量 v 转为 HDR map 的纹理坐标 uv
// -----------------------------------
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

// 获取 HDR 环境颜色
// ---------------
vec3 sampleHdr(vec3 v) {
    vec2 uv = SampleSphericalMap(normalize(v));
    vec3 color = texture(hdrMap, uv).rgb;
//    color = min(color, vec3(10));
    return color;
}

// 半球均匀采样
// ----------
vec3 SampleHemisphere() {
    float z = rand();
    float r = max(0, sqrt(1.0 - z*z));
    float phi = 2.0 * PI * rand();
    return vec3(r * cos(phi), r * sin(phi), z);
}

// 将向量 v 投影到 N 的法向半球
// ------------------------
vec3 toNormalHemisphere(vec3 v, vec3 N) {
    vec3 helper = vec3(1, 0, 0);
    if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
    vec3 tangent = normalize(cross(N, helper));
    vec3 bitangent = normalize(cross(N, tangent));
    return v.x * tangent + v.y * bitangent + v.z * N;
}

// 三角形求交
// --------
HitRecord hitTriangle(Triangle triangle, Ray ray) {
    HitRecord rec;
    rec.distance = INF;
    rec.isHit = false;
    rec.isInside = false;

    vec3 p1 = triangle.p1;
    vec3 p2 = triangle.p2;
    vec3 p3 = triangle.p3;

    vec3 S = ray.origin;        // 射线起点
    vec3 d = ray.direction;     // 射线方向
    vec3 N = normalize(cross(p2-p1, p3-p1));    // 法向量

    // 从三角形背后（模型内部）击中
    if (dot(N, d) > 0.0f) {
        N = -N;
        rec.isInside = true;
    }

    // 如果视线和三角形平行
    if (abs(dot(N, d)) < 0.00001f) return rec;

    // 距离
    float t = (dot(N, p1) - dot(S, N)) / dot(d, N);
    if (t < 0.0005f) return rec;    // 如果三角形在光线背面

    // 交点计算
    vec3 P = S + d * t;

    // 判断交点是否在三角形中
    vec3 c1 = cross(p2 - p1, P - p1);
    vec3 c2 = cross(p3 - p2, P - p2);
    vec3 c3 = cross(p1 - p3, P - p3);
    bool r1 = (dot(c1, N) > 0 && dot(c2, N) > 0 && dot(c3, N) > 0);
    bool r2 = (dot(c1, N) < 0 && dot(c2, N) < 0 && dot(c3, N) < 0);

    // 命中，封装返回结果
    if (r1 || r2) {
        rec.isHit = true;
        rec.hitPoint = P;
        rec.distance = t - 0.00001;
        rec.normal = N;
        rec.viewDir = d;
        // 根据交点位置插值顶点法线
        float alpha = (-(P.x-p2.x)*(p3.y-p2.y) + (P.y-p2.y)*(p3.x-p2.x)) / (-(p1.x-p2.x-0.00005)*(p3.y-p2.y+0.00005) + (p1.y-p2.y+0.00005)*(p3.x-p2.x+0.00005));
        float beta  = (-(P.x-p3.x)*(p1.y-p3.y) + (P.y-p3.y)*(p1.x-p3.x)) / (-(p2.x-p3.x-0.00005)*(p1.y-p3.y+0.00005) + (p2.y-p3.y+0.00005)*(p1.x-p3.x+0.00005));
        float gama  = 1.0 - alpha - beta;
        vec3 Nsmooth = alpha * triangle.n1 + beta * triangle.n2 + gama * triangle.n3;
        Nsmooth = normalize(Nsmooth);
        rec.normal = (rec.isInside) ? (-Nsmooth) : (Nsmooth);
    }

    return rec;
}

// 和 aabb 盒子求交，没有交点则返回 -1
// ------------------------------
float hitAABB(Ray r, vec3 AA, vec3 BB) {
    vec3 invdir = 1.0 / r.direction;

    vec3 f = (BB - r.origin) * invdir;
    vec3 n = (AA - r.origin) * invdir;

    vec3 tmax = max(f, n);
    vec3 tmin = min(f, n);

    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    float t0 = max(tmin.x, max(tmin.y, tmin.z));

    return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
}

// 暴力求交
// -------
HitRecord hitArray(Ray ray, int l, int r) {
    HitRecord rec;
    rec.isHit = false;
    rec.distance = INF;
    for(int i = l; i <= r; i++) {
        Triangle triangle = getTriangle(i);
        HitRecord r = hitTriangle(triangle, ray);
        if(r.isHit && r.distance < rec.distance) {
            rec = r;
            rec.material = getMaterial(i);
        }
    }
    return rec;
}

// 遍历 BVH 求交
// ------------
HitRecord hitBVH(Ray ray) {
    HitRecord rec;
    rec.isHit = false;
    rec.distance = INF;

    // 栈
    int stack[256];
    int sp = 0;

    stack[sp++] = 1;
    while(sp>0) {
        int top = stack[--sp];
        BVHNode node = getBVHNode(top);

        // 是叶子节点，遍历三角形，求最近交点
        if(node.n>0) {
            int L = node.index;
            int R = node.index + node.n - 1;
            HitRecord r = hitArray(ray, L, R);
            if(r.isHit && r.distance<rec.distance) rec = r;
            continue;
        }

        // 和左右盒子 AABB 求交
        float d1 = INF; // 左盒子距离
        float d2 = INF; // 右盒子距离
        if(node.left>0) {
            BVHNode leftNode = getBVHNode(node.left);
            d1 = hitAABB(ray, leftNode.AA, leftNode.BB);
        }
        if(node.right>0) {
            BVHNode rightNode = getBVHNode(node.right);
            d2 = hitAABB(ray, rightNode.AA, rightNode.BB);
        }

        // 在最近的盒子中搜索
        if(d1>0 && d2>0) {
            if(d1<d2) { // d1<d2, 左边先
                stack[sp++] = node.right;
                stack[sp++] = node.left;
            } else {    // d2<d1, 右边先
                stack[sp++] = node.left;
                stack[sp++] = node.right;
            }
        } else if(d1>0) {   // 仅命中左边
            stack[sp++] = node.left;
        } else if(d2>0) {   // 仅命中右边
            stack[sp++] = node.right;
        }
    }

    return rec;
}

// 路径追踪着色
// ----------
vec3 shading(HitRecord hit) {

    vec3 Lo = vec3(0);
    vec3 history = vec3(1);

    for (int i = 0; i < 2; i++) {
        // 随机出射方向 wi
        vec3 wi = toNormalHemisphere(SampleHemisphere(), hit.normal);

        // 漫反射: 随机发射光线
        Ray randomRay;
        randomRay.origin = hit.hitPoint;
        randomRay.direction = wi;
//        HitRecord newHit = hitArray(randomRay, 0, nTriangles - 1);
        HitRecord newHit = hitBVH(randomRay);

        float pdf = 1.0 / (2.0 * PI);                                   // 半球均匀采样概率密度
        float cosine_o = max(0, dot(-hit.viewDir, hit.normal));         // 入射光和法线夹角余弦
        float cosine_i = max(0, dot(randomRay.direction, hit.normal));  // 出射光和法线夹角余弦
        vec3 f_r = hit.material.baseColor / PI;                         // 漫反射 BRDF

        // 未命中
        if(!newHit.isHit) {
//            vec3 skyColor = vec3(0);
            vec3 skyColor = sampleHdr(randomRay.direction);
            Lo += history * skyColor * f_r * cosine_i / pdf;
            break;
        }

        // 命中光源积累颜色
        vec3 Le = newHit.material.emissive;
        Lo += history * Le * f_r * cosine_i / pdf;

        // 递归(步进)
        hit = newHit;
        history *= f_r * cosine_i / pdf;  // 累积颜色
    }
    return Lo;
}


void main() {

    wseed = uint(randOrigin * float(6.95857) * (TexCoords.x * TexCoords.y));
    vec3 hist = texture(historyTexture, TexCoords).rgb;

    Ray cameraRay;
    cameraRay.origin = camera.position;
    cameraRay.direction = normalize(camera.leftBottomCorner + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);

    //    BVHNode node = getBVHNode(1);
    //    BVHNode left = getBVHNode(node.left);
    //    BVHNode right = getBVHNode(node.right);
    //
    //    float r1 = hitAABB(cameraRay, left.AA, left.BB);
    //    float r2 = hitAABB(cameraRay, right.AA, right.BB);

    ////    vec3 color;
    //    if(r1>0) FragColor = vec4(1, 0, 0, 1);
    //    if(r2>0) FragColor = vec4(0, 1, 0, 1);
    //    if(r1>0 && r2>0) FragColor = vec4(1, 1, 0, 1);

    //        for(int i = 0; i < nNodes; i++) {
    //            BVHNode node = getBVHNode(i);
    //            if(node.n > 0) {
    //                int L = node.index;
    //                int R = node.index + node.n - 1;
    //                HitRecord res = hitArray(cameraRay, L, R);
    //                if(res.isHit) FragColor = vec4(res.material.baseColor, 1);
    //            }
    //        }

    //    HitRecord firstHit = hitArray(cameraRay, 0, nTriangles - 1);
    HitRecord firstHit = hitBVH(cameraRay);

    vec3 curColor = vec3(0);

    if(!firstHit.isHit) {
        curColor = vec3(0);
//        float t = 0.5 * (cameraRay.direction.y + 1.0);
//        curColor = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
        curColor = sampleHdr(cameraRay.direction);
    } else {
        vec3 Le = firstHit.material.emissive;
        vec3 Li = shading(firstHit);
        curColor = Le + Li;
    }

    curColor = (1.0 / float(camera.loopNum)) * curColor + (float(camera.loopNum - 1) / float(camera.loopNum)) * hist;
    FragColor = vec4(curColor, 1.0);

}