#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in uint stickerIndex;

out vec3 vertexColor;

uniform vec2 look;
uniform uint time;
uniform uvec4 animation;
uniform uint turnTime;

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
	vec3 pos = position;

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
			pos = rotateVector(pos, rotateAxis, ani);
		}
	}

	pos = rotateVector(pos, vec3(-1.0, 0.0, 0.0), radians(180.0));

	// rotation on Y-axis (yaw)
	pos = rotateVector(pos, vec3(0.0, -1.0, 0.0), radians(look.x));

	// rotation on X-axis (pitch)
	pos = rotateVector(pos, vec3(-1.0, 0.0, 0.0), radians(look.y));

	gl_Position = vec4(pos, 1.0);

	vertexColor = color;
}
