
// ============== define ===============

#define PI              3.14159265358979323
#define INV_PI          0.31830988618379067
#define TWO_PI          6.28318530717958648
#define INV_TWO_PI      0.15915494309189533
#define INV_4_PI        0.07957747154594766
#define EPS             0.0001
#define INF             114514.0

#define SIZE_TRIANGLE   14
#define SIZE_BVHNODE    4

#define MEDIUM_NONE 0
#define MEDIUM_ABSORB 1
#define MEDIUM_SCATTER 2
#define MEDIUM_EMISSIVE 3

// ============== struct ===============

// triangle data
// -------------
struct Triangle {
    vec3 p1, p2, p3;    // position
    vec3 n1, n2, n3;    // position normal
};

// BVH node
// ---------
struct BVHNode {
    int left;           // left child tree
    int right;          // right child tree
    int n;              // triangle count
    int index;          // triangle index
    vec3 AA, BB;        // AABB Min and Max position
};

struct Medium           // TODO: Add Medium Params
{
    int     type;       // 0:NONE, 1:ABSORB, 2:SCATTER, 3:EMISSIVE
    float   density;    // 0-1
    vec3    color;
    float   anisotropy;
};

// material params
// ---------------
struct Material {
    vec3    emissive;
    vec3    baseColor;
    float   subsurface;
    float   metallic;
    float   specular;
    float   specularTint;
    float   roughness;
    float   anisotropic;
    float   sheen;
    float   sheenTint;
    float   clearcoat;
    float   clearcoatGloss;
    float   IOR;
    float   transmission;       // specTrans

    float   ax;
    float   ay;

    Medium medium;
};

// camera params
// -------------
struct Camera {
    int     loopNum;            // iterations
    vec3    position;
    vec3    front;
    vec3    right;
    vec3    up;
    vec3    leftBottomCorner;
    float   halfH;
    float   halfW;
};

// ray params
// ----------
struct Ray {
    vec3    origin;
    vec3    direction;
};

// hit info
// --------
struct HitRecord {
    bool    isHit;
    bool    isInside;
    vec3    hitPoint;
    vec3    normal;            // hit point normal
    vec3    viewDir;
    float   distance;
    Material material;
};

// ============== uniform ===============

in vec2 TexCoords;
out vec4 FragColor;

uniform Camera camera;

uniform int screenWidth;
uniform int screenHeight;
uniform int hdrResolution;

uniform sampler2D historyTexture;
uniform sampler2D hdrMap;
uniform sampler2D hdrCache;         // R:u, G:v, B:pdf(u, v)

uniform samplerBuffer triangles;    // triangle data
uniform int nTriangles;

uniform samplerBuffer nodes;        // bvh data
uniform int nNodes;

uniform float randOrigin;

uniform bool enableMultiImportantSample;
uniform bool enableEnvMap;
uniform bool enableBSDF;

uniform float envIntensity;
uniform float envAngle;

uniform int maxBounce;
uniform int maxIterations;

// ============== function ===============

float sqr(float x) { return x*x; }

// Luminance
// ---------
float Luminance(vec3 c)
{
    return 0.212671 * c.x + 0.715160 * c.y + 0.072169 * c.z;
}

// Get triangle with index i
// -------------------------
Triangle getTriangle(int i) {

    // vec3 -> 12 byte
    int offset = i * SIZE_TRIANGLE;
    Triangle t;

    t.p1 = texelFetch(triangles, offset + 0).xyz;
    t.p2 = texelFetch(triangles, offset + 1).xyz;
    t.p3 = texelFetch(triangles, offset + 2).xyz;

    t.n1 = texelFetch(triangles, offset + 3).xyz;
    t.n2 = texelFetch(triangles, offset + 4).xyz;
    t.n3 = texelFetch(triangles, offset + 5).xyz;

    return t;
}

// Get triangle normal
// -------------------
vec3 getTriangleNormal(Triangle trian) {
    return normalize(cross(trian.p3 - trian.p1, trian.p2 - trian.p1));
}

// Get triangle material with index i
// ----------------------------------
Material getMaterial(int i) {

    Material m;

    int offset          = i * SIZE_TRIANGLE;
    vec3 param1         = texelFetch(triangles, offset + 8).xyz;
    vec3 param2         = texelFetch(triangles, offset + 9).xyz;
    vec3 param3         = texelFetch(triangles, offset + 10).xyz;
    vec3 param4         = texelFetch(triangles, offset + 11).xyz;
    vec3 param5         = texelFetch(triangles, offset + 13).xyz;

    m.emissive          = texelFetch(triangles, offset + 6).xyz;
    m.baseColor         = texelFetch(triangles, offset + 7).xyz;
    m.medium.color      = texelFetch(triangles, offset + 12).xyz;

    m.subsurface        = param1.x;
    m.metallic          = param1.y;
    m.specular          = param1.z;

    m.specularTint      = param2.x;
    m.roughness         = param2.y;
    m.anisotropic       = param2.z;

    m.sheen             = param3.x;
    m.sheenTint         = param3.y;
    m.clearcoat         = param3.z;

    m.clearcoatGloss    = param4.x;
    m.IOR               = param4.y;
    m.transmission      = param4.z;

    float aspect        = sqrt(1.0 - m.anisotropic * 0.9);
    m.ax                = max(0.001, sqr(m.roughness) / aspect);
    m.ay                = max(0.001, sqr(m.roughness) * aspect);

    m.medium.type       = int(param5.x);
    m.medium.density    = param5.y;
    m.medium.anisotropy = param5.z;

    return m;
}

// Get bvh node with index i
// -------------------------
BVHNode getBVHNode(int i) {
    BVHNode node;

    // left and right child node
    int offset      = i * SIZE_BVHNODE;

    ivec3 childs    = ivec3(texelFetch(nodes, offset + 0).xyz);
    ivec3 leafInfo  = ivec3(texelFetch(nodes, offset + 1).xyz);

    node.left       = int(childs.x);
    node.right      = int(childs.y);
    node.n          = int(leafInfo.x);
    node.index      = int(leafInfo.y);

    // aabb
    node.AA = texelFetch(nodes, offset + 2).xyz;
    node.BB = texelFetch(nodes, offset + 3).xyz;

    return node;
}

// triangle intersection
// ---------------------
HitRecord hitTriangle(Triangle triangle, Ray ray) {
    HitRecord rec;
    rec.distance    = INF;
    rec.isHit       = false;
    rec.isInside    = false;

    vec3 p1 = triangle.p1;
    vec3 p2 = triangle.p2;
    vec3 p3 = triangle.p3;

    vec3 S = ray.origin;
    vec3 d = ray.direction;
    vec3 N = normalize(cross(p2-p1, p3-p1));

    // hit from behind the triangle (inside the model)
    if (dot(N, d) > 0.0) {
        N = -N;
        rec.isInside = true;
    }

    // if the line of sight is parallel to the triangle
    if (abs(dot(N, d)) < 0.00001) return rec;

    // distance
    float t = (dot(N, p1) - dot(S, N)) / dot(d, N);

    // if the triangle is on the back of the ray
    if (t < 0.0005) return rec;

    // hit point in the plane of triangle
    vec3 P = S + d * t;

    // is hit point in the triangle?
    vec3 c1 = cross(p2 - p1, P - p1);
    vec3 c2 = cross(p3 - p2, P - p2);
    vec3 c3 = cross(p1 - p3, P - p3);

    bool r1 = (dot(c1, N) > 0 && dot(c2, N) > 0 && dot(c3, N) > 0);
    bool r2 = (dot(c1, N) < 0 && dot(c2, N) < 0 && dot(c3, N) < 0);

    if (r1 || r2) {
        rec.isHit       = true;
        rec.hitPoint    = P;
        rec.distance    = t - 0.00001;
        rec.viewDir     = d;
        rec.normal      = N;

        // smooth normal
        float   alpha   = (-(P.x-p2.x)*(p3.y-p2.y) + (P.y-p2.y)*(p3.x-p2.x)) / (-(p1.x-p2.x)*(p3.y-p2.y) + (p1.y-p2.y)*(p3.x-p2.x)+1e-7);
        float   beta    = (-(P.x-p3.x)*(p1.y-p3.y) + (P.y-p3.y)*(p1.x-p3.x)) / (-(p2.x-p3.x)*(p1.y-p3.y) + (p2.y-p3.y)*(p1.x-p3.x)+1e-7);
        float   gama    = 1.0 - alpha - beta;
        vec3    Nsmooth = alpha * triangle.n1 + beta * triangle.n2 + gama * triangle.n3;
                Nsmooth = normalize(Nsmooth);

        rec.normal = (rec.isInside) ? (-Nsmooth) : (Nsmooth);
    }

    return rec;
}

