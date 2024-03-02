#include "util.h"

struct vec3 vec3(float x, float y, float z) {
	return (struct vec3){{{x, y, z}}};
}

struct tri tri(struct vec3 a, struct vec3 b, struct vec3 c) {
	return (struct tri){
	        {a, b, c}
    };
}

struct rect rect(struct vec3 a, struct vec3 b, struct vec3 c, struct vec3 d) {
	return (struct rect){
	        {a, b, c, d}
    };
}
