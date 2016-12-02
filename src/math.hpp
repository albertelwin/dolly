
#ifndef MATH_HPP_INCLUDED
#define MATH_HPP_INCLUDED

//NOTE: Get rid of these?
#include <cmath>
#include <cstdlib>

namespace math {
	f32 const PI = 3.14159265359f;
	f32 const TAU = 6.28318530718f;

	inline f32 to_radians(f32 x) {
		return (PI / 180.0f) * x;
	}

	inline f32 to_degrees(f32 x) {
		return (180.0f / PI) * x;
	}

	inline f32 abs(f32 x) {
		return std::fabsf(x);
	}

	inline f32 sin(f32 x) {
		return std::sinf(x);
	}

	inline f32 cos(f32 x) {
		return std::cosf(x);
	}

	inline f32 tan(f32 x) {
		return std::tanf(x);
	}

	inline f32 acos(f32 x) {
		return std::acosf(x);
	}

	inline f32 min(f32 x, f32 y) {
		return (x < y) ? x : y;
	}

	inline f32 max(f32 x, f32 y) {
		return (x > y) ? x : y;
	}

	inline f32 clamp(f32 x, f32 u, f32 v) {
		return max(min(x, v), u);
	}

	inline f32 clamp01(f32 x) {
		return max(min(x, 1.0f), 0.0f);
	}

	inline f32 frac(f32 x) {
		return x - (i32)x;
	}

	inline f32 pow(f32 x, f32 e) {
		return std::pow(x, e);
	}

	inline f32 sqr(f32 x) {
		return x * x;
	}

	inline f32 sqrt(f32 x) {
		//TODO: Optimize this??
		return std::sqrtf(x);
	}

	inline f32 lerp(f32 x, f32 y, f32 t) {
		//TODO: Assert 0 <= t <= 1??
		return x * (1.0f - t) + y * t;
	}

	inline f32 ease(f32 x, f32 y, f32 t) {
		f32 u = (1.0f - t);
		f32 t_sqr = sqr(t);
		f32 u_sqr = sqr(u);
		return 3.0f * t * u_sqr * x + 3.0f * t_sqr * u * y + (t_sqr * t);
	}

	inline i32 rand_i32() {
		return rand();
	}

	inline u32 rand_u32() {
		return (u32)rand();
	}

	inline f32 rand_f32() {
		return (f32)rand() / (f32)RAND_MAX;
	}

	inline i32 round_to_i32(f32 x) {
		return (i32)(x + 0.5f);
	}

	inline i32 ceil_to_i32(f32 x) {
		return (i32)std::ceil(x);
	}

	union Vec2 {
		struct { f32 x, y; };
		f32 v[2];
	};

	inline Vec2 vec2(f32 x, f32 y) {
		return { x, y };
	}

	inline Vec2 vec2(u32 x, u32 y) {
		Vec2 v;
		v.x = (f32)x;
		v.y = (f32)y;
		return v;
	}

	inline Vec2 vec2(i32 x, i32 y) {
		Vec2 v;
		v.x = (f32)x;
		v.y = (f32)y;
		return v;
	}

	inline Vec2 vec2(f32 x) {
		return { x, x };
	}

	inline b32 operator==(Vec2 const & x, Vec2 const & y) {
		return (x.x == y.x && x.y == y.y);
	}

	inline Vec2 operator+(Vec2 const & x, Vec2 const & y) {
		Vec2 tmp = x;
		tmp.x += y.x;
		tmp.y += y.y;
		return tmp;
	}

	inline Vec2 operator+(Vec2 const & v, f32 x) {
		Vec2 tmp = v;
		tmp.x += x;
		tmp.y += x;
		return tmp;
	}

	inline Vec2 & operator+=(Vec2 & x, Vec2 const & y) {
		x.x += y.x;
		x.y += y.y;
		return x;
	}

	inline Vec2 & operator+=(Vec2 & v, f32 x) {
		v.x += x;
		v.y += x;
		return v;
	}

	inline Vec2 operator-(Vec2 const & v) {
		Vec2 tmp = v;
		tmp.x = -v.x;
		tmp.y = -v.y;
		return tmp;
	}

	inline Vec2 operator-(Vec2 const & x, Vec2 const & y) {
		Vec2 tmp = x;
		tmp.x -= y.x;
		tmp.y -= y.y;
		return tmp;
	}