// intersect with aabb box, if there is no intersection return - 1
// ---------------------------------------------------------------
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

// Violence Seeks Intersection
// ---------------------------
HitRecord hitArray(Ray ray, int l, int r) {
    HitRecord rec;
    rec.isHit       = false;
    rec.distance    = INF;

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

// Traversal BVH Seeks intersection
// --------------------------------
HitRecord hitBVH(Ray ray) {
    HitRecord rec;
    rec.isHit       = false;
    rec.distance    = INF;

    int stack[256];
    int sp = 0;

    stack[sp++] = 1;
    while(sp > 0) {
        int top = stack[--sp];
        BVHNode node = getBVHNode(top);

        // 是叶子节点，遍历三角形，求最近交点
        if(node.n > 0) {
            int L = node.index;
            int R = node.index + node.n - 1;
            HitRecord r = hitArray(ray, L, R);
            if(r.isHit && r.distance < rec.distance) rec = r;
            continue;
        }

        // 和左右盒子 AABB 求交
        float d1 = INF; // 左盒子距离
        float d2 = INF; // 右盒子距离
        if(node.left > 0) {
            BVHNode leftNode = getBVHNode(node.left);
            d1 = hitAABB(ray, leftNode.AA, leftNode.BB);
        }
        if(node.right > 0) {
            BVHNode rightNode = getBVHNode(node.right);
            d2 = hitAABB(ray, rightNode.AA, rightNode.BB);
        }

        // 在最近的盒子中搜索
        if(d1 > 0 && d2 > 0) {
            if(d1 < d2) { // d1<d2, 左边先
                stack[sp++] = node.right;
                stack[sp++] = node.left;
            }
            else {    // d2<d1, 右边先
                stack[sp++] = node.left;
                stack[sp++] = node.right;
            }
        }
        else if(d1 > 0) {   // 仅命中左边
            stack[sp++] = node.left;
        }
        else if(d2 > 0) {   // 仅命中右边
            stack[sp++] = node.right;
        }
    }

    return rec;
}

// get tangent and bitangent
// -------------------------
void getTangent(vec3 N, inout vec3 tangent, inout vec3 bitangent) {
    /*
    vec3 helper = vec3(0, 0, 1);
    if(abs(N.z) > 0.999) helper = vec3(0, -1, 0);
    tangent = normalize(cross(N, helper));
    bitangent = normalize(cross(N, tangent));
    */
    vec3 helper = vec3(1, 0, 0);
    if(abs(N.x) > 0.999) helper = vec3(0, 0, 1);
    bitangent = normalize(cross(N, helper));
    tangent = normalize(cross(N, bitangent));
}

// calculate tint from basecolor
vec3 CalculateTint(vec3 baseColor)
{
    // -- The color tint is never mentioned in the SIGGRAPH presentations as far as I recall but it was done in
    // --  the BRDF Explorer so I'll replicate that here.
    float luminance = Luminance(baseColor);
    return (luminance > 0.0) ? baseColor * (1.0 / luminance) : vec3(1.0);
}

// calculate specularcolor from baseColor/specularTint/metallic
// calculate sheen color color form baseColor/sheenTint
void GetSpecColor(Material mat, float eta, out vec3 specCol, out vec3 sheenCol)
{
    float luminance = Luminance(mat.baseColor);
    vec3 ctint = luminance > 0.0 ? mat.baseColor / luminance : vec3(1.0);
    float F0 = (1.0 - eta) / (1.0 + eta);
    specCol = mix(F0 * F0 * mix(vec3(1.0), ctint, mat.specularTint), mat.baseColor, mat.metallic);
    sheenCol = mix(vec3(1.0), ctint, mat.sheenTint);
}

// Normal Distribution: Generalized-Trowbridge-Reitz, γ=1, Berry
// -------------------------------------------------------------
float GTR1(float NdotH, float alpha) {
    if (alpha >= 1) return INV_PI;
    float a2 = alpha * alpha;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return (a2 - 1) / (PI * log(a2) * t);
}

// Normal Distribution: Generalized-Trowbridge-Reitz, γ=2, GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
// -------------------------------------------------------------
float GTR2(float NdotH, float alpha) {
    float a2 = alpha * alpha;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return a2 / (PI * t * t);
}

float GTR2_Aniso(float NdotH, float HdotX, float HdotY, float ax, float ay) {
    float a = HdotX / ax;
    float b = HdotY / ay;
    float c = a * a + b * b + NdotH * NdotH;
    return 1.0 / (PI * ax * ay * c * c);
}

// Geometry
// --------
float SmithG_GGX(float NdotV, float alphaG) {
    float a = alphaG * alphaG;
    float b = NdotV * NdotV;
    return (2.0 * NdotV) / (NdotV + sqrt(a + b - a * b));
    return 1.0           / (NdotV + sqrt(a + b - a * b));
}

// Geometry
// --------
float SmithG_GGX_Aniso(float NdotV, float VdotX, float VdotY, float ax, float ay) {
    float a = VdotX * ax;
    float b = VdotY * ay;
    float c = NdotV;
    return (2.0 * NdotV) / (NdotV + sqrt(a * a + b * b + c * c));
    return 1.0           / (NdotV + sqrt(a * a + b * b + c * c));
}

// Schlick Fresnel
// ---------------
float SchlickFresnel(float u) {
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

// Dielectric Fresnel
// ------------------
float DielectricFresnel(float cosThetaI, float eta)
{
    float sinThetaTSq = eta * eta * (1.0f - cosThetaI * cosThetaI);

    // Total internal reflection
    if (sinThetaTSq > 1.0)
    return 1.0;

    float cosThetaT = sqrt(max(1.0 - sinThetaTSq, 0.0));

    float rs = (eta * cosThetaT - cosThetaI) / (eta * cosThetaT + cosThetaI);
    float rp = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);

    return 0.5 * (rs * rs + rp * rp);
}

// mix schlick fresnel and dielectric fresnel
// ------------------------------------------
float DisneyFresnel(Material mat, float eta, float LDotH, float VDotH)
{
    float metallicFresnel = SchlickFresnel(LDotH);
    float dielectricFresnel = DielectricFresnel(abs(VDotH), eta);
    return mix(dielectricFresnel, metallicFresnel, mat.metallic);
}

vec3 ToWorld(vec3 X, vec3 Y, vec3 Z, vec3 V)
{
    return V.x * X + V.y * Y + V.z * Z;
}

vec3 ToLocal(vec3 X, vec3 Y, vec3 Z, vec3 V)
{
    return vec3(dot(V, X), dot(V, Y), dot(V, Z));
}

// BRDF each item's pdf
// --------------------
void CalculateBRDFLobePdfs(Material material, out float pDiffuse, out float pSpecular, out float pClearcoat) {

    // radiance
    float r_diffuse     = (1.0 - material.metallic);
    float r_specular    = (1.0 - material.metallic) + material.metallic;
    float r_clearcoat   = (1.0 - material.metallic) * 0.25 * material.clearcoat;

    float r_sum_inv     = 1.0 / (r_diffuse + r_specular + r_clearcoat); // + r_refraction

    // pdf
    pDiffuse    = r_diffuse * r_sum_inv;
    pSpecular   = r_specular * r_sum_inv;
    pClearcoat  = r_clearcoat * r_sum_inv;
}

// BSDF each item's pdf with fresnel
// ---------------------------------
void CalculateBSDFLobePdfs(Material material, float eta, vec3 specCol, float approxFresnel, out float diffuseWeight, out float specReflectWt, out float specRefractWt, out float clearcoatWt)
{
    float r_diffuse      = (1.0 - material.metallic) * (1.0 - material.transmission) * Luminance(material.baseColor);
    float r_specular     = Luminance(mix(specCol, vec3(1.0), approxFresnel));
    float r_clearcoat    = (1.0 - material.metallic) * 0.25 * material.clearcoat;
    float r_refraction   = (1.0 - material.metallic) * material.transmission * Luminance(material.baseColor) * (1.0 - approxFresnel);

    float r_sum_inv = 1.0 / (r_diffuse + r_specular + r_clearcoat + r_refraction);

    diffuseWeight   = r_diffuse * r_sum_inv;
    specReflectWt   = r_specular * r_sum_inv;
    clearcoatWt     = r_clearcoat * r_sum_inv;
    specRefractWt   = r_refraction * r_sum_inv;
}

// BSDF each item's pdf without fresnel
// ------------------------------------
void CalculateBSDFLobePdfs(Material material, out float pSpecular, out float pDiffuse, out float pClearcoat, out float pSpecTrans)
{
    float metallicBRDF   = material.metallic;
    float specularBSDF   = (1.0 - material.metallic) * material.transmission;
    float dielectricBRDF = (1.0 - material.metallic) * (1.0 - material.transmission);

    float diffuseWeight      = dielectricBRDF;
    float specularWeight     = metallicBRDF + dielectricBRDF;
    float transmissionWeight = specularBSDF;
    float clearcoatWeight    = 1.0 * max(0, material.clearcoat);

    float norm = 1.0 / (specularWeight + transmissionWeight + diffuseWeight + clearcoatWeight);

    pSpecular  = specularWeight     * norm;
    pSpecTrans = transmissionWeight * norm;
    pDiffuse   = diffuseWeight      * norm;
    pClearcoat = clearcoatWeight    * norm;
}

uint wseed;

// Thomas Wang hash
// ----------------
float randcore(uint seed) {
    seed = (seed ^ uint(61)) ^ (seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> uint(4));
    seed *= uint(0x27d4eb2d);
    wseed = seed ^ (seed >> uint(15));
    return float(wseed) * (1.0 / 4294967296.0);
}

float rand() { return randcore(wseed); }

// 1 ~ 8 dimensional sobol matrix
// https://wiki.blender.org/wiki/Source/Render/Cycles/SamplingPatterns
const uint V[8*32] = uint[8*32](
2147483648u, 1073741824u, 536870912u, 268435456u, 134217728u, 67108864u, 33554432u, 16777216u, 8388608u, 4194304u, 2097152u, 1048576u, 524288u, 262144u, 131072u, 65536u, 32768u, 16384u, 8192u, 4096u, 2048u, 1024u, 512u, 256u, 128u, 64u, 32u, 16u, 8u, 4u, 2u, 1u, 2147483648u, 3221225472u, 2684354560u, 4026531840u, 2281701376u, 3422552064u, 2852126720u, 4278190080u, 2155872256u, 3233808384u, 2694840320u, 4042260480u, 2290614272u, 3435921408u, 2863267840u, 4294901760u, 2147516416u, 3221274624u, 2684395520u, 4026593280u, 2281736192u, 3422604288u, 2852170240u, 4278255360u, 2155905152u, 3233857728u, 2694881440u, 4042322160u, 2290649224u, 3435973836u, 2863311530u, 4294967295u, 2147483648u, 3221225472u, 1610612736u, 2415919104u, 3892314112u, 1543503872u, 2382364672u, 3305111552u, 1753219072u, 2629828608u, 3999268864u, 1435500544u, 2154299392u, 3231449088u, 1626210304u, 2421489664u, 3900735488u, 1556135936u, 2388680704u, 3314585600u, 1751705600u, 2627492864u, 4008611328u, 1431684352u, 2147543168u, 3221249216u, 1610649184u, 2415969680u, 3892340840u, 1543543964u, 2382425838u, 3305133397u, 2147483648u, 3221225472u, 536870912u, 1342177280u, 4160749568u, 1946157056u, 2717908992u, 2466250752u, 3632267264u, 624951296u, 1507852288u, 3872391168u, 2013790208u, 3020685312u, 2181169152u, 3271884800u, 546275328u, 1363623936u, 4226424832u, 1977167872u, 2693105664u, 2437829632u, 3689389568u, 635137280u, 1484783744u, 3846176960u, 2044723232u, 3067084880u, 2148008184u, 3222012020u, 537002146u, 1342505107u, 2147483648u, 1073741824u, 536870912u, 2952790016u, 4160749568u, 3690987520u, 2046820352u, 2634022912u, 1518338048u, 801112064u, 2707423232u, 4038066176u, 3666345984u, 1875116032u, 2170683392u, 1085997056u, 579305472u, 3016343552u, 4217741312u, 3719483392u, 2013407232u, 2617981952u, 1510979072u, 755882752u, 2726789248u, 4090085440u, 3680870432u, 1840435376u, 2147625208u, 1074478300u, 537900666u, 2953698205u, 2147483648u, 1073741824u, 1610612736u, 805306368u, 2818572288u, 335544320u, 2113929216u, 3472883712u, 2290089984u, 3829399552u, 3059744768u, 1127219200u, 3089629184u, 4199809024u, 3567124480u, 1891565568u, 394297344u, 3988799488u, 920674304u, 4193267712u, 2950604800u, 3977188352u, 3250028032u, 129093376u, 2231568512u, 2963678272u, 4281226848u, 432124720u, 803643432u, 1633613396u, 2672665246u, 3170194367u, 2147483648u, 3221225472u, 2684354560u, 3489660928u, 1476395008u, 2483027968u, 1040187392u, 3808428032u, 3196059648u, 599785472u, 505413632u, 4077912064u, 1182269440u, 1736704000u, 2017853440u, 2221342720u, 3329785856u, 2810494976u, 3628507136u, 1416089600u, 2658719744u, 864310272u, 3863387648u, 3076993792u, 553150080u, 272922560u, 4167467040u, 1148698640u, 1719673080u, 2009075780u, 2149644390u, 3222291575u, 2147483648u, 1073741824u, 2684354560u, 1342177280u, 2281701376u, 1946157056u, 436207616u, 2566914048u, 2625634304u, 3208642560u, 2720006144u, 2098200576u, 111673344u, 2354315264u, 3464626176u, 4027383808u, 2886631424u, 3770826752u, 1691164672u, 3357462528u, 1993345024u, 3752330240u, 873073152u, 2870150400u, 1700563072u, 87021376u, 1097028000u, 1222351248u, 1560027592u, 2977959924u, 23268898u, 437609937u
);
// const int V[8*32] = int[8*32](
// 2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216, 8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1, 2147483648, 3221225472, 2684354560, 4026531840, 2281701376, 3422552064, 2852126720, 4278190080, 2155872256, 3233808384, 2694840320, 4042260480, 2290614272, 3435921408, 2863267840, 4294901760, 2147516416, 3221274624, 2684395520, 4026593280, 2281736192, 3422604288, 2852170240, 4278255360, 2155905152, 3233857728, 2694881440, 4042322160, 2290649224, 3435973836, 2863311530, 4294967295, 2147483648, 3221225472, 1610612736, 2415919104, 3892314112, 1543503872, 2382364672, 3305111552, 1753219072, 2629828608, 3999268864, 1435500544, 2154299392, 3231449088, 1626210304, 2421489664, 3900735488, 1556135936, 2388680704, 3314585600, 1751705600, 2627492864, 4008611328, 1431684352, 2147543168, 3221249216, 1610649184, 2415969680, 3892340840, 1543543964, 2382425838, 3305133397, 2147483648, 3221225472, 536870912, 1342177280, 4160749568, 1946157056, 2717908992, 2466250752, 3632267264, 624951296, 1507852288, 3872391168, 2013790208, 3020685312, 2181169152, 3271884800, 546275328, 1363623936, 4226424832, 1977167872, 2693105664, 2437829632, 3689389568, 635137280, 1484783744, 3846176960, 2044723232, 3067084880, 2148008184, 3222012020, 537002146, 1342505107, 2147483648, 1073741824, 536870912, 2952790016, 4160749568, 3690987520, 2046820352, 2634022912, 1518338048, 801112064, 2707423232, 4038066176, 3666345984, 1875116032, 2170683392, 1085997056, 579305472, 3016343552, 4217741312, 3719483392, 2013407232, 2617981952, 1510979072, 755882752, 2726789248, 4090085440, 3680870432, 1840435376, 2147625208, 1074478300, 537900666, 2953698205, 2147483648, 1073741824, 1610612736, 805306368, 2818572288, 335544320, 2113929216, 3472883712, 2290089984, 3829399552, 3059744768, 1127219200, 3089629184, 4199809024, 3567124480, 1891565568, 394297344, 3988799488, 920674304, 4193267712, 2950604800, 3977188352, 3250028032, 129093376, 2231568512, 2963678272, 4281226848, 432124720, 803643432, 1633613396, 2672665246, 3170194367, 2147483648, 3221225472, 2684354560, 3489660928, 1476395008, 2483027968, 1040187392, 3808428032, 3196059648, 599785472, 505413632, 4077912064, 1182269440, 1736704000, 2017853440, 2221342720, 3329785856, 2810494976, 3628507136, 1416089600, 2658719744, 864310272, 3863387648, 3076993792, 553150080, 272922560, 4167467040, 1148698640, 1719673080, 2009075780, 2149644390, 3222291575, 2147483648, 1073741824, 2684354560, 1342177280, 2281701376, 1946157056, 436207616, 2566914048, 2625634304, 3208642560, 2720006144, 2098200576, 111673344, 2354315264, 3464626176, 4027383808, 2886631424, 3770826752, 1691164672, 3357462528, 1993345024, 3752330240, 873073152, 2870150400, 1700563072, 87021376, 1097028000, 1222351248, 1560027592, 2977959924, 23268898, 437609937
// );


int grayCode(int i) {
    return i ^ (i>>1);
}

// Generate d Dimensional, i Sobol Number
// --------------------------------------
float sobol(int d, int i) {
    uint result = 0u;
    int offset = d * 32;
    for (int j = 0; i != 0; i >>= 1, j++)
    if ((i & 1) != 0)
    result ^= V[j+offset];

    return float(result) * (1.0f / float(0xFFFFFFFFU));
}

// generate random vec2 for the b-th bounce of the i frame
// 生成第 i 帧的第 b 次反弹需要的二维随机向量
vec2 sobolVec2(int i, int b) {
    float u = sobol(b * 2, grayCode(i));
    float v = sobol(b * 2 + 1, grayCode(i));
    return vec2(u, v);
}


// 将三维向量 v 转为 HDR map 的纹理坐标 uv
// -----------------------------------
vec2 toSphericalCoord(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv + vec2(envAngle, 0);
}

// 采样预计算的 HDR cache
// --------------------
vec3 SampleHdr(float xi_1, float xi_2) {
    vec2 xy = texture(hdrCache, vec2(xi_1, xi_2)).rg; // x, y
    xy.y = 1.0 - xy.y; // flip y

    // 获取角度
    float phi = 2.0 * PI * (xy.x - 0.5);    // [-pi ~ pi]
    float theta = PI * (xy.y - 0.5);        // [-pi/2 ~ pi/2]

    // 出射方向：球坐标计算方向
    vec3 L = vec3(cos(theta) * cos(phi), sin(theta), cos(theta) * sin(phi));
    return L;
}

// 半球均匀采样
// ----------
vec3 CosineSampleHemisphere(float r1, float r2)
{
    vec3 dir;
    float r = sqrt(r1);
    float phi = TWO_PI * r2;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));
    return dir;
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

