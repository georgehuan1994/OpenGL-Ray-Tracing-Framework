#version 450
out vec4 FragColor;
in vec2 TexCoords;

//uniform sampler2D screenTexture;
//
//void main() {
//	vec3 col = texture(screenTexture, TexCoords).rgb;
//	FragColor = vec4(col, 1.0);
//}

uniform sampler2D ComTexture;

void main() {
	FragColor = texture(ComTexture, vec2(gl_FragCoord.xy) / vec2(textureSize(ComTexture, 0)));
}

