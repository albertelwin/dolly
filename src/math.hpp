
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

	struct Mat4 {
		f32 v[16];
	};

	Mat4 const MAT4_IDENTITY = {
		 1.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 1.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 1.0f, 0.0f,
		 0.0f, 0.0f, 0.0f, 1.0f,
	};

	inline Mat4 operator*(Mat4 const & x, Mat4 const & y) {
		Mat4 tmp;

		tmp.v[ 0] = y.v[ 0] * x.v[ 0] + y.v[ 1] * x.v[ 4] + y.v[ 2] * x.v[ 8] + y.v[ 3] * x.v[12];
		tmp.v[ 1] = y.v[ 0] * x.v[ 1] + y.v[ 1] * x.v[ 5] + y.v[ 2] * x.v[ 9] + y.v[ 3] * x.v[13];
		tmp.v[ 2] = y.v[ 0] * x.v[ 2] + y.v[ 1] * x.v[ 6] + y.v[ 2] * x.v[10] + y.v[ 3] * x.v[14];
		tmp.v[ 3] = y.v[ 0] * x.v[ 3] + y.v[ 1] * x.v[ 7] + y.v[ 2] * x.v[11] + y.v[ 3] * x.v[15];

		tmp.v[ 4] = y.v[ 4] * x.v[ 0] + y.v[ 5] * x.v[ 4] + y.v[ 6] * x.v[ 8] + y.v[ 7] * x.v[12];
		tmp.v[ 5] = y.v[ 4] * x.v[ 1] + y.v[ 5] * x.v[ 5] + y.v[ 6] * x.v[ 9] + y.v[ 7] * x.v[13];
		tmp.v[ 6] = y.v[ 4] * x.v[ 2] + y.v[ 5] * x.v[ 6] + y.v[ 6] * x.v[10] + y.v[ 7] * x.v[14];
		tmp.v[ 7] = y.v[ 4] * x.v[ 3] + y.v[ 5] * x.v[ 7] + y.v[ 6] * x.v[11] + y.v[ 7] * x.v[15];

		tmp.v[ 8] = y.v[ 8] * x.v[ 0] + y.v[ 9] * x.v[ 4] + y.v[10] * x.v[ 8] + y.v[11] * x.v[12];
		tmp.v[ 9] = y.v[ 8] * x.v[ 1] + y.v[ 9] * x.v[ 5] + y.v[10] * x.v[ 9] + y.v[11] * x.v[13];
		tmp.v[10] = y.v[ 8] * x.v[ 2] + y.v[ 9] * x.v[ 6] + y.v[10] * x.v[10] + y.v[11] * x.v[14];
		tmp.v[11] = y.v[ 8] * x.v[ 3] + y.v[ 9] * x.v[ 7] + y.v[10] * x.v[11] + y.v[11] * x.v[15];

		tmp.v[12] = y.v[12] * x.v[ 0] + y.v[13] * x.v[ 4] + y.v[14] * x.v[ 8] + y.v[15] * x.v[12];
		tmp.v[13] = y.v[12] * x.v[ 1] + y.v[13] * x.v[ 5] + y.v[14] * x.v[ 9] + y.v[15] * x.v[13];
		tmp.v[14] = y.v[12] * x.v[ 2] + y.v[13] * x.v[ 6] + y.v[14] * x.v[10] + y.v[15] * x.v[14];
		tmp.v[15] = y.v[12] * x.v[ 3] + y.v[13] * x.v[ 7] + y.v[14] * x.v[11] + y.v[15] * x.v[15];

		return tmp;
	}

	inline Vec4 operator*(Mat4 const & m, Vec4 const & v) {
		Vec4 tmp;
		tmp.x = v.x * m.v[ 0] + v.y * m.v[ 4] + v.z * m.v[ 8] + v.w * m.v[12];
		tmp.y = v.x * m.v[ 1] + v.y * m.v[ 5] + v.z * m.v[ 9] + v.w * m.v[13];
		tmp.z = v.x * m.v[ 2] + v.y * m.v[ 6] + v.z * m.v[10] + v.w * m.v[14];
		tmp.w = v.x * m.v[ 3] + v.y * m.v[ 7] + v.z * m.v[11] + v.w * m.v[15];
		return tmp;
	}

	inline Mat4 inverse(Mat4 const & x) {
		f32 c_00 = x.v[10] * x.v[15] - x.v[14] * x.v[11];
		f32 c_02 = x.v[ 6] * x.v[15] - x.v[14] * x.v[ 7];
		f32 c_03 = x.v[ 6] * x.v[11] - x.v[10] * x.v[ 7];

		f32 c_04 = x.v[ 9] * x.v[15] - x.v[13] * x.v[11];
		f32 c_06 = x.v[ 5] * x.v[15] - x.v[13] * x.v[ 7];
		f32 c_07 = x.v[ 5] * x.v[11] - x.v[ 9] * x.v[ 7];

		f32 c_08 = x.v[ 9] * x.v[14] - x.v[13] * x.v[10];
		f32 c_10 = x.v[ 5] * x.v[14] - x.v[13] * x.v[ 6];
		f32 c_11 = x.v[ 5] * x.v[10] - x.v[ 9] * x.v[ 6];

		f32 c_12 = x.v[ 8] * x.v[15] - x.v[12] * x.v[11];
		f32 c_14 = x.v[ 4] * x.v[15] - x.v[12] * x.v[ 7];
		f32 c_15 = x.v[ 4] * x.v[11] - x.v[ 8] * x.v[ 7];

		f32 c_16 = x.v[ 8] * x.v[14] - x.v[12] * x.v[10];
		f32 c_18 = x.v[ 4] * x.v[14] - x.v[12] * x.v[ 6];
		f32 c_19 = x.v[ 4] * x.v[10] - x.v[ 8] * x.v[ 6];

		f32 c_20 = x.v[ 8] * x.v[13] - x.v[12] * x.v[ 9];
		f32 c_22 = x.v[ 4] * x.v[13] - x.v[12] * x.v[ 5];
		f32 c_23 = x.v[ 4] * x.v[ 9] - x.v[ 8] * x.v[ 5];

		Vec4 sign_0 = vec4(+1.0f, -1.0f, +1.0f, -1.0f);
		Vec4 sign_1 = vec4(-1.0f, +1.0f, -1.0f, +1.0f);

		Vec4 fac_0 = vec4(c_00, c_00, c_02, c_03);
		Vec4 fac_1 = vec4(c_04, c_04, c_06, c_07);
		Vec4 fac_2 = vec4(c_08, c_08, c_10, c_11);
		Vec4 fac_3 = vec4(c_12, c_12, c_14, c_15);
		Vec4 fac_4 = vec4(c_16, c_16, c_18, c_19);
		Vec4 fac_5 = vec4(c_20, c_20, c_22, c_23);

		Vec4 vec_0 = vec4(x.v[ 4], x.v[ 0], x.v[ 0], x.v[ 0]);
		Vec4 vec_1 = vec4(x.v[ 5], x.v[ 1], x.v[ 1], x.v[ 1]);
		Vec4 vec_2 = vec4(x.v[ 6], x.v[ 2], x.v[ 2], x.v[ 2]);
		Vec4 vec_3 = vec4(x.v[ 7], x.v[ 3], x.v[ 3], x.v[ 3]);

		Vec4 inv_0 = sign_0 * (vec_1 * fac_0 - vec_2 * fac_1 + vec_3 * fac_2);
		Vec4 inv_1 = sign_1 * (vec_0 * fac_0 - vec_2 * fac_3 + vec_3 * fac_4);
		Vec4 inv_2 = sign_0 * (vec_0 * fac_1 - vec_1 * fac_3 + vec_3 * fac_5);
		Vec4 inv_3 = sign_1 * (vec_0 * fac_2 - vec_1 * fac_4 + vec_2 * fac_5);

		Vec4 row_0 = vec4(inv_0.x, inv_1.x, inv_2.x, inv_3.x);
		f32 det = dot(vec4(x.v[ 0], x.v[ 1], x.v[ 2], x.v[ 3]), row_0);
		ASSERT(math::abs(det) > 0.0f);

		return {
			inv_0.x / det, inv_0.y / det, inv_0.z / det, inv_0.w / det,
			inv_1.x / det, inv_1.y / det, inv_1.z / det, inv_1.w / det,
			inv_2.x / det, inv_2.y / det, inv_2.z / det, inv_2.w / det,
			inv_3.x / det, inv_3.y / det, inv_3.z / det, inv_3.w / det,
		};
	}

	inline Mat4 translate(f32 x, f32 y, f32 z) {
		return {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			   x,    y,    z, 1.0f,
		};		
	}

	inline Mat4 translate(Vec3 const & v) {
		return {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			 v.x,  v.y,  v.z, 1.0f,
		};
	}

	inline Mat4 rotate_around_axis(Vec3 const & a, f32 t) {
		f32 sin_t = math::sin(t);
		f32 cos_t = math::cos(t);
		f32 i_cos_t = (1.0f - cos_t);

		Vec3 v = normalize(a);
		f32 vx2 = v.x * v.x;
		f32 vy2 = v.y * v.y;
		f32 vz2 = v.z * v.z;
		f32 vxy = v.x * v.y;
		f32 vxz = v.x * v.z;
		f32 vyz = v.y * v.z;

		return {
			cos_t + vx2 * i_cos_t,       vxy * i_cos_t + v.z * sin_t, vxz * i_cos_t - v.y * sin_t, 0.0f,
			vxy * i_cos_t - v.z * sin_t, cos_t + vy2 * i_cos_t,       vyz * i_cos_t + v.x * sin_t, 0.0f,
			vxz * i_cos_t + v.y * sin_t, vyz * i_cos_t - v.x * sin_t, cos_t + vz2 * i_cos_t,       0.0f,
			0.0f,                        0.0f,                        0.0f,                        1.0f,
		};

	}

	inline Mat4 rotate_around_x(f32 t) {
		f32 sin_t = math::sin(t);
		f32 cos_t = math::cos(t);

		return {
			1.0f,  0.0f,  0.0f, 0.0f,
			0.0f, cos_t, sin_t, 0.0f,
			0.0f,-sin_t, cos_t, 0.0f,
			0.0f,  0.0f,  0.0f, 1.0f,
		};		
	}

	inline Mat4 rotate_around_y(f32 t) {
		f32 sin_t = math::sin(t);
		f32 cos_t = math::cos(t);

		return {
			cos_t, 0.0f,-sin_t, 0.0f,
			 0.0f, 1.0f,  0.0f, 0.0f,
			sin_t, 0.0f, cos_t, 0.0f,
			 0.0f, 0.0f,  0.0f, 1.0f,
		};
	}

	inline Mat4 rotate_around_z(f32 t) {
		f32 sin_t = math::sin(t);
		f32 cos_t = math::cos(t);

		return {
			 cos_t, sin_t, 0.0f, 0.0f,
			-sin_t, cos_t, 0.0f, 0.0f,
			  0.0f,  0.0f, 1.0f, 0.0f,
			  0.0f,  0.0f, 0.0f, 1.0f,
		};
	}

	inline Mat4 scale(f32 x) {
		return {
			  x, 0.0f, 0.0f, 0.0f,
			0.0f,   x, 0.0f, 0.0f,
			0.0f, 0.0f,   x, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};		
	}

	inline Mat4 scale(f32 x, f32 y, f32 z) {
		return {
			  x, 0.0f, 0.0f, 0.0f,
			0.0f,   y, 0.0f, 0.0f,
			0.0f, 0.0f,   z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}

	inline Mat4 scale(Vec3 const & v) {
		return {
			 v.x, 0.0f, 0.0f, 0.0f,
			0.0f,  v.y, 0.0f, 0.0f,
			0.0f, 0.0f,  v.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}

	inline Mat4 look_at(Vec3 const & pos, Vec3 const & at, Vec3 const & up) {
		Vec3 z = normalize(at - pos);
		Vec3 x = normalize(cross(z, normalize(up)));
		Vec3 y = cross(x, z);
		return {
			          x.x,         y.x,        -z.x, 0.0f,
			          x.y,         y.y,        -z.y, 0.0f,
			          x.z,         y.z,        -z.z, 0.0f,
			 -dot(x, pos),-dot(y, pos), dot(z, pos), 1.0f,
		};
	}

	inline Mat4 perspective_projection(f32 aspect_ratio, f32 field_of_view, f32 near_plane, f32 far_plane) {
		f32 tan_fov_over_2 = math::tan(math::to_radians(field_of_view) / 2.0f);
		return {
			1.0f / (aspect_ratio * tan_fov_over_2), 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f / tan_fov_over_2, 0.0f, 0.0f,
			0.0f, 0.0f, -(far_plane + near_plane) / (far_plane - near_plane), -1.0f,
			0.0f, 0.0f, -(2.0f * far_plane * near_plane) / (far_plane - near_plane), 0.0f,						
		};
	}

	inline Mat4 orthographic_projection(f32 width, f32 height) {
		return {
			2.0f / width, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f / height, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
	}

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