// 余弦加权的法向半球采样
// ------------------
vec3 SampleCosineHemisphere(float xi_1, float xi_2, vec3 N) {

    // 均匀采样 xy 圆盘然后投影到 z 半球
    float r = sqrt(xi_1);
    float theta = xi_2 * TWO_PI;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(1.0 - x*x - y*y);

    // 从 z 半球投影到法向半球
    vec3 L = toNormalHemisphere(vec3(x, y, z), N);
    return L;
}

vec3 UniformSampleSphere(float r1, float r2)
{
    float z = 1.0 - 2.0 * r1;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * r2;
    return vec3(r * cos(phi), r * sin(phi), z);
}

// GTR1 重要性采样
// -------------
vec3 SampleGTR1(float xi_1, float xi_2, vec3 V, vec3 N, float alpha) {

    float phi_h = xi_1 * TWO_PI;
    float sin_phi_h = sin(phi_h);
    float cos_phi_h = cos(phi_h);

    float cos_theta_h = sqrt((1.0 - pow(alpha * alpha, 1.0 - xi_2))/(1.0 - alpha * alpha));
    float sin_theta_h = sqrt(max(0.0, 1.0 - cos_theta_h * cos_theta_h));

    // 采样 "微平面" 的法向量 作为镜面反射的半角向量 h
    vec3 H = vec3(sin_theta_h * cos_phi_h, sin_theta_h * sin_phi_h, cos_theta_h);
    H = toNormalHemisphere(H, N);   // 投影到真正的法向半球

    // 根据 "微法线" 计算反射光方向
    vec3 L = reflect(-V, H);

    return L;
}

