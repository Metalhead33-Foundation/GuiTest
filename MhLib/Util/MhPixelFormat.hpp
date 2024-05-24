#ifndef PIXELFORMAT_HPP
#define PIXELFORMAT_HPP
#include <MhLib/Util/half.hpp>
#include <glm/glm.hpp>
#include "MhNormDenorm.hpp"
#include "MhDither.hpp"

namespace MH33 {
namespace Util {
template <typename T> struct PixelGreyscale {
	T pixel;
	// 0.299R + 0.587G + 0.114B
	inline void fromKernel(const glm::fvec4& kernel) {
		pixel = fdenormalize<T>(
					(kernel.x * 0.299f) +
					(kernel.y * 0.587f) +
					(kernel.z * 0.114f)
					);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		pixel = fdenormalize<T>(
					OrderedDither<T>::ditherUp(((kernel.x * 0.299f) +
							 (kernel.y * 0.587f) +
							 (kernel.z * 0.114f)
							 ),coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		float normalized = fnormalize(pixel);
		kernel.x = normalized;
		kernel.y = normalized;
		kernel.z = normalized;
		kernel.w = 1.0f;
	}
};
typedef PixelGreyscale<uint8_t> PixelGreyscale_U8;
typedef PixelGreyscale<uint16_t> PixelGreyscale_U16;
typedef PixelGreyscale<uint32_t> PixelGreyscale_U32;
typedef PixelGreyscale<uint64_t> PixelGreyscale_U64;
typedef PixelGreyscale<int8_t> PixelGreyscale_S8;
typedef PixelGreyscale<int16_t> PixelGreyscale_S16;
typedef PixelGreyscale<int32_t> PixelGreyscale_S32;
typedef PixelGreyscale<int64_t> PixelGreyscale_S64;
typedef PixelGreyscale<half_float::half> PixelGreyscale_F16;
typedef PixelGreyscale<float> PixelGreyscale_F32;
typedef PixelGreyscale<double> PixelGreyscale_F64;

template <typename T> struct PixelRG {
	T r,g;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = 0.0f;
		kernel.w = 1.0f;
	}
};
typedef PixelRG<uint8_t> PixelRG_U8;
typedef PixelRG<uint16_t> PixelRG_U16;
typedef PixelRG<uint32_t> PixelRG_U32;
template <typename T> struct PixelRGB {
	T r,g,b;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
		b = fdenormalize<T>(kernel.z);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = fnormalize(b);
		kernel.w = 1.0f;
	}
};
typedef PixelRGB<uint8_t> PixelRGB_U8;
typedef PixelRGB<uint16_t> PixelRGB_U16;
typedef PixelRGB<uint32_t> PixelRGB_U32;
template <typename T> struct PixelBGR {
	T b,g,r;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
		b = fdenormalize<T>(kernel.z);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = fnormalize(b);
		kernel.w = 1.0f;
	}
};
typedef PixelBGR<uint8_t> PixelBGR_U8;
typedef PixelBGR<uint16_t> PixelBGR_U16;
typedef PixelBGR<uint32_t> PixelBGR_U32;
template <typename T> struct PixelRGBA {
	T r,g,b,a;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
		b = fdenormalize<T>(kernel.z);
		a = fdenormalize<T>(kernel.w);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = fnormalize(b);
		kernel.w = fnormalize(a);
	}
};
typedef PixelRGBA<uint8_t> PixelRGBA_U8;
typedef PixelRGBA<uint16_t> PixelRGBA_U16;
typedef PixelRGBA<uint32_t> PixelRGBA_U32;

template <typename T> struct PixelBGRA {
	T b,g,r,a;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
		b = fdenormalize<T>(kernel.z);
		a = fdenormalize<T>(kernel.w);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = fnormalize(b);
		kernel.w = fnormalize(a);
	}
};
typedef PixelBGRA<uint8_t> PixelBGRA_U8;
typedef PixelBGRA<uint16_t> PixelBGRA_U16;
typedef PixelBGRA<uint32_t> PixelBGRA_U32;
template <typename T> struct PixelARGB {
	T a,r,g,b;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
		b = fdenormalize<T>(kernel.z);
		a = fdenormalize<T>(kernel.w);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = fnormalize(b);
		kernel.w = fnormalize(a);
	}
};
typedef PixelARGB<uint8_t> PixelARGB_U8;
typedef PixelARGB<uint16_t> PixelARGB_U16;
typedef PixelARGB<uint32_t> PixelARGB_U32;
template <typename T> struct PixelABGR {
	T a,b,g,r;
	inline void fromKernel(const glm::fvec4& kernel) {
		r = fdenormalize<T>(kernel.x);
		g = fdenormalize<T>(kernel.y);
		b = fdenormalize<T>(kernel.z);
		a = fdenormalize<T>(kernel.w);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		fromKernel(OrderedDither<T>::ditherUp(kernel,coords));
	}
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = fnormalize(r);
		kernel.y = fnormalize(g);
		kernel.z = fnormalize(b);
		kernel.w = fnormalize(a);
	}
};
typedef PixelABGR<uint8_t> PixelABGR_U8;
typedef PixelABGR<uint16_t> PixelABGR_U16;
typedef PixelABGR<uint32_t> PixelABGR_U32;

