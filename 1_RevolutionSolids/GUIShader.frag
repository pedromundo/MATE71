#version 400
in vec4 gl_FragCoord;
in vec4 vColor;
out vec4 fragColor;
void main() {
	fragColor = vColor;    
}