vec3 SampleGTR1(float rgh, float r1, float r2)
{
    float a = max(0.001, rgh);  // alpha
    float a2 = a * a;

    float phi = r1 * TWO_PI;

    float cosTheta = sqrt((1.0 - pow(a2, 1.0 - r1)) / (1.0 - a2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta); // return H
}

// GTR2 重要性采样
vec3 SampleGTR2(float xi_1, float xi_2, vec3 V, vec3 N, float alpha) {

    float phi_h = 2.0 * PI * xi_1;
    float sin_phi_h = sin(phi_h);
    float cos_phi_h = cos(phi_h);

    float cos_theta_h = sqrt((1.0-xi_2)/(1.0+(alpha*alpha-1.0)*xi_2));
    float sin_theta_h = sqrt(max(0.0, 1.0 - cos_theta_h * cos_theta_h));

    // 采样 "微平面" 的法向量 作为镜面反射的半角向量 h
    vec3 H = vec3(sin_theta_h*cos_phi_h, sin_theta_h*sin_phi_h, cos_theta_h);
    H = toNormalHemisphere(H, N);   // 投影到真正的法向半球

    // 根据 "微法线" 计算反射光方向
    vec3 L = reflect(-V, H);

    return L;
}

vec3 SampleGGXVNDF(vec3 V, float ax, float ay, float r1, float r2)
{
    vec3 Vh = normalize(vec3(ax * V.x, ay * V.y, V.z));

    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1, 0, 0);
    vec3 T2 = cross(Vh, T1);

    float r = sqrt(r1);
    float phi = 2.0 * PI * r2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

    return normalize(vec3(ax * Nh.x, ay * Nh.y, max(0.0, Nh.z)));
}