/// 8-bit texture format
struct PixelRGB332 {
	typedef OrderedDither<uint8_t,3> Dither3;
	typedef OrderedDither<uint8_t,3> Dither2;
	static constexpr const uint_fast8_t max2 = (1 << 2) - 1;
	static constexpr const uint_fast8_t max3 = (1 << 3) - 1;
	static constexpr const float max2F = static_cast<float>(max2);
	static constexpr const float max2F_rec = 1.0f / max2F;
	static constexpr const float max3F = static_cast<float>(max3);
	static constexpr const float max3F_rec = 1.0f / max3F;
	static constexpr const uint_fast8_t maskB = max2;
	static constexpr const uint_fast8_t maskG = max3 << 2;
	static constexpr const uint_fast8_t maskR = max3 << 5;
	uint8_t container;
	inline void extractParts(uint_fast8_t& r, uint_fast8_t& g, uint_fast8_t& b) const {
		r = (container & maskR) >> 5;
		g = (container & maskG) >> 2;
		b = (container & maskB);
	}
	inline void packParts(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b) {
		container = (r << 5) | (g << 2) | b;
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast8_t>(std::round(kernel.x * max3F)),
					static_cast<uint_fast8_t>(std::round(kernel.y * max3F)),
					static_cast<uint_fast8_t>(std::round(kernel.z * max2F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast8_t>(std::round( Dither3::ditherUp(kernel.x,coords) * max3F)),
		static_cast<uint_fast8_t>(std::round( Dither3::ditherUp(kernel.y,coords) * max3F)),
		static_cast<uint_fast8_t>(std::round( Dither2::ditherUp(kernel.z,coords) * max2F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast8_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max3F_rec;
		kernel.y = static_cast<float>(g) * max3F_rec;
		kernel.z = static_cast<float>(b) * max2F_rec;
		kernel.w = 1.0f;
	}
};
/// 16-bit texture formats
struct PixelRGB444 {
	typedef OrderedDither<uint_fast16_t,4> Dither4;
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1;
	static constexpr const float max4F = static_cast<float>(max4);
	static constexpr const float max4F_rec = 1.0f / max4F;
	static constexpr const uint_fast16_t maskB = max4;
	static constexpr const uint_fast16_t maskG = max4 << 4;
	static constexpr const uint_fast16_t maskR = max4 << 8;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		r = (container & maskR) >> 8;
		g = (container & maskG) >> 4;
		b = (container & maskB);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (r << 8) | (g << 4) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelRGB555 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t maskB = max5;
	static constexpr const uint_fast16_t maskG = max5 << 5;
	static constexpr const uint_fast16_t maskR = max5 << 10;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		r = (container & maskR) >> 10;
		g = (container & maskG) >> 5;
		b = (container & maskB);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (r << 10) | (g << 5) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelRGB565 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	typedef OrderedDither<uint_fast16_t,6> Dither6;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t max6 = (1 << 6) - 1;
	static constexpr const float max6F = static_cast<float>(max6);
	static constexpr const float max6F_rec = 1.0f / max6F;
	static constexpr const uint_fast16_t maskB = max5;
	static constexpr const uint_fast16_t maskG = max6 << 5;
	static constexpr const uint_fast16_t maskR = max5 << 11;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		r = (container & maskR) >> 11;
		g = (container & maskG) >> 5;
		b = (container & maskB);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (r << 11) | (g << 5) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max6F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max6F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max6F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelBGR555 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t maskB = max5;
	static constexpr const uint_fast16_t maskG = max5 << 5;
	static constexpr const uint_fast16_t maskR = max5 << 10;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		b = (container & maskB) >> 10;
		g = (container & maskG) >> 5;
		r = (container & maskR);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (b << 8) | (g << 4) | r );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelBGR565 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	typedef OrderedDither<uint_fast16_t,6> Dither6;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t max6 = (1 << 6) - 1;
	static constexpr const float max6F = static_cast<float>(max6);
	static constexpr const float max6F_rec = 1.0f / max6F;
	static constexpr const uint_fast16_t maskR = max5;
	static constexpr const uint_fast16_t maskG = max6 << 5;
	static constexpr const uint_fast16_t maskB = max5 << 11;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		b = (container & maskB) >> 11;
		g = (container & maskG) >> 5;
		r = (container & maskR);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (b << 11) | (g << 5) | r );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max6F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max6F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max6F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelARGB4444 {
	typedef OrderedDither<uint_fast16_t,4> Dither4;
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1;
	static constexpr const float max4F = static_cast<float>(max4);
	static constexpr const float max4F_rec = 1.0f / max4F;
	static constexpr const uint_fast16_t maskB = max4;
	static constexpr const uint_fast16_t maskG = max4 << 4;
	static constexpr const uint_fast16_t maskR = max4 << 8;
	static constexpr const uint_fast16_t maskA = max4 << 12;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 12;
		r = (container & maskR) >> 8;
		g = (container & maskG) >> 4;
		b = (container & maskB);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 12) | (r << 8) | (g << 4) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
struct PixelBGRA4444 {
	typedef OrderedDither<uint_fast16_t,4> Dither4;
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1;
	static constexpr const float max4F = static_cast<float>(max4);
	static constexpr const float max4F_rec = 1.0f / max4F;
	static constexpr const uint_fast16_t maskB = max4 << 12;
	static constexpr const uint_fast16_t maskG = max4 << 8;
	static constexpr const uint_fast16_t maskR = max4 << 4;
	static constexpr const uint_fast16_t maskA = max4;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		b = (container & maskB) >> 12;
		g = (container & maskG) >> 8;
		r = (container & maskR) >> 4;
		a = (container & maskA);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (b << 12) | (g << 8) | (r << 4) | a );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
struct PixelRGBA4444 {
	typedef OrderedDither<uint_fast16_t,4> Dither4;
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1;
	static constexpr const float max4F = static_cast<float>(max4);
	static constexpr const float max4F_rec = 1.0f / max4F;
	static constexpr const uint_fast16_t maskR = max4 << 12;
	static constexpr const uint_fast16_t maskB = max4 << 8;
	static constexpr const uint_fast16_t maskG = max4 << 4;
	static constexpr const uint_fast16_t maskA = max4;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		r = (container & maskR) >> 12;
		b = (container & maskB) >> 8;
		g = (container & maskG) >> 4;
		a = (container & maskA);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (r << 12) | (b << 8) | (g << 4) | a );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
struct PixelABGR4444 {
	typedef OrderedDither<uint_fast16_t,4> Dither4;
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1;
	static constexpr const float max4F = static_cast<float>(max4);
	static constexpr const float max4F_rec = 1.0f / max4F;
	static constexpr const uint_fast16_t maskA = max4 << 12;
	static constexpr const uint_fast16_t maskB = max4 << 8;
	static constexpr const uint_fast16_t maskG = max4 << 4;
	static constexpr const uint_fast16_t maskR = max4;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 12;
		b = (container & maskB) >> 8;
		g = (container & maskG) >> 4;
		r = (container & maskR);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 12) | (b << 8) | (g << 4) | r );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
struct PixelARGB1555 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t maskA = 1 << 15;
	static constexpr const uint_fast16_t maskR = max5 << 10;
	static constexpr const uint_fast16_t maskG = max5 << 5;
	static constexpr const uint_fast16_t maskB = max5;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 15;
		r = (container & maskR) >> 10;
		g = (container & maskG) >> 5;
		b = (container & maskB);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 15) | (r << 10) | (g << 5) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( stippleAlpha(kernel.w, coords))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
struct PixelRGBA5551 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t maskR = 1 << 11;
	static constexpr const uint_fast16_t maskG = max5 << 6;
	static constexpr const uint_fast16_t maskB = max5 << 1;
	static constexpr const uint_fast16_t maskA = 1;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		r = (container & maskR) >> 11;
		g = (container & maskG) >> 6;
		b = (container & maskB) >> 1;
		a = (container & maskA);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (r << 11) | (g << 6) | (b << 1) | a );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( stippleAlpha(kernel.w, coords))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
