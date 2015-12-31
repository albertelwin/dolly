
#ifndef INTRIN_HPP_INCLUDED
#define INTRIN_HPP_INCLUDED

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

	i32 round_to_i32(f32 x) {
		return (i32)(x + 0.5f);
	}

	i32 ceil_to_i32(f32 x) {
		return (i32)std::ceil(x);
	}
}

#endif