vec2 CranleyPattersonRotation(vec2 p) {
    float u = rand();
    float v = rand();

    p.x += u;
    if(p.x > 1) p.x -= 1;
    if(p.x < 0) p.x += 1;

    p.y += v;
    if(p.y > 1) p.y -= 1;
    if(p.y < 0) p.y += 1;

    return p;
}


// 按照辐射度分布分别采样三种 BRDF
vec3 SampleBRDF(float xi_1, float xi_2, float xi_3, vec3 V, vec3 N, in Material material) {

    float p_diffuse;
    float p_specular;
    float p_clearcoat;

    CalculateBRDFLobePdfs(material, p_diffuse, p_specular, p_clearcoat);

    float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);
    float alpha_GTR2 = max(0.001, sqr(material.roughness));

    // CDF for picking a lobe
    float cdf[3];
    cdf[0] = p_diffuse;
    cdf[1] = cdf[0] + p_clearcoat;
    cdf[2] = cdf[1] + p_specular;

    // 按照概率采样
    float rd = xi_3;
    float eta = dot(V, N) > 0.0 ? (1.0 / material.IOR) : material.IOR;

    vec3 H = SampleGGXVNDF(V, material.ax, material.ay, xi_1, xi_2);

    float VdotH = dot(V, H);
    if (dot(V, N) < 0.0) {
        VdotH = -VdotH;
    }
    // float F = DielectricFresnel(abs(VdotH), eta);
    float F = SchlickFresnel(VdotH);

    // 漫反射
    if(rd <= cdf[0]) {
        return SampleCosineHemisphere(xi_1, xi_2, N);
    }
    // 清漆
    else if(rd <= cdf[1]) {
        return SampleGTR1(xi_1, xi_2, V, N, alpha_GTR1);
    }
    // 镜面 p_diffuse < rd && rd <= p_diffuse + p_specular
    else if(rd <= cdf[2]) {
        return SampleGTR2(xi_1, xi_2, V, N, alpha_GTR2);
    }

    return vec3(0, 1, 0);
}

// https://github.com/wdas/brdf/blob/master/src/brdfs/disney.brdf
vec3 BRDF_Evaluate(vec3 V, vec3 N, vec3 L, vec3 X, vec3 Y, in Material material, out float pdf) {

    pdf = 1e-10;

    float NdotL = dot(N, L);
    float NdotV = dot(N, V);

    if(NdotL < 0 || NdotV < 0) return vec3(0);

    vec3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);
    float VdotH = dot(V, H);

    // Color
    vec3    Cdlin   = material.baseColor;
    float   Cdlum   = Luminance(Cdlin);
    vec3    Ctint   = (Cdlum > 0) ? (Cdlin/Cdlum) : (vec3(1));
    vec3    Cspec   = material.specular * mix(vec3(1), Ctint, material.specularTint);
    vec3    Cspec0  = mix(0.08 * Cspec, Cdlin, material.metallic);  // 0° 镜面反射颜色
    vec3    Csheen  = mix(vec3(1), Ctint, material.sheenTint);      // 织物颜色

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float Fd90  = 0.5 + 2.0 * LdotH * LdotH * material.roughness;
    float FL    = SchlickFresnel(NdotL);
    float FV    = SchlickFresnel(NdotV);
    float Fd    = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // 伪次表面散射 Fake Subsurface
    float Fss90 = LdotH * LdotH * material.roughness;
    float Fss   = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss    = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);


    // 镜面反射 -- 各向同性
    float FH    = SchlickFresnel(LdotH);
    float alpha = max(0.001, sqr(material.roughness));
    float   Ds  = GTR2(NdotH, alpha);
    vec3    Fs  = mix(Cspec0, vec3(1), FH);
    float   Gs  = SmithG_GGX(NdotL, material.roughness);
            Gs *= SmithG_GGX(NdotV, material.roughness);

    // 镜面反射 -- 各向异性
    if (material.anisotropic > 0) {
        // float aspect = sqrt(1.0 - material.anisotropic * 0.9);
        // float ax = max(0.001, sqr(material.roughness) / aspect);
        // float ay = max(0.001, sqr(material.roughness) * aspect);
        Ds  = GTR2_Aniso(NdotH, dot(H, X), dot(H, Y), material.ax, material.ay);
        Gs  = SmithG_GGX_Aniso(NdotL, dot(L, X), dot(L, Y), material.ax, material.ay);
        Gs *= SmithG_GGX_Aniso(NdotV, dot(V, X), dot(V, Y), material.ax, material.ay);
    }

    // 清漆 Clearcoat
    float Dr = GTR1(NdotH, mix(0.1, 0.001, 1.0 - material.clearcoatGloss));
    float Fr = mix(0.04, 1.0, FH);
    float Gr = SmithG_GGX(NdotL, 0.25) * SmithG_GGX(NdotV, 0.25);

    // 光泽 sheen
    vec3 Fsheen = FH * material.sheen * Csheen;

    // Evaluate xxx Item BRDF fr
    vec3 diffuse = INV_PI * mix(Fd, ss, material.subsurface) * Cdlin + Fsheen;
    vec3 specular = Gs * Fs * Ds / (4.0 * NdotV * NdotL);
    vec3 clearcoat = vec3(0.25) * Gr * Fr * Dr * material.clearcoat / (4.0 * NdotV * NdotL);

    // 根据辐射度计算概率
    float p_diffuse;
    float p_specular;
    float p_clearcoat;

    CalculateBRDFLobePdfs(material, p_diffuse, p_specular, p_clearcoat);

    // 分别计算三种 BRDF 的概率密度
    float pdf_diffuse = NdotL * INV_PI;
    float pdf_specular = Ds * NdotH / (4.0 * LdotH);
    float pdf_clearcoat = Dr * NdotH / (4.0 * LdotH);

    // 根据概率混合 pdf
    pdf = p_diffuse * pdf_diffuse + p_specular * pdf_specular + p_clearcoat * pdf_clearcoat;

    pdf = max(1e-10, pdf);

    // BRDF
    return (1.0 - material.metallic) * diffuse + specular + clearcoat;
}

// ============== BSDF ===============

vec3 EvalDiffuse(Material mat, vec3 Csheen, vec3 V, vec3 L, vec3 H, out float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
    return vec3(0.0);

    // Diffuse
    float FL = SchlickFresnel(L.z);
    float FV = SchlickFresnel(V.z);
    float FH = SchlickFresnel(dot(L, H));
    float Fd90 = 0.5 + 2.0 * dot(L, H) * dot(L, H) * mat.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Fake Subsurface TODO: Replace with volumetric scattering
    float Fss90 = dot(L, H) * dot(L, H) * mat.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (L.z + V.z) - 0.5) + 0.5);

    // Sheen
    vec3 Fsheen = FH * mat.sheen * Csheen;

    pdf = L.z * INV_PI;
    return (1.0 - mat.metallic) * (1.0 - mat.transmission) * (INV_PI * mix(Fd, ss, mat.subsurface) * mat.baseColor + Fsheen);
}