struct PixelABGR1555 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t maskA = 1 << 15;
	static constexpr const uint_fast16_t maskB = max5 << 10;
	static constexpr const uint_fast16_t maskG = max5 << 5;
	static constexpr const uint_fast16_t maskR = max5;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 15;
		b = (container & maskB) >> 10;
		g = (container & maskG) >> 5;
		r = (container & maskR);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 15) | (b << 10) | (g << 5) | r );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( stippleAlpha(kernel.w, coords))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
struct PixelBGRA5551 {
	typedef OrderedDither<uint_fast16_t,5> Dither5;
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1;
	static constexpr const float max5F = static_cast<float>(max5);
	static constexpr const float max5F_rec = 1.0f / max5F;
	static constexpr const uint_fast16_t maskB = 1 << 11;
	static constexpr const uint_fast16_t maskG = max5 << 6;
	static constexpr const uint_fast16_t maskR = max5 << 1;
	static constexpr const uint_fast16_t maskA = 1;
	uint16_t container;
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		b = (container & maskB) >> 11;
		g = (container & maskG) >> 6;
		r = (container & maskR) >> 1;
		a = (container & maskA);
	}
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (b << 11) | (g << 6) | (r << 1) | a );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( stippleAlpha(kernel.w, coords))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
/// 32-bit texture formats
struct PixelRGBX8888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskR = max8 << 24;
	static constexpr const uint32_t maskG = max8 << 16;
	static constexpr const uint32_t maskB = max8 << 8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		r = (container & maskR) >> 16;
		g = (container & maskG) >> 8;
		b = (container & maskB);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (r << 24u) |(g << 16u) | (b << 8u) );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelBGRX8888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskB = max8 << 24;
	static constexpr const uint32_t maskG = max8 << 16;
	static constexpr const uint32_t maskR = max8 << 8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		r = (container & maskR);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (b << 24u) |(g << 16u) | (r << 8u) );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelRGB888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskR = max8 << 16;
	static constexpr const uint32_t maskG = max8 << 8;
	static constexpr const uint32_t maskB = max8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		r = (container & maskR) >> 16;
		g = (container & maskG) >> 8;
		b = (container & maskB);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (r << 16u) | (g << 8u) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelBGR888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskB = max8 << 16;
	static constexpr const uint32_t maskG = max8 << 8;
	static constexpr const uint32_t maskR = max8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		r = (container & maskR);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (b << 16u) | (g << 8u) | r );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