	inline Vec2 operator-(Vec2 const & v, f32 x) {
		Vec2 tmp = v;
		tmp.x -= x;
		tmp.y -= x;
		return tmp;
	}

	inline Vec2 & operator-=(Vec2 & x, Vec2 const & y) {
		x.x -= y.x;
		x.y -= y.y;
		return x;
	}

	inline Vec2 & operator-=(Vec2 & v, f32 x) {
		v.x -= x;
		v.y -= x;
		return v;
	}

	inline Vec2 operator*(Vec2 const & x, Vec2 const & y) {
		Vec2 tmp = x;
		tmp.x *= y.x;
		tmp.y *= y.y;
		return tmp;
	}

	inline Vec2 operator*(Vec2 const & v, f32 x) {
		Vec2 tmp = v;
		tmp.x *= x;
		tmp.y *= x;
		return tmp;
	}

	inline Vec2 & operator*=(Vec2 & v, f32 x) {
		v.x *= x;
		v.y *= x;
		return v;
	}

	inline Vec2 operator/(Vec2 const & x, Vec2 const & y) {
		Vec2 tmp = x;
		tmp.x /= y.x;
		tmp.y /= y.y;
		return tmp;
	}

	inline Vec2 operator/(Vec2 const & v, f32 x) {
		Vec2 tmp = v;
		tmp.x /= x;
		tmp.y /= x;
		return tmp;
	}

	inline Vec2 & operator/=(Vec2 & x, Vec2 const & y) {
		x.x /= y.x;
		x.y /= y.y;
		return x;
	}

	inline Vec2 & operator/=(Vec2 & v, f32 x) {
		v.x /= x;
		v.y /= x;
		return v;
	}

	inline Vec2 clamp01(Vec2 const & x) {
		return math::vec2(math::clamp01(x.x), math::clamp01(x.y));
	}

	inline f32 dot(Vec2 const & x, Vec2 const & y) {
		return x.x * y.x + x.y * y.y;
	}

	inline Vec2 perp(Vec2 x) {
		Vec2 y;
		y.x = -x.y;
		y.y = x.x;
		return y;
	}

	inline Vec2 lerp(Vec2 const & x, Vec2 const & y, f32 t) {
		return x * (1.0f - t) + y * t;
	}

	inline f32 length(Vec2 const & v) {
		return std::sqrt(v.x * v.x + v.y * v.y);
	}

	inline f32 length_squared(Vec2 const & v) {
		return v.x * v.x + v.y * v.y;
	}

	inline Vec2 normalize(Vec2 const & v) {
		f32 const len = std::sqrt(v.x * v.x + v.y * v.y);
		return (len > 0.0f) ? v / len : vec2(0.0f);
	}

	inline Vec2 rand_vec2() {
		return { rand_f32(), rand_f32() };
	}

	union Vec3 {
		struct { f32 x, y, z; };
		struct { Vec2 xy; f32 z_; };
		struct { f32 x_; Vec2 yz; };
		f32 v[3];
	};

	Vec3 const VEC3_RIGHT = { 1.0f, 0.0f, 0.0f };
	Vec3 const VEC3_UP = { 0.0f, 1.0f, 0.0f };
	Vec3 const VEC3_FORWARD = { 0.0f, 0.0f, 1.0f };

	inline Vec3 vec3(f32 x, f32 y, f32 z) {
		return { x, y, z };
	}

	inline Vec3 vec3(f32 x) {
		return { x, x, x };
	}

	inline Vec3 vec3(Vec2 const & xy, f32 z) {
		return { xy.x, xy.y, z };
	}

	inline b32 operator==(Vec3 const & x, Vec3 const & y) {
		return (x.x == y.x && x.y == y.y && x.z == y.z);
	}

	inline Vec3 operator+(Vec3 const & x, Vec3 const & y) {
		Vec3 tmp = x;
		tmp.x += y.x;
		tmp.y += y.y;
		tmp.z += y.z;
		return tmp;
	}

	inline Vec3 operator+(Vec3 const & v, f32 x) {
		Vec3 tmp = v;
		tmp.x += x;
		tmp.y += x;
		tmp.z += x;
		return tmp;
	}