vec3 EvalSpecReflection(Material mat, float eta, vec3 specCol, vec3 V, vec3 L, vec3 H, out float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
    return vec3(0.0);

    float   FM  = DisneyFresnel(mat, eta, dot(L, H), dot(V, H)); // SchlickFresnel(dot(L, H));
    vec3    F   = mix(specCol, vec3(1.0), FM); // mix(Cspec0, vec3(1), FM);
    float   D   = GTR2_Aniso(H.z, H.x, H.y, mat.ax, mat.ay);
    float   G1  = SmithG_GGX_Aniso(abs(V.z), V.x, V.y, mat.ax, mat.ay);
    float   G2  = G1 * SmithG_GGX_Aniso(abs(L.z), L.x, L.y, mat.ax, mat.ay);

    pdf = G1 * D / (4.0 * V.z);
    return F * D * G2 / (4.0 * L.z * V.z);
}

vec3 EvalSpecRefraction(Material mat, float eta, vec3 V, vec3 L, vec3 H, out float pdf)
{
    pdf = 0.0;
    if (L.z >= 0.0)
    return vec3(1.0, 0.0, 0.0);

    float F = DielectricFresnel(abs(dot(V, H)), eta);
    float D = GTR2_Aniso(H.z, H.x, H.y, mat.ax, mat.ay);
    float G1 = SmithG_GGX_Aniso(abs(V.z), V.x, V.y, mat.ax, mat.ay);
    float G2 = G1 * SmithG_GGX_Aniso(abs(L.z), L.x, L.y, mat.ax, mat.ay);
    float denom = dot(L, H) + dot(V, H) * eta;
    denom *= denom;
    float eta2 = eta * eta;
    float jacobian = abs(dot(L, H)) / denom;

    pdf = G1 * max(0.0, dot(V, H)) * D * jacobian / V.z;

    return pow(mat.baseColor, vec3(0.5)) * (1.0 - mat.metallic) * mat.transmission * (1.0 - F) * D * G2 * abs(dot(V, H)) * jacobian * eta2 / abs(L.z * V.z);
}

vec3 EvalClearcoat(Material mat, vec3 V, vec3 L, vec3 H, out float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
    return vec3(0.0);

    float FH = DielectricFresnel(dot(V, H), 1.0 / 1.5);
    float F = mix(0.04, 1.0, FH);
    float D = GTR1(H.z, mat.clearcoatGloss); // float D = GTR1(H.z, mix(0.1, 0.001, mat.clearcoatGloss));
    float G = SmithG_GGX(L.z, 0.25) * SmithG_GGX(V.z, 0.25);
    float jacobian = 1.0 / (4.0 * dot(V, H));

    pdf = D * H.z * jacobian;
    return vec3(0.25) * mat.clearcoat * F * D * G / (4.0 * L.z * V.z);
}

vec3 DisneyEval(Material material, vec3 V, vec3 N, vec3 L, out float bsdfPdf)
{
    // V = -hit.viewDir
    // N already flipped

    bsdfPdf = 0.0;
    vec3 f = vec3(0.0);

    float eta = dot(V, N) > 0.0 ? (1.0 / material.IOR) : material.IOR;

    // TODO: Tangent and bitangent should be calculated from mesh (provided, the mesh has proper uvs)
    vec3 T, B;
    getTangent(N, T, B);
    V = ToLocal(T, B, N, V); // NDotL = L.z; NDotV = V.z; NDotH = H.z
    L = ToLocal(T, B, N, L);

    vec3 H;
    if (L.z > 0.0)
    H = normalize(L + V);
    else
    H = normalize(L + V * eta);

    if (H.z < 0.0)
    H = -H;

    // Specular and sheen color
    vec3 specCol, sheenCol;
    GetSpecColor(material, eta, specCol, sheenCol);

    // Lobe weights
    float diffuseWt, specReflectWt, specRefractWt, clearcoatWt;
    float fresnel = DisneyFresnel(material, eta, dot(L, H), dot(V, H));
    CalculateBSDFLobePdfs(material, eta, specCol, fresnel, diffuseWt, specReflectWt, specRefractWt, clearcoatWt);

    float pdf;

    // Diffuse
    if (diffuseWt > 0.0 && L.z > 0.0)
    {
        f += EvalDiffuse(material, sheenCol, V, L, H, pdf);
        bsdfPdf += pdf * diffuseWt;
    }

    // Specular Reflection
    if (specReflectWt > 0.0 && L.z > 0.0 && V.z > 0.0)
    {
        f += EvalSpecReflection(material, eta, specCol, V, L, H, pdf);
        bsdfPdf += pdf * specReflectWt;
    }

    // Specular Refraction
    if (specRefractWt > 0.0 && L.z < 0.0)
    {
        f += EvalSpecRefraction(material, eta, V, L, H, pdf);
        bsdfPdf += pdf * specRefractWt;
    }

    // Clearcoat
    if (clearcoatWt > 0.0 && L.z > 0.0 && V.z > 0.0)
    {
        f += EvalClearcoat(material, V, L, H, pdf);
        bsdfPdf += pdf * clearcoatWt;
    }

    return f * abs(L.z);
}

// SampleBRDF(xi_1, xi_2, xi_3, V, N, hit.material);
vec3 DisneySample(float xi_1, float xi_2, float xi_3, Material material, vec3 V, vec3 N, out vec3 L, out float pdf, out bool isRefract)
{
    pdf = 0.0;
    vec3 f = vec3(0.0);
    isRefract = false;

    float r1 = xi_1;
    float r2 = xi_2;

    float eta = dot(V, N) > 0.0 ? (1.0 / material.IOR) : material.IOR;

    // TODO: Tangent and bitangent should be calculated from mesh (provided, the mesh has proper uvs)
    vec3 T, B;
    getTangent(N, T, B);
    V = ToLocal(T, B, N, V); // NDotL = L.z; NDotV = V.z; NDotH = H.z

    // Specular and sheen color
    vec3 specCol, sheenCol;
    GetSpecColor(material, eta, specCol, sheenCol);

    // Lobe weights
    float diffuseWt, specReflectWt, specRefractWt, clearcoatWt;
    // Note: Fresnel is approx and based on N and not H since H isn't available at this stage.
    float approxFresnel = DisneyFresnel(material, eta, V.z, V.z);
    CalculateBSDFLobePdfs(material, eta, specCol, approxFresnel, diffuseWt, specReflectWt, specRefractWt, clearcoatWt);

    // CDF for picking a lobe
    float cdf[4];
    cdf[0] = diffuseWt;
    cdf[1] = cdf[0] + clearcoatWt;
    cdf[2] = cdf[1] + specReflectWt;
    cdf[3] = cdf[2] + specRefractWt;

    if (r1 < cdf[0]) // Diffuse Reflection Lobe
    {
        r1 /= cdf[0];
        L = CosineSampleHemisphere(r1, r2);
        // L = SampleCosineHemisphere(xi_1, xi_2, N); // Error

        vec3 H = normalize(L + V);

        f = EvalDiffuse(material, sheenCol, V, L, H, pdf);
        pdf *= diffuseWt;
    }
    else if (r1 < cdf[1]) // Clearcoat Lobe
    {
        r1 = (r1 - cdf[0]) / (cdf[1] - cdf[0]);

        vec3 H = SampleGTR1(material.clearcoatGloss, r1, r2);

        if (H.z < 0.0)
        H = -H;

        L = normalize(reflect(-V, H));

        f = EvalClearcoat(material, V, L, H, pdf);
        pdf *= clearcoatWt;
    }
    else  // Specular Reflection/Refraction Lobes
    {
        r1 = (r1 - cdf[1]) / (1.0 - cdf[1]);
        vec3 H = SampleGGXVNDF(V, material.ax, material.ay, r1, r2);

        if (H.z < 0.0)
        H = -H;

        // TODO: Refactor into metallic BRDF and specular BSDF
        float fresnel = DisneyFresnel(material, eta, dot(L, H), dot(V, H));
        float F = 1.0 - ((1.0 - fresnel) * material.transmission * (1.0 - material.metallic));

        if (xi_3 < F)
        {
            L = normalize(reflect(-V, H));

            f = EvalSpecReflection(material, eta, specCol, V, L, H, pdf);
            pdf *= F;
        }
        else
        {
            isRefract = true;
            L = normalize(refract(-V, H, eta));

            f = EvalSpecRefraction(material, eta, V, L, H, pdf);
            pdf *= (1.0 - F);
        }

        pdf *= specReflectWt + specRefractWt;
    }

    L = ToWorld(T, B, N, L);
    return f * abs(dot(N, L));
}

