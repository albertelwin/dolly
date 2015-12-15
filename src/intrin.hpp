
#ifndef NAMESPACE_INTRIN_INCLUDED
#define NAMESPACE_INTRIN_INCLUDED

#include <cmath>

namespace intrin {
	f32 abs(f32 x) {
		return std::fabsf(x);
	}

	f32 sin(f32 x) {
		return std::sinf(x);
	}

	f32 cos(f32 x) {
		return std::cosf(x);
	}

	f32 tan(f32 x) {
		return std::tanf(x);
	}

	s32 round_to_s32(f32 x) {
		return (s32)(x + 0.5f);
	}

	s32 ceil_to_s32(f32 x) {
		return (s32)std::ceil(x);
	}
}

#endif