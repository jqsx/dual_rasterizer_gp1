#pragma once
#include <cmath>
#include <cfloat>
#include <algorithm>

#include "ColorRGB.h"

namespace dae
{
	/* --- HELPER STRUCTS --- */
	struct Int2
	{
		int x{};
		int y{};
	};

	struct Int_AABB {
		Int2 corner;
		Int2 size;
	};

	/* --- CONSTANTS --- */
	constexpr auto PI = 3.14159265358979323846f;
	constexpr auto PI_DIV_2 = 1.57079632679489661923f;
	constexpr auto PI_DIV_4 = 0.785398163397448309616f;
	constexpr auto PI_2 = 6.283185307179586476925f;
	constexpr auto PI_4 = 12.56637061435917295385f;

	constexpr auto TO_DEGREES = (180.0f / PI);
	constexpr auto TO_RADIANS(PI / 180.0f);

	/* --- HELPER FUNCTIONS --- */

	inline bool isIntersect(const Int_AABB& b0, const Int_AABB& b1) {
		bool x = b0.corner.x > b1.corner.x - b0.size.x && b0.corner.x < b1.corner.x + b1.size.x;
		bool y = b0.corner.y > b1.corner.y - b0.size.y && b0.corner.y < b1.corner.y + b1.size.y;

		return x && y;
	}

	inline float Square(float a)
	{
		return a * a;
	}

	inline float Lerpf(float a, float b, float factor)
	{
		return ((1 - factor) * a) + (factor * b);
	}

	inline bool AreEqual(float a, float b, float epsilon = FLT_EPSILON)
	{
		return abs(a - b) < epsilon;
	}

	inline int Clamp(const int v, int min, int max)
	{
		if (v < min) return min;
		if (v > max) return max;
		return v;
	}

	inline float Clamp(const float v, float min, float max)
	{
		if (v < min) return min;
		if (v > max) return max;
		return v;
	}

	inline float Saturate(const float v)
	{
		if (v < 0.f) return 0.f;
		if (v > 1.f) return 1.f;
		return v;
	}
}