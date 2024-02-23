#version 330 core

in vec3 aPosition;
out vec3 vertexColor;
uniform vec2 look;

mat3 rotationMatrix(float angle, vec3 axis) {
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;
	vec3 asq = axis * axis;

	return mat3(
	        asq.x + c * (1.0 - asq.x),
	        axis.x * axis.y * oc + axis.z * s,
	        axis.x * axis.z * oc - axis.y * s,

	        axis.x * axis.y * oc - axis.z * s,
	        asq.y + c * (1.0 - asq.y),
	        axis.y * axis.z * oc + axis.x * s,

	        axis.x * axis.z * oc + axis.y * s,
	        axis.y * axis.z * oc - axis.x * s,
	        asq.z + c * (1.0 - asq.z));
}

void main() {
	vec3 pos = aPosition;

	// Rotation around X-axis (pitch)
	mat3 pitchRotation = rotationMatrix(radians(look.y), vec3(-1.0, 0.0, 0.0));
	// Rotation around Y-axis (yaw)
	mat3 yawRotation = rotationMatrix(radians(look.x), vec3(0.0, -1.0, 0.0));

	pos = yawRotation * pitchRotation * pos;

	gl_Position = vec4(pos, 1.0);
	vertexColor = vec3(0, 1, 0);
}
