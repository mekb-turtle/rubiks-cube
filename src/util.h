#ifndef UTIL_H
#define UTIL_H
struct tri {
	struct vec3 {
		union {
			struct {
				float x, y, z;
			};
			float points[3];
		};
	} vec[3];
};

struct rect {
	struct vec3 vec[4];
};

struct vec3 vec3(float x, float y, float z);
struct tri tri(struct vec3 a, struct vec3 b, struct vec3 c);
struct rect rect(struct vec3 a, struct vec3 b, struct vec3 c, struct vec3 d);
#endif
