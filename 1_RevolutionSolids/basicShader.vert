#version 120
attribute vec3 aPosition_object;
attribute vec3 aNormal_object;
attribute vec4 aColor;
attribute vec2 aUV;
varying vec4 vColor;
varying vec3 vNormal_world;
varying vec3 vFragPos_world;
varying vec2 vUV;
uniform mat4 MVP;
uniform mat4 M;
uniform mat3 normalMatrix;
uniform vec3 lightPos_world;
uniform vec3 eyePos_world;

void main() {
    gl_Position = MVP * vec4(aPosition_object,1);
	vFragPos_world = vec3(M * vec4(aPosition_object, 1));
    vColor = aColor;
	vUV = aUV;
	vNormal_world = normalMatrix * aNormal_object;
}
