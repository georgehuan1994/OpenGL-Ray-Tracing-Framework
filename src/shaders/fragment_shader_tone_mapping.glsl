#version 450

in vec2 TexCoords;
out vec4 fragColor;

uniform bool enableToneMapping;
uniform bool enableGammaCorrection;
uniform sampler2D texPass0;
//uniform sampler2D texPass1;
//uniform sampler2D texPass2;
//uniform sampler2D texPass3;
//uniform sampler2D texPass4;
//uniform sampler2D texPass5;
//uniform sampler2D texPass6;

vec3 toneMapping(in vec3 c, float limit) {
    float luminance = 0.3 * c.x + 0.6 * c.y + 0.1 * c.z;
    return c * 1.0 / (1.0 + luminance / limit);
}

vec3 reinhard(in vec3 c) {
    c.xyz /= c.xyz + 1.0;
    return c;
}

// Sources:
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
mat3 ACESInputMat = mat3
(
vec3(0.59719, 0.35458, 0.04823),
vec3(0.07600, 0.90834, 0.01566),
vec3(0.02840, 0.13383, 0.83777)
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
mat3 ACESOutputMat = mat3
(
vec3(1.60475, -0.53108, -0.07367),
vec3(-0.10208, 1.10813, -0.00605),
vec3(-0.00327, -0.07276, 1.07602)
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

vec3 simpleACES(in vec3 c)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

void main() {
    vec3 color = texture(texPass0, TexCoords.xy).rgb;

    // Tone Mapping
    if (enableToneMapping) {
//        color = reinhard(color);
        color = simpleACES(color);
//        color = ACESFitted(color);
    }

    // Gamma Correction
    if (enableGammaCorrection) {
        color = pow(color, vec3(1.0 / 2.2));
    }

    fragColor = vec4(color, 1.0);
}
