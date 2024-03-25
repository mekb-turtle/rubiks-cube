#version 330 core

in vec3 fColor;
in vec2 fTexCoords;
in vec3 fPosition;
in vec3 fNormal;

out vec4 fragColor;

float normalDistance(vec3 normal1, vec3 normal2) {
	float dotProduct = dot(normalize(normal1), normalize(normal2));
	float distance = acos(dotProduct) / radians(180.0f);
	return distance;
}

void main() {
	float nDist = normalDistance(fNormal, vec3(0.0f, 0.0f, 1.0f));
	float pDist = distance(fPosition, vec3(0.0f, 0.0f, 1.0f));
	fragColor = vec4(fColor, 1.0) * nDist * pDist;
}
