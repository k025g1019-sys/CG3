#include "Game/Util/Easing.h"

float EaseOutQuad(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float EaseInQuad(float t) {
	return t * t;
}

float EaseInOut(float t) {
	return t * t * (3.0f - 2.0f * t);
}
