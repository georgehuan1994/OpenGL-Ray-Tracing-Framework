#version 330 core

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

void main() {
    vec3 color = texture(texPass0, TexCoords.xy).rgb;

    // Tone Mapping
    if (enableToneMapping) {
        color = reinhard(color);
    }

    // Gamma Correction
    if (enableGammaCorrection) {
        color = pow(color, vec3(1.0 / 2.2));
    }

    fragColor = vec4(color, 1.0);
}