	inline Vec3 & operator+=(Vec3 & x, Vec3 const & y) {
		x.x += y.x;
		x.y += y.y;
		x.z += y.z;
		return x;
	}

	inline Vec3 & operator+=(Vec3 & v, f32 x) {
		v.x += x;
		v.y += x;
		v.z += x;
		return v;
	}

	inline Vec3 operator-(Vec3 const & x, Vec3 const & y) {
		Vec3 tmp = x;
		tmp.x -= y.x;
		tmp.y -= y.y;
		tmp.z -= y.z;
		return tmp;
	}

	inline Vec3 operator-(Vec3 const & v) {
		Vec3 tmp;
		tmp.x = -v.x;
		tmp.y = -v.y;
		tmp.z = -v.z;
		return tmp;
	}

	inline Vec3 operator-(Vec3 const & v, f32 x) {
		Vec3 tmp = v;
		tmp.x -= x;
		tmp.y -= x;
		tmp.z -= x;
		return tmp;
	}

	inline Vec3 & operator-=(Vec3 & x, Vec3 const & y) {
		x.x -= y.x;
		x.y -= y.y;
		x.z -= y.z;
		return x;
	}

	inline Vec3 & operator-=(Vec3 & v, f32 x) {
		v.x -= x;
		v.y -= x;
		v.z -= x;
		return v;
	}

	inline Vec3 operator*(Vec3 const & x, Vec3 const & y) {
		Vec3 tmp = x;
		tmp.x *= y.x;
		tmp.y *= y.y;
		tmp.z *= y.z;
		return tmp;
	}

	inline Vec3 operator*(Vec3 const & v, f32 x) {
		Vec3 tmp = v;
		tmp.x *= x;
		tmp.y *= x;
		tmp.z *= x;
		return tmp;
	}

	inline Vec3 & operator*=(Vec3 & x, Vec3 const & y) {
		x.x *= y.x;
		x.y *= y.y;
		x.z *= y.z;
		return x;
	}

	inline Vec3 & operator*=(Vec3 & v, f32 x) {
		v.x *= x;
		v.y *= x;
		v.z *= x;
		return v;
	}

	inline Vec3 operator/(Vec3 const & x, Vec3 const & y) {
		Vec3 tmp = x;
		tmp.x /= y.x;
		tmp.y /= y.y;
		tmp.z /= y.z;
		return tmp;
	}

	inline Vec3 operator/(Vec3 const & v, f32 x) {
		Vec3 tmp = v;
		tmp.x /= x;
		tmp.y /= x;
		tmp.z /= x;
		return tmp;
	}

	inline Vec3 & operator/=(Vec3 & v, f32 x) {
		v.x /= x;
		v.y /= x;
		v.z /= x;
		return v;
	}

	inline Vec3 lerp(Vec3 const & x, Vec3 const & y, f32 t) {
		return x * (1.0f - t) + y * t;
	}