// Sample HDR Environment Color with L
// -----------------------------------
vec3 hdrColor(vec3 L) {
    vec2 uv = toSphericalCoord(normalize(L));
    vec3 color = texture(hdrMap, uv).rgb;
    return color;
}

// 输入光线方向 L 获取 HDR 在该位置的概率密度
// hdr 分辨率为 4096 x 2048 --> hdrResolution = 4096
float hdrPdf(vec3 L, int hdrResolution) {
    vec2 uv = toSphericalCoord(normalize(L));   // 方向向量转 uv 纹理坐标

    float pdf = texture(hdrCache, uv).b;      // 采样概率密度

    // float theta = PI * (0.5 - uv.y);            // theta 范围 [-pi/2 ~ pi/2]
    float theta = PI * uv.y;
    float sin_theta = max(sin(theta), 1e-10);

    // 球坐标和图片积分域的转换系数
    float p_convert = float(hdrResolution * hdrResolution / 2) / (TWO_PI * PI * sin_theta);

    return pdf * p_convert;
}

// Default Sky Color
// -----------------
vec3 getDefaultSkyColor(float y) {
    float t = 0.5 * (y + 1.0);
    return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
}

vec3 SampleHG(vec3 V, float g, float r1, float r2)
{
    float cosTheta;

    if (abs(g) < 0.001)
    cosTheta = 1 - 2 * r2;
    else
    {
        float sqrTerm = (1 - g * g) / (1 + g - 2 * g * r2);
        cosTheta = -(1 + g * g - sqrTerm * sqrTerm) / (2 * g);
    }

    float phi = r1 * TWO_PI;
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    vec3 v1, v2;
    getTangent(V, v1, v2);

    return sinTheta * cosPhi * v1 + sinTheta * sinPhi * v2 + cosTheta * V;
}

float PhaseHG(float cosTheta, float g)
{
    float denom = 1 + g * g + 2 * g * cosTheta;
    return INV_4_PI * (1 - g * g) / (denom * sqrt(denom));
}

vec3 EvalTransmittance(Ray r)
{
    // LightSampleRec lightSample;
    // State state;

    HitRecord hitRec;

    vec3 transmittance = vec3(1);

    for (int depth = 0; depth < maxBounce; depth++)
    {
        // bool isHit = ClosestHit(r, state, lightSample);
        hitRec = hitBVH(r);

        // If no hit (environment map) or if ray hit a light source then return transmittance
        if (!hitRec.isHit)// || state.isEmitter)
        {
//            if (depth == 0) {
//                transmittance = vec3(1.0, 0.0, 0.0);
//            }
//            else {
//                transmittance = vec3(0.0, 1.0, 0.0);
////                transmittance *= hdrColor(r.direction);
//            }
            transmittance *= hdrColor(r.direction) * envIntensity;
            break;
        }

        // TODO: Get only parameters that are needed to calculate transmittance
        // GetMaterial(state, r);

        Material mat = hitRec.material;

        // bool alphatest = (state.mat.alphaMode == ALPHA_MODE_MASK && state.mat.opacity < state.mat.alphaCutoff) || (state.mat.alphaMode == ALPHA_MODE_BLEND && rand() > state.mat.opacity);
        bool refractive = (1.0 - mat.metallic) * mat.transmission > 0.0;

        // Refraction is ignored (Not physically correct but helps with sampling lights from inside refractive objects)
        if(hitRec.isHit && !(/*alphatest ||*/ refractive))
        return vec3(0);

        // Evaluate transmittance
        if (true /*hitRec.isInside*/ /*dot(r.direction, hitRec.normal) > 0 && mat.medium.type != MEDIUM_NONE*/)
        {
            vec3 color = vec3(1.0);//mat.medium.type == MEDIUM_ABSORB ? vec3(1.0) - mat.medium.color : vec3(1.0);
            // transmittance *= exp(-color * mat.medium.density * hitRec.distance);
            transmittance *= exp(-color * 0.5 * hitRec.distance);
//            transmittance = vec3(0.0, 0.0, 1.0);
        }
//        else {
//            transmittance = vec3(0.0, 0.0, 1.0);
//        }

        // Move ray origin to hit point
        r.origin = hitRec.hitPoint + r.direction * EPS;
    }

    return transmittance;
}

// Mix Pdf
// -------
float misMixWeight(float a, float b) {
    float t = a * a;
    return t / (b*b + t);
}

vec3 shadingImportanceSampling_BRDF(HitRecord hit) {

    vec3 Lo         = vec3(0);
    vec3 history    = vec3(1);

    for (int i = 0; i < maxBounce; i++) {

        vec3 V = -hit.viewDir;
        vec3 N = hit.normal;

        // Random Sample HDR Environment Map
        Ray hdrTestRay;
        hdrTestRay.origin       = hit.hitPoint;
        hdrTestRay.direction    = SampleHdr(rand(), rand());

        vec3 tangent, bitangent;
        getTangent(N, tangent, bitangent);

        if(dot(N, hdrTestRay.direction) > 0.0) {
            HitRecord hdrHit = hitBVH(hdrTestRay);

            if(!hdrHit.isHit) {
                vec3 L = hdrTestRay.direction;

                float   light_pdf   = hdrPdf(L, hdrResolution);
                vec3    light_fr    = hdrColor(L) * envIntensity;

                float   disney_brdf_pdf;
                vec3    disney_brdf_fr = BRDF_Evaluate(V, N, L, tangent, bitangent, hit.material, disney_brdf_pdf);

                float mis_weight = misMixWeight(light_pdf, disney_brdf_pdf);
                Lo += mis_weight * history * light_fr * disney_brdf_fr * abs(dot(N, L)) / light_pdf;
            }
        }

        vec2 uv = sobolVec2(camera.loopNum + 1, i);
        uv = CranleyPattersonRotation(uv);
        float xi_1 = uv.x;
        float xi_2 = uv.y;
        float xi_3 = rand();

        vec3 L = SampleBRDF(xi_1, xi_2, xi_3, V, N, hit.material);
        float NdotL = dot(N, L);

        float pdf_brdf;
        vec3 f_r = BRDF_Evaluate(V, N, L, tangent, bitangent, hit.material, pdf_brdf);
        if(pdf_brdf <= 0.0) break;

        history *= f_r * abs(NdotL) / pdf_brdf;

        Ray randomRay;
        randomRay.origin = hit.hitPoint;
        randomRay.direction = L;
        HitRecord newHit = hitBVH(randomRay);

        if(!newHit.isHit) {
            vec3 skyColor = vec3(0);
            if(enableEnvMap){
                skyColor = hdrColor(L) * envIntensity;
                float pdf_light = hdrPdf(L, hdrResolution);

                float mis_weight = misMixWeight(pdf_brdf, pdf_light);   // f(a,b) = a^2 / (a^2 + b^2)
                Lo += mis_weight * history * skyColor * f_r * abs(NdotL) / pdf_brdf;
            }
            else {
                skyColor = getDefaultSkyColor(randomRay.direction.y);
                Lo += history * skyColor * f_r * abs(NdotL) / pdf_brdf;
            }
            break;
        }

        vec3 Le = newHit.material.emissive;
        Lo += history * Le * f_r * abs(NdotL) / pdf_brdf;

        hit = newHit;
    }
    return Lo;
}

