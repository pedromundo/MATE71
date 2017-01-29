#version 400
in vec3 aPosition;
in vec4 aColor;
out vec4 vColor;
void main() {
    gl_Position = vec4(aPosition,1);
    vColor = aColor;
}