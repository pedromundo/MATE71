#version 400
in vec3 aPosition;
void main() {
    gl_Position = vec4(aPosition,1);
}