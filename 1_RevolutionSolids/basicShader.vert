#version 400
in vec3 aPosition_object;
in vec3 aNormal_object;
in vec4 aColor;
in vec2 aUV;
out vec4 vColor;
out vec3 vNormal_world;
out vec3 vFragPos_world;
out vec2 vUV;
uniform mat4 MVP;
uniform mat4 M;
uniform vec3 lightPos_world;
uniform vec3 eyePos_world;

void main() {
    gl_Position = MVP * vec4(aPosition_object,1);
	vFragPos_world = vec3(M * vec4(aPosition_object, 1));
    vColor = aColor;
	vUV = aUV;
	vNormal_world = mat3(transpose(inverse(M))) * aNormal_object;
}