	inline f32 length(Vec3 const & v) {
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	inline f32 length_squared(Vec3 const & v) {
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	inline Vec3 normalize(Vec3 const & v) {
		f32 const len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		return (len > 0.0f) ? v / len : vec3(0.0f);
	}

	inline f32 dot(Vec3 const & x, Vec3 const & y) {
		return x.x * y.x + x.y * y.y + x.z * y.z;
	}

	inline Vec3 cross(Vec3 const & x, Vec3 const & y) {
		return { x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x };
	}

	inline Vec3 rand_vec3() {
		return { rand_f32(), rand_f32(), rand_f32() };
	}

	inline Vec3 rand_sample_in_sphere(f32 d = 1.0f) {
		f32 t = rand_f32() * TAU;
		f32 p = std::acosf(rand_f32() * 2.0f - 1.0f);
		f32 r = std::pow(rand_f32(), (1.0f / 3.0f) * d);

		f32 x = r * math::cos(t) * math::sin(p);
		f32 y = r * math::sin(t) * math::sin(p);
		f32 z = r * math::cos(p);

		return vec3(x, y, z);
	}

	union Vec4 {
		struct { f32 x, y, z, w; };
		struct { Vec3 xyz; f32 w_; };
		struct { Vec2 xy, zw; };
		struct { f32 r, g, b, a; };
		struct { Vec3 rgb; f32 a_; };
		f32 v[4];
	};

	inline Vec4 vec4(f32 x, f32 y, f32 z, f32 w) {
		return { x, y, z, w };
	}

	inline Vec4 vec4(f32 x) {
		return { x, x, x, x };
	}

	inline Vec4 vec4(Vec3 const & v, f32 w) {
		return { v.x, v.y, v.z, w };
	}

	inline Vec4 vec4(Vec2 const & v, f32 z, f32 w) {
		return { v.x, v.y, z, w };
	}

	inline Vec4 operator+(Vec4 const & x, Vec4 const & y) {
		Vec4 tmp = x;
		tmp.x += y.x;
		tmp.y += y.y;
		tmp.z += y.z;
		tmp.w += y.w;
		return tmp;
	}

	inline Vec4 operator-(Vec4 const & x, Vec4 const & y) {
		Vec4 tmp = x;
		tmp.x -= y.x;
		tmp.y -= y.y;
		tmp.z -= y.z;
		tmp.w -= y.w;
		return tmp;
	}

	inline Vec4 operator*(Vec4 const & x, Vec4 const & y) {
		Vec4 tmp = x;
		tmp.x *= y.x;
		tmp.y *= y.y;
		tmp.z *= y.z;
		tmp.w *= y.w;
		return tmp;
	}

	inline f32 dot(Vec4 const & x, Vec4 const & y) {
		return x.x * y.x + x.y * y.y + x.z * y.z + x.w * y.w;
	}

	inline Vec4 rand_vec4() {
		return vec4(rand_f32(), rand_f32(), rand_f32(), rand_f32());
	}

	struct Mat3 {
		f32 v[9];
	};

	struct Rec2 {
		Vec2 min;
		Vec2 max;
	};

	inline Rec2 rec2(Vec2 const & min, Vec2 const & max) {
		Rec2 r;
		r.min = min;
		r.max = max;
		return r;
	}

	inline Rec2 rec2_pos_dim(Vec2 const & pos, Vec2 const & dim) {
		Rec2 r;
		r.min = pos - dim * 0.5f;
		r.max = pos + dim * 0.5f;
		return r;
	}

	inline Rec2 rec2_min_dim(Vec2 const & min, Vec2 const & dim) {
		Rec2 r;
		r.min = min;
		r.max = min + dim;
		return r;
	}

	inline Rec2 rec_offset(Rec2 const & r, Vec2 const & v) {
		Rec2 r2 = r;
		r2.min += v;
		r2.max += v;
		return r2;
	}

	inline Rec2 rec_scale(Rec2 const & r, f32 x) {
		return rec2_pos_dim(r.min + (r.max - r.min) * 0.5f, (r.max - r.min) * x);
	}

	inline Rec2 rec_scale(Rec2 const & r, math::Vec2 x) {
		return rec2_pos_dim(r.min + (r.max - r.min) * 0.5f, (r.max - r.min) * x);
	}

	inline Vec2 rec_pos(Rec2 const & r) {
		return r.min + (r.max - r.min) * 0.5f;
	}

	inline Vec2 rec_dim(Rec2 const & r) {
		return r.max - r.min;
	}

	inline b32 rec_overlap(Rec2 const & r0, Rec2 const & r1) {
		return ((r0.min.x < r1.min.x && r0.max.x > r1.min.x) || (r0.min.x >= r1.min.x && r0.min.x < r1.max.x)) && ((r0.min.y < r1.min.y && r0.max.y > r1.min.y) || (r0.min.y >= r1.min.y && r0.min.y < r1.max.y));
	}

	inline b32 inside_rec(Rec2 const & r, math::Vec2 const v) {
		return (v.x >= r.min.x && v.x < r.max.x && v.y >= r.min.y && v.y < r.max.y);
	}

	struct Ray {
		Vec3 o;
		Vec3 d;
	};

	inline Ray ray(Vec3 o, Vec3 d) {
		return { o, d };
	}

	inline f32 ray_plane_intersection(Ray const & r, Vec3 const & p_n, f32 p_d) {
		f32 t = -1.0f;

		f32 denom = dot(p_n, r.d);
		if(math::abs(denom) > 0.0f) {
			t = -(dot(p_n, r.o) + p_d) / denom;
		}

		return t;
	}
}

#endif