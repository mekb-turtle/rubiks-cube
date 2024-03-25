#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in uint stickerIndex;
layout(location = 3) in vec2 texCoords;
layout(location = 4) in vec3 normal;

out vec3 fColor;
out vec2 fTexCoords;
out vec3 fPosition;
out vec3 fNormal;

uniform vec2 look;
uniform uint time;
uniform uvec4 animation;
uniform uint turnTime;

// returns a matrix for a rotation
// https://github.com/dmnsgn/glsl-rotate/blob/main/rotation-3d.glsl
mat4 rotate(vec3 axis, float angle) {
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	return mat4(
	        oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0,
	        oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
	        oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c, 0.0,
	        0.0, 0.0, 0.0, 1.0);
}

void main() {
	mat4 rotation = mat4(1.0);

	// handle rotation
	uint bit = 1u << (stickerIndex & 0x1fu);
	// check that this sticker should be rotated
	if (((stickerIndex <= 0x1fu ? animation.y : animation.z) & bit) != 0u) {
		// get what axis to rotate on
		uint axis = animation.x % 3u;

		// get direction to rotate in
		int dir = 0;
		switch (animation.x / 3u) {
			case 0u:
				dir = -1;
				break;
			case 1u:
				dir = 1;
				break;
			case 2u:
				dir = -2;
				break;
		}

		// animation timer
		if (time >= animation.w && time < animation.w + turnTime) {
			float ani = (1.0f - float(time - animation.w) / turnTime) * radians(90.0) * float(dir);
			vec3 rotateAxis = vec3(axis == 0u ? 1.0 : 0.0, axis == 1u ? 1.0 : 0.0, axis == 2u ? 1.0 : 0.0);

			// do rotation
			rotation *= rotate(rotateAxis, ani);
		}
	}

	rotation *= rotate(vec3(-1.0, 0.0, 0.0), radians(180.0));

	// rotation on Y-axis (yaw)
	rotation *= rotate(vec3(0.0, -1.0, 0.0), radians(look.x));

	// rotation on X-axis (pitch)
	rotation *= rotate(vec3(-1.0, 0.0, 0.0), radians(look.y));

	// apply rotation to vertex position
	vec3 outPos = (vec4(position, 1.0) * rotation).xyz;

	// same with normal
	vec3 outNormal = normalize((vec4(normal, 1.0) * rotation).xyz);

	// write data to fragment shader
	fTexCoords = texCoords;
	fColor = color;
	fPosition = outPos;
	fNormal = outNormal;

	gl_Position = vec4(outPos, 1.0);
}
