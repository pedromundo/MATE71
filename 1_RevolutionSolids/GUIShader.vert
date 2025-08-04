#version 120
attribute vec3 aPosition;
void main() {
    gl_Position = vec4(aPosition,1);
}
