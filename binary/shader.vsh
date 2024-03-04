#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in uint stickerIndex;

out vec3 vertexColor;

uniform vec2 look;
uniform uint time;
uniform uvec4 animations[1]; // make sure this is the same as max_animations

// rotates a vector around an axis
vec3 rotateVector(vec3 vec, vec3 axis, float angle) {
	float c = cos(angle);
	float s = sin(angle);

	vec3 rotatedVec = vec * c +
	                  cross(axis, vec) * s +
	                  axis * dot(axis, vec) * (1.0 - c);

	return rotatedVec;
}

void main() {
	const uint turn_time = 400u;

	vec3 pos = position;

	// handle rotation
	for (int i = 0; i < 6; ++i) {
		uvec4 anim = animations[i];
		uint bit = 1u << (stickerIndex & 0x1fu);
		// check that this sticker should be rotated
		if (((stickerIndex <= 0x1fu ? anim.y : anim.z) & bit) == 0u) continue;

		// get what axis to rotate on
		uint axis = anim.x % 3u;
		vec3 rotateAxis = vec3(axis == 0u ? 1.0 : 0.0, axis == 1u ? 1.0 : 0.0, axis == 2u ? 1.0 : 0.0);

		// get direction to rotate in
		int dir = 0;
		switch (anim.x / 3u) {
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
		float ani = 0.0f;
		if (time >= anim.w && time < anim.w + turn_time) {
			ani = (1.0f - float(time - anim.w) / turn_time) * radians(90.0) * float(dir);
		}

		// do rotation
		pos = rotateVector(pos, rotateAxis, ani);
	}

	pos = rotateVector(pos, vec3(-1.0, 0.0, 0.0), radians(180.0));

	// Rotation on Y-axis (yaw)
	pos = rotateVector(pos, vec3(0.0, -1.0, 0.0), radians(look.x));

	// Rotation on X-axis (pitch)
	pos = rotateVector(pos, vec3(-1.0, 0.0, 0.0), radians(look.y));

	gl_Position = vec4(pos, 1.0);

	vertexColor = color;
}