struct PixelARGB8888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskA = max8 << 24;
	static constexpr const uint32_t maskR = max8 << 16;
	static constexpr const uint32_t maskG = max8 << 8;
	static constexpr const uint32_t maskB = max8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		a = (container & maskA) >> 24;
		r = (container & maskR) >> 16;
		g = (container & maskG) >> 8;
		b = (container & maskB);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (a << 24u) |(r << 16u) | (g << 8u) | b );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
struct PixelRGBA8888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskR = max8 << 24;
	static constexpr const uint32_t maskB = max8 << 16;
	static constexpr const uint32_t maskG = max8 << 8;
	static constexpr const uint32_t maskA = max8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		r = (container & maskR) >> 24;
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		a = (container & maskA);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (r << 24) |(b << 16) | (g << 8) | a );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
struct PixelABGR8888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskA = max8 << 24;
	static constexpr const uint32_t maskB = max8 << 16;
	static constexpr const uint32_t maskG = max8 << 8;
	static constexpr const uint32_t maskR = max8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		a = (container & maskA) >> 24;
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		r = (container & maskR);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (a << 24) |(b << 16) | (g << 8) | r );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
struct PixelBGRA8888 {
	typedef OrderedDither<uint8_t> Dither8;
	static constexpr const uint32_t max8 = (1 << 8) - 1;
	static constexpr const float max8F = static_cast<float>(max8);
	static constexpr const float max8F_rec = 1.0f / max8F;
	static constexpr const uint32_t maskB = max8 << 24;
	static constexpr const uint32_t maskG = max8 << 16;
	static constexpr const uint32_t maskR = max8 << 8;
	static constexpr const uint32_t maskA = max8;
	uint32_t container;
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		b = (container & maskB) >> 24;
		g = (container & maskG) >> 16;
		r = (container & maskR) >> 8;
		a = (container & maskA);
	}
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (b << 24) |(g << 16) | (r << 8) | a );
	}
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::ivec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
}
}

#endif // PIXELFORMAT_HPP
