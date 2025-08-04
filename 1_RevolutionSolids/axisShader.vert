#version 120
attribute vec3 aPosition;
varying vec4 vColor;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(aPosition,1.0);
	if(aPosition == vec3(0.0)){
		vColor = vec4(1.0);
	}else{
		vColor = vec4(aPosition,1.0);
	}
}