vec3 shadingImportanceSampling_BSDF(HitRecord hit) {

    vec3 Lo         = vec3(0);
    vec3 history    = vec3(1);

    for (int i = 0; i < maxBounce; i++) {

        vec3 V = -hit.viewDir;
        vec3 N = hit.normal;

        // Random Sample HDR Environment Map
        Ray hdrTestRay;
        hdrTestRay.origin       = hit.hitPoint;
        hdrTestRay.direction    = SampleHdr(rand(), rand());

        // Surface
        if(dot(N, hdrTestRay.direction) > 0.0) {
            HitRecord hdrHit = hitBVH(hdrTestRay);

            // Hit HDR Map
            if(!hdrHit.isHit) {
                vec3 L = hdrTestRay.direction;

                float   light_pdf   = hdrPdf(L, hdrResolution);
                vec3    light_fr    = hdrColor(L) * envIntensity;

                float   disney_eval_pdf;
                vec3    disney_eval_fr = DisneyEval(hit.material, V, N, L, disney_eval_pdf);

                float   mis_weight = misMixWeight(light_pdf, disney_eval_pdf);

                if (!enableMultiImportantSample) {
                    mis_weight = 1.0;
                }

                Lo += mis_weight * history * light_fr * disney_eval_fr / light_pdf;
            }
        }

        // sobol random
        vec2    uv = sobolVec2(camera.loopNum + 1, i);
                uv = CranleyPattersonRotation(uv);

        float xi_1 = uv.x;
        float xi_2 = uv.y;
        float xi_3 = rand();

        float disney_sample_pdf    = 0.0;
        vec3  disney_sample_fr     = vec3(0);
        vec3  L                    = vec3(0);

        bool isRefract;
        disney_sample_fr = DisneySample(xi_1, xi_2, xi_3, hit.material, V, N, L, disney_sample_pdf, isRefract);


        bool mediumSampled = false;
        float scatter_pdf = 0.0;
        float transmittance = 1.0;

        // Cumulative the Color
        if (disney_sample_pdf > 0.0) {
            if (!isRefract) {
                history *= disney_sample_fr / disney_sample_pdf;
            }
            else {
                if (hit.material.medium.type == MEDIUM_ABSORB) {
                    history *= exp(-(1.0 - hit.material.medium.color) * hit.distance * hit.material.medium.density);
                }
                else if(hit.material.medium.type == MEDIUM_EMISSIVE) {
                    Lo += hit.material.medium.color * hit.distance * hit.material.medium.density * history;
                }
                else if(hit.material.medium.type == MEDIUM_SCATTER) {
                    float scatterDist = min(-log(xi_3) / hit.material.medium.density, hit.distance);
                    mediumSampled = scatterDist < hit.distance;

                    if (mediumSampled)
                    {
                        transmittance *= exp(-1.0 * scatterDist);
                        history *= hit.material.medium.color * transmittance;

                        // Move ray origin to scattering position
                        hit.hitPoint += hit.viewDir * scatterDist;

                        // Pick a new direction based on the phase function
                        vec3 scatterDir = SampleHG(V, hit.material.medium.anisotropy, xi_1, xi_2);
                        scatter_pdf = PhaseHG(dot(V, scatterDir), hit.material.medium.anisotropy);
                        L = scatterDir;
                    }
                }
            }
        }
        else {
            break;
        }
        // TODO: InMeduie

        // Next Hit
        float disney_eval_pdf     = 0.0;
        vec3  disney_eval_fr      = vec3(0);
        disney_eval_fr = DisneyEval(hit.material, V, N, L, disney_eval_pdf);

        // Medium Sampled
        if (mediumSampled && scatter_pdf > 0.0) {
            disney_eval_pdf = scatter_pdf;
            disney_eval_fr = vec3(scatter_pdf);
        }

        Ray randomRay;
        randomRay.origin        = hit.hitPoint;
        randomRay.direction     = L;

        HitRecord nextHit       = hitBVH(randomRay);

        // Next Hit HDR Map
        if(!nextHit.isHit) {
            vec3 light_fr = vec3(0);
            if(enableEnvMap) {
                light_fr = hdrColor(L) * envIntensity;

                float light_pdf = hdrPdf(L, hdrResolution);
                float mis_weight = misMixWeight(disney_eval_pdf, light_pdf);

                if (!enableMultiImportantSample) {
                    mis_weight = 1.0;
                }

                if (!mediumSampled)
                Lo += mis_weight * history * light_fr * disney_eval_fr / disney_eval_pdf;
                else
                Lo += history * light_fr * disney_eval_fr / light_pdf;

            }
            else {
                light_fr = getDefaultSkyColor(randomRay.direction.y);
                Lo += history * light_fr * disney_eval_fr / disney_eval_pdf;
            }
            break;
        }

        // Next Hit Emisstive
        vec3 Le = nextHit.material.emissive;
        Lo += history * Le * disney_eval_fr / disney_eval_pdf;

        // Recursion
        hit = nextHit;
    }
    return Lo;
}

void main() {

    wseed = uint(randOrigin * float(6.95857) * (TexCoords.x * TexCoords.y));

    vec3 hist = texture(historyTexture, TexCoords).rgb;

    if (maxIterations == -1 || camera.loopNum < maxIterations) {
        Ray cameraRay;
        cameraRay.origin = camera.position;
        cameraRay.direction = normalize(camera.leftBottomCorner + (TexCoords.x * 2.0 * camera.halfW) * camera.right + (TexCoords.y * 2.0 * camera.halfH) * camera.up);
        HitRecord firstHit = hitBVH(cameraRay);

        vec3 curColor = vec3(1);

        if(!firstHit.isHit) {
            if(enableEnvMap) {
                curColor = hdrColor(cameraRay.direction) * envIntensity;
            }
            else {
                curColor = getDefaultSkyColor(cameraRay.direction.y);
            }
        }
        else {
            vec3 Le = firstHit.material.emissive;
            vec3 Li = vec3(0);
            if (enableBSDF) {
                Li = shadingImportanceSampling_BSDF(firstHit);
            }
            else {
                Li = shadingImportanceSampling_BRDF(firstHit);
            }
            curColor = Le + Li;
        }

        curColor = (1.0 / float(camera.loopNum)) * curColor + (float(camera.loopNum - 1) / float(camera.loopNum)) * hist;
        FragColor = vec4(curColor, 1.0);
    }
    else {
        FragColor = vec4(hist, 1.0);
    }
}