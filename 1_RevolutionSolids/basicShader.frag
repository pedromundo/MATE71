#version 120
varying vec4 vColor;
varying vec3 vNormal_world;
varying vec3 vFragPos_world;
varying vec2 vUV;
uniform mat4 MVP;
uniform mat4 M;
uniform vec3 lightPos_world;
uniform vec3 eyePos_world;
uniform sampler2D tex;
uniform float specularStrength;
uniform int specularCoefficient;

void main() {
	float ambientStrength = 0.1f;
	vec3 lightColor = vec3(1.0);
    vec3 ambient = ambientStrength * lightColor;

	vec3 lightDir_world = normalize(lightPos_world - vFragPos_world);
	vec3 norm = normalize(vNormal_world);

	float diff = max(dot(norm, lightDir_world), 0.0);
	vec3 diffuse = diff * lightColor;

	vec3 viewDir_world = normalize(eyePos_world - vFragPos_world);
	vec3 reflectDir_world = reflect(-lightDir_world, norm);
	float spec = pow(max(dot(viewDir_world, reflectDir_world), 0.0), float(specularCoefficient));
	vec3 specular = specularStrength * spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * vec3(texture2D(tex, vUV));
	gl_FragColor = vec4(result, 1.0f);
}
