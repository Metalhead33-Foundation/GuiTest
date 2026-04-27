#ifndef EUPHPIXELFORMAT_HPP
#define EUPHPIXELFORMAT_HPP
/**
 * @file EuphPixelFormat.hpp
 * @brief Declares the EuphPixelFormat API in the Euphemy/Media/Image module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Image.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <glm/glm.hpp>
#include <Elvavena/Util/half.hpp>
#include <Elvavena/Util/ElvNormDenorm.hpp>
#include <Elvavena/Util/ElvDither.hpp>
namespace Euph {
namespace Media {
namespace Image {

/**
 * @brief Generic single-channel greyscale pixel representation.
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelGreyscale {
	T pixel; ///< Stored greyscale channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	// 0.299R + 0.587G + 0.114B
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		pixel = Elv::Util::fdenormalize<T>(
					(kernel.x * 0.299f) +
					(kernel.y * 0.587f) +
					(kernel.z * 0.114f)
					);
	}
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		float normalized = Elv::Util::fnormalize(pixel);
		kernel.x = normalized;
		kernel.y = normalized;
		kernel.z = normalized;
		kernel.w = 1.0f;
	}
};
/** @brief Documents the PixelGreyscale_U8 type or declaration. */
typedef PixelGreyscale<uint8_t, Format::R8U> PixelGreyscale_U8;
/** @brief Documents the PixelGreyscale_U16 type or declaration. */
typedef PixelGreyscale<uint16_t, Format::R16U> PixelGreyscale_U16;
/** @brief Documents the PixelGreyscale_U32 type or declaration. */
typedef PixelGreyscale<uint32_t, Format::R32U> PixelGreyscale_U32;
/** @brief Documents the PixelGreyscale_U64 type or declaration. */
typedef PixelGreyscale<uint64_t, Format::INVALID> PixelGreyscale_U64;
/** @brief Documents the PixelGreyscale_S8 type or declaration. */
typedef PixelGreyscale<int8_t, Format::R8S> PixelGreyscale_S8;
/** @brief Documents the PixelGreyscale_S16 type or declaration. */
typedef PixelGreyscale<int16_t, Format::R16S> PixelGreyscale_S16;
/** @brief Documents the PixelGreyscale_S32 type or declaration. */
typedef PixelGreyscale<int32_t, Format::R32S> PixelGreyscale_S32;
/** @brief Documents the PixelGreyscale_S64 type or declaration. */
typedef PixelGreyscale<int64_t, Format::INVALID> PixelGreyscale_S64;
/** @brief Documents the PixelGreyscale_F16 type or declaration. */
typedef PixelGreyscale<half_float::half, Format::R16F> PixelGreyscale_F16;
/** @brief Documents the PixelGreyscale_F32 type or declaration. */
typedef PixelGreyscale<float, Format::R32F> PixelGreyscale_F32;
/** @brief Documents the PixelGreyscale_F64 type or declaration. */
typedef PixelGreyscale<double, Format::R64F> PixelGreyscale_F64;
/**
 * @brief Generic two-channel pixel representation (R,G).
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelRG {
	T r; ///< Red channel value.
	T g; ///< Green channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		r = Elv::Util::fdenormalize<T>(kernel.x);
		g = Elv::Util::fdenormalize<T>(kernel.y);
	}
	// Generic dithering function (enabled for non-half types)
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = Elv::Util::fnormalize(r);
		kernel.y = Elv::Util::fnormalize(g);
		kernel.z = 0.0f;
		kernel.w = 1.0f;
	}
};
/** @brief Documents the PixelRG_U8 type or declaration. */
typedef PixelRG<uint8_t, Format::RG8U> PixelRG_U8;
/** @brief Documents the PixelRG_U16 type or declaration. */
typedef PixelRG<uint16_t, Format::RG16U> PixelRG_U16;
/** @brief Documents the PixelRG_U32 type or declaration. */
typedef PixelRG<uint32_t, Format::RG32U> PixelRG_U32;
/** @brief Documents the PixelRG_U64 type or declaration. */
typedef PixelRG<uint64_t, Format::INVALID> PixelRG_U64;
/** @brief Documents the PixelRG_S8 type or declaration. */
typedef PixelRG<int8_t, Format::RG8S> PixelRG_S8;
/** @brief Documents the PixelRG_S16 type or declaration. */
typedef PixelRG<int16_t, Format::RG16S> PixelRG_S16;
/** @brief Documents the PixelRG_S32 type or declaration. */
typedef PixelRG<int32_t, Format::RG32S> PixelRG_S32;
/** @brief Documents the PixelRG_S64 type or declaration. */
typedef PixelRG<int64_t, Format::INVALID> PixelRG_S64;
/** @brief Documents the PixelRG_F16 type or declaration. */
typedef PixelRG<half_float::half, Format::RG16F> PixelRG_F16;
/** @brief Documents the PixelRG_F32 type or declaration. */
typedef PixelRG<float, Format::RG32F> PixelRG_F32;
/** @brief Documents the PixelRG_F64 type or declaration. */
typedef PixelRG<double, Format::RG64F> PixelRG_F64;

/**
 * @brief Generic three-channel pixel representation (R,G,B).
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelRGB {
	T r; ///< Red channel value.
	T g; ///< Green channel value.
	T b; ///< Blue channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		r = Elv::Util::fdenormalize<T>(kernel.x);
		g = Elv::Util::fdenormalize<T>(kernel.y);
		b = Elv::Util::fdenormalize<T>(kernel.z);
	}
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = Elv::Util::fnormalize(r);
		kernel.y = Elv::Util::fnormalize(g);
		kernel.z = Elv::Util::fnormalize(b);
		kernel.w = 1.0f;
	}
};
/** @brief Documents the PixelRGB_U8 type or declaration. */
typedef PixelRGB<uint8_t, Format::RGB8U> PixelRGB_U8;
/** @brief Documents the PixelRGB_U16 type or declaration. */
typedef PixelRGB<uint16_t, Format::RGB16U> PixelRGB_U16;
/** @brief Documents the PixelRGB_U32 type or declaration. */
typedef PixelRGB<uint32_t, Format::RGB32U> PixelRGB_U32;
/** @brief Documents the PixelRGB_U64 type or declaration. */
typedef PixelRGB<uint64_t, Format::INVALID> PixelRGB_U64;
/** @brief Documents the PixelRGB_S8 type or declaration. */
typedef PixelRGB<int8_t, Format::RGB8S> PixelRGB_S8;
/** @brief Documents the PixelRGB_S16 type or declaration. */
typedef PixelRGB<int16_t, Format::RGB16S> PixelRGB_S16;
/** @brief Documents the PixelRGB_S32 type or declaration. */
typedef PixelRGB<int32_t, Format::RGB32S> PixelRGB_S32;
/** @brief Documents the PixelRGB_S64 type or declaration. */
typedef PixelRGB<int64_t, Format::INVALID> PixelRGB_S64;
/** @brief Documents the PixelRGB_F16 type or declaration. */
typedef PixelRGB<half_float::half, Format::RGB16F> PixelRGB_F16;
/** @brief Documents the PixelRGB_F32 type or declaration. */
typedef PixelRGB<float, Format::RGB32F> PixelRGB_F32;
/** @brief Documents the PixelRGB_F64 type or declaration. */
typedef PixelRGB<double, Format::RGB64F> PixelRGB_F64;

/**
 * @brief Generic three-channel pixel representation (B,G,R memory order).
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelBGR {
	T b; ///< Blue channel value.
	T g; ///< Green channel value.
	T r; ///< Red channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		r = Elv::Util::fdenormalize<T>(kernel.x);
		g = Elv::Util::fdenormalize<T>(kernel.y);
		b = Elv::Util::fdenormalize<T>(kernel.z);
	}
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = Elv::Util::fnormalize(r);
		kernel.y = Elv::Util::fnormalize(g);
		kernel.z = Elv::Util::fnormalize(b);
		kernel.w = 1.0f;
	}
};
/** @brief Documents the PixelBGR_U8 type or declaration. */
typedef PixelBGR<uint8_t, Format::BGR8U> PixelBGR_U8;
/** @brief Documents the PixelBGR_U16 type or declaration. */
typedef PixelBGR<uint16_t, Format::BGR16U> PixelBGR_U16;
/** @brief Documents the PixelBGR_U32 type or declaration. */
typedef PixelBGR<uint32_t, Format::BGR32U> PixelBGR_U32;
/** @brief Documents the PixelBGR_U64 type or declaration. */
typedef PixelBGR<uint64_t, Format::INVALID> PixelBGR_U64;
/** @brief Documents the PixelBGR_S8 type or declaration. */
typedef PixelBGR<int8_t, Format::BGR8S> PixelBGR_S8;
/** @brief Documents the PixelBGR_S16 type or declaration. */
typedef PixelBGR<int16_t, Format::BGR16S> PixelBGR_S16;
/** @brief Documents the PixelBGR_S32 type or declaration. */
typedef PixelBGR<int32_t, Format::BGR32S> PixelBGR_S32;
/** @brief Documents the PixelBGR_S64 type or declaration. */
typedef PixelBGR<int64_t, Format::INVALID> PixelBGR_S64;
/** @brief Documents the PixelBGR_F16 type or declaration. */
typedef PixelBGR<half_float::half, Format::BGR16F> PixelBGR_F16;
/** @brief Documents the PixelBGR_F32 type or declaration. */
typedef PixelBGR<float, Format::BGR32F> PixelBGR_F32;
/** @brief Documents the PixelBGR_F64 type or declaration. */
typedef PixelBGR<double, Format::BGR64F> PixelBGR_F64;

/**
 * @brief Generic four-channel pixel representation (B,G,R,A memory order).
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelRGBA {
	T b; ///< Blue channel value.
	T g; ///< Green channel value.
	T r; ///< Red channel value.
	T a; ///< Alpha channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		r = Elv::Util::fdenormalize<T>(kernel.x);
		g = Elv::Util::fdenormalize<T>(kernel.y);
		b = Elv::Util::fdenormalize<T>(kernel.z);
		a = Elv::Util::fdenormalize<T>(kernel.w);
	}
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = Elv::Util::fnormalize(r);
		kernel.y = Elv::Util::fnormalize(g);
		kernel.z = Elv::Util::fnormalize(b);
		kernel.w = Elv::Util::fnormalize(a);
	}
};
/** @brief Documents the PixelRGBA_U8 type or declaration. */
typedef PixelRGBA<uint8_t, Format::RGBA8U> PixelRGBA_U8;
/** @brief Documents the PixelRGBA_U16 type or declaration. */
typedef PixelRGBA<uint16_t, Format::RGBA16U> PixelRGBA_U16;
/** @brief Documents the PixelRGBA_U32 type or declaration. */
typedef PixelRGBA<uint32_t, Format::RGBA32U> PixelRGBA_U32;
/** @brief Documents the PixelRGBA_U64 type or declaration. */
typedef PixelRGBA<uint64_t, Format::INVALID> PixelRGBA_U64;
/** @brief Documents the PixelRGBA_S8 type or declaration. */
typedef PixelRGBA<int8_t, Format::RGBA8S> PixelRGBA_S8;
/** @brief Documents the PixelRGBA_S16 type or declaration. */
typedef PixelRGBA<int16_t, Format::RGBA16S> PixelRGBA_S16;
/** @brief Documents the PixelRGBA_S32 type or declaration. */
typedef PixelRGBA<int32_t, Format::RGBA32S> PixelRGBA_S32;
/** @brief Documents the PixelRGBA_S64 type or declaration. */
typedef PixelRGBA<int64_t, Format::INVALID> PixelRGBA_S64;
/** @brief Documents the PixelRGBA_F16 type or declaration. */
typedef PixelRGBA<half_float::half, Format::RGBA16F> PixelRGBA_F16;
/** @brief Documents the PixelRGBA_F32 type or declaration. */
typedef PixelRGBA<float, Format::RGBA32F> PixelRGBA_F32;
/** @brief Documents the PixelRGBA_F64 type or declaration. */
typedef PixelRGBA<double, Format::RGBA64F> PixelRGBA_F64;

/**
 * @brief Generic four-channel pixel representation (B,G,R,A).
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelBGRA {
	T b; ///< Blue channel value.
	T g; ///< Green channel value.
	T r; ///< Red channel value.
	T a; ///< Alpha channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		r = Elv::Util::fdenormalize<T>(kernel.x);
		g = Elv::Util::fdenormalize<T>(kernel.y);
		b = Elv::Util::fdenormalize<T>(kernel.z);
		a = Elv::Util::fdenormalize<T>(kernel.w);
	}
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = Elv::Util::fnormalize(r);
		kernel.y = Elv::Util::fnormalize(g);
		kernel.z = Elv::Util::fnormalize(b);
		kernel.w = Elv::Util::fnormalize(a);
	}
};
/** @brief Documents the PixelBGRA_U8 type or declaration. */
typedef PixelBGRA<uint8_t, Format::BGRA8U> PixelBGRA_U8;
/** @brief Documents the PixelBGRA_U16 type or declaration. */
typedef PixelBGRA<uint16_t, Format::BGRA16U> PixelBGRA_U16;
/** @brief Documents the PixelBGRA_U32 type or declaration. */
typedef PixelBGRA<uint32_t, Format::BGRA32U> PixelBGRA_U32;
/** @brief Documents the PixelBGRA_U64 type or declaration. */
typedef PixelBGRA<uint64_t, Format::INVALID> PixelBGRA_U64;
/** @brief Documents the PixelBGRA_S8 type or declaration. */
typedef PixelBGRA<int8_t, Format::BGRA8S> PixelBGRA_S8;
/** @brief Documents the PixelBGRA_S16 type or declaration. */
typedef PixelBGRA<int16_t, Format::BGRA16S> PixelBGRA_S16;
/** @brief Documents the PixelBGRA_S32 type or declaration. */
typedef PixelBGRA<int32_t, Format::BGRA32S> PixelBGRA_S32;
/** @brief Documents the PixelBGRA_S64 type or declaration. */
typedef PixelBGRA<int64_t, Format::INVALID> PixelBGRA_S64;
/** @brief Documents the PixelBGRA_F16 type or declaration. */
typedef PixelBGRA<half_float::half, Format::BGRA16F> PixelBGRA_F16;
/** @brief Documents the PixelBGRA_F32 type or declaration. */
typedef PixelBGRA<float, Format::BGRA32F> PixelBGRA_F32;
/** @brief Documents the PixelBGRA_F64 type or declaration. */
typedef PixelBGRA<double, Format::BGRA64F> PixelBGRA_F64;

/**
 * @brief Generic four-channel pixel representation (A,R,G,B memory order).
 * @tparam T Channel storage type.
 * @tparam formatId Format identifier exposed through `FMT_ID`.
 */
template <typename T, Format formatId> struct PixelARGB {
	T a; ///< Alpha channel value.
	T r; ///< Red channel value.
	T g; ///< Green channel value.
	T b; ///< Blue channel value.
	static const Format FMT_ID = formatId; /**< Pixel format identifier. */ ///< Runtime format identifier.
	/**
	 * @brief Converts a normalized RGBA kernel to this pixel format.
	 */
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		r = Elv::Util::fdenormalize<T>(kernel.x);
		g = Elv::Util::fdenormalize<T>(kernel.y);
		b = Elv::Util::fdenormalize<T>(kernel.z);
		a = Elv::Util::fdenormalize<T>(kernel.w);
	}
	/**
	 * @brief Converts a normalized RGBA kernel using ordered dithering when supported.
	 */
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		if constexpr (std::is_same_v<T, half_float::half> || std::is_same_v<T, float> || std::is_same_v<T, double> ) {
			// For half_float, just do a regular conversion (no dithering)
			fromKernel(kernel);
		} else {
			fromKernel(Elv::Util::OrderedDither<T>::ditherUp(kernel,coords));
		}
	}
	/**
	 * @brief Converts this pixel value back into a normalized RGBA kernel.
	 */
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		kernel.x = Elv::Util::fnormalize(r);
		kernel.y = Elv::Util::fnormalize(g);
		kernel.z = Elv::Util::fnormalize(b);
		kernel.w = Elv::Util::fnormalize(a);
	}
};
/** @brief Documents the PixelARGB_U8 type or declaration. */
typedef PixelARGB<uint8_t, Format::ARGB8U> PixelARGB_U8;
/** @brief Documents the PixelARGB_U16 type or declaration. */
typedef PixelARGB<uint16_t, Format::ARGB16U> PixelARGB_U16;
/** @brief Documents the PixelARGB_U32 type or declaration. */
typedef PixelARGB<uint32_t, Format::ARGB32U> PixelARGB_U32;
/** @brief Documents the PixelARGB_U64 type or declaration. */
typedef PixelARGB<uint64_t, Format::INVALID> PixelARGB_U64;
/** @brief Documents the PixelARGB_S8 type or declaration. */
typedef PixelARGB<int8_t, Format::ARGB8S> PixelARGB_S8;
/** @brief Documents the PixelARGB_S16 type or declaration. */
typedef PixelARGB<int16_t, Format::ARGB16S> PixelARGB_S16;
/** @brief Documents the PixelARGB_S32 type or declaration. */
typedef PixelARGB<int32_t, Format::ARGB32S> PixelARGB_S32;
/** @brief Documents the PixelARGB_S64 type or declaration. */
typedef PixelARGB<int64_t, Format::INVALID> PixelARGB_S64;
/** @brief Documents the PixelARGB_F16 type or declaration. */
typedef PixelARGB<half_float::half, Format::ARGB16F> PixelARGB_F16;
/** @brief Documents the PixelARGB_F32 type or declaration. */
typedef PixelARGB<float, Format::ARGB32F> PixelARGB_F32;
/** @brief Documents the PixelARGB_F64 type or declaration. */
typedef PixelARGB<double, Format::ARGB64F> PixelARGB_F64;

/// 8-bit texture format
/// @brief Packed 8-bit RGB format using 3:3:2 channel bit allocation.
struct PixelRGB332 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t,3> Dither3; /**< Ordered dithering helper. */
	typedef Elv::Util::OrderedDither<uint8_t,3> Dither2; /**< Ordered dithering helper. */
	static constexpr const uint_fast8_t max2 = (1 << 2) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast8_t max3 = (1 << 3) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max2F = static_cast<float>(max2); /**< Precomputed packing/normalization constant. */
	static constexpr const float max2F_rec = 1.0f / max2F; /**< Precomputed packing/normalization constant. */
	static constexpr const float max3F = static_cast<float>(max3); /**< Precomputed packing/normalization constant. */
	static constexpr const float max3F_rec = 1.0f / max3F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast8_t maskB = max2; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast8_t maskG = max3 << 2; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast8_t maskR = max3 << 5; /**< Precomputed packing/normalization constant. */
	uint8_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast8_t& r, uint_fast8_t& g, uint_fast8_t& b) const {
		r = (container & maskR) >> 5;
		g = (container & maskG) >> 2;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b) {
		container = (r << 5) | (g << 2) | b;
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast8_t>(std::round(kernel.x * max3F)),
					static_cast<uint_fast8_t>(std::round(kernel.y * max3F)),
					static_cast<uint_fast8_t>(std::round(kernel.z * max2F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast8_t>(std::round( Dither3::ditherUp(kernel.x,coords) * max3F)),
		static_cast<uint_fast8_t>(std::round( Dither3::ditherUp(kernel.y,coords) * max3F)),
		static_cast<uint_fast8_t>(std::round( Dither2::ditherUp(kernel.z,coords) * max2F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
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
/// @brief Packed 16-bit RGB format using 4:4:4 channel bit allocation.
struct PixelRGB444 {
	static const Format FMT_ID = Format::RGB444; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,4> Dither4; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F = static_cast<float>(max4); /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F_rec = 1.0f / max4F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max4 << 4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max4 << 8; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		r = (container & maskR) >> 8;
		g = (container & maskG) >> 4;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (r << 8) | (g << 4) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 16-bit RGB format using 5:5:5 channel bit allocation.
struct PixelRGB555 {
	static const Format FMT_ID = Format::RGB555; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max5 << 5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5 << 10; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		r = (container & maskR) >> 10;
		g = (container & maskG) >> 5;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (r << 10) | (g << 5) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 16-bit RGB format using 5:6:5 channel bit allocation.
struct PixelRGB565 {
	static const Format FMT_ID = Format::RGB565; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,6> Dither6; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t max6 = (1 << 6) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max6F = static_cast<float>(max6); /**< Precomputed packing/normalization constant. */
	static constexpr const float max6F_rec = 1.0f / max6F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max6 << 5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5 << 11; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		r = (container & maskR) >> 11;
		g = (container & maskG) >> 5;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (r << 11) | (g << 5) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max6F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max6F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max6F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 16-bit BGR format using 5:5:5 channel bit allocation.
struct PixelBGR555 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max5 << 5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5 << 10; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		b = (container & maskB) >> 10;
		g = (container & maskG) >> 5;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (b << 8) | (g << 4) | r );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 16-bit BGR format using 5:6:5 channel bit allocation.
struct PixelBGR565 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,6> Dither6; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t max6 = (1 << 6) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max6F = static_cast<float>(max6); /**< Precomputed packing/normalization constant. */
	static constexpr const float max6F_rec = 1.0f / max6F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max6 << 5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5 << 11; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b) const {
		b = (container & maskB) >> 11;
		g = (container & maskG) >> 5;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b) {
		container = static_cast<uint16_t>( (b << 11) | (g << 5) | r );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max6F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max6F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max6F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 16-bit ARGB format using 4:4:4:4 channel bit allocation.
struct PixelARGB4444 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,4> Dither4; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F = static_cast<float>(max4); /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F_rec = 1.0f / max4F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max4 << 4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max4 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = max4 << 12; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 12;
		r = (container & maskR) >> 8;
		g = (container & maskG) >> 4;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 12) | (r << 8) | (g << 4) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
/// @brief Packed 16-bit BGRA format using 4:4:4:4 channel bit allocation.
struct PixelBGRA4444 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,4> Dither4; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F = static_cast<float>(max4); /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F_rec = 1.0f / max4F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max4 << 12; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max4 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max4 << 4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = max4; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		b = (container & maskB) >> 12;
		g = (container & maskG) >> 8;
		r = (container & maskR) >> 4;
		a = (container & maskA);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (b << 12) | (g << 8) | (r << 4) | a );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
/// @brief Packed 16-bit RGBA format using 4:4:4:4 channel bit allocation.
struct PixelRGBA4444 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,4> Dither4; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F = static_cast<float>(max4); /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F_rec = 1.0f / max4F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max4 << 12; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max4 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max4 << 4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = max4; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		r = (container & maskR) >> 12;
		b = (container & maskB) >> 8;
		g = (container & maskG) >> 4;
		a = (container & maskA);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (r << 12) | (b << 8) | (g << 4) | a );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
/// @brief Packed 16-bit ABGR format using 4:4:4:4 channel bit allocation.
struct PixelABGR4444 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,4> Dither4; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max4 = (1 << 4) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F = static_cast<float>(max4); /**< Precomputed packing/normalization constant. */
	static constexpr const float max4F_rec = 1.0f / max4F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = max4 << 12; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max4 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max4 << 4; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max4; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 12;
		b = (container & maskB) >> 8;
		g = (container & maskG) >> 4;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 12) | (b << 8) | (g << 4) | r );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max4F)),
					static_cast<uint_fast16_t>(std::round(kernel.w * max4F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.x,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.y,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.z,coords) * max4F)),
		static_cast<uint_fast16_t>(std::round( Dither4::ditherUp(kernel.w,coords) * max4F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max4F_rec;
		kernel.y = static_cast<float>(g) * max4F_rec;
		kernel.z = static_cast<float>(b) * max4F_rec;
		kernel.w = static_cast<float>(a) * max4F_rec;
	}
};
/// @brief Packed 16-bit ARGB format using 1:5:5:5 channel bit allocation.
struct PixelARGB1555 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = 1 << 15; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5 << 10; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max5 << 5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 15;
		r = (container & maskR) >> 10;
		g = (container & maskG) >> 5;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 15) | (r << 10) | (g << 5) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( Elv::Util::stippleAlpha(kernel.w, coords))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
/// @brief Packed 16-bit RGBA format using 5:5:5:1 channel bit allocation.
struct PixelRGBA5551 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = 1 << 11; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max5 << 6; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5 << 1; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = 1; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		r = (container & maskR) >> 11;
		g = (container & maskG) >> 6;
		b = (container & maskB) >> 1;
		a = (container & maskA);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (r << 11) | (g << 6) | (b << 1) | a );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( Elv::Util::stippleAlpha(kernel.w, coords))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
/// @brief Packed 16-bit ABGR format using 1:5:5:5 channel bit allocation.
struct PixelABGR1555 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = 1 << 15; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = max5 << 10; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max5 << 5; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		a = (container & maskA) >> 15;
		b = (container & maskB) >> 10;
		g = (container & maskG) >> 5;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (a << 15) | (b << 10) | (g << 5) | r );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( Elv::Util::stippleAlpha(kernel.w, coords))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint_fast16_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max5F_rec;
		kernel.y = static_cast<float>(g) * max5F_rec;
		kernel.z = static_cast<float>(b) * max5F_rec;
		kernel.w = static_cast<float>(a);
	}
};
/// @brief Packed 16-bit BGRA format using 5:5:5:1 channel bit allocation.
struct PixelBGRA5551 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint_fast16_t,5> Dither5; /**< Ordered dithering helper. */
	static constexpr const uint_fast16_t max5 = (1 << 5) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F = static_cast<float>(max5); /**< Precomputed packing/normalization constant. */
	static constexpr const float max5F_rec = 1.0f / max5F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskB = 1 << 11; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskG = max5 << 6; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskR = max5 << 1; /**< Precomputed packing/normalization constant. */
	static constexpr const uint_fast16_t maskA = 1; /**< Precomputed packing/normalization constant. */
	uint16_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint_fast16_t& r, uint_fast16_t& g, uint_fast16_t& b, uint_fast16_t& a) const {
		b = (container & maskB) >> 11;
		g = (container & maskG) >> 6;
		r = (container & maskR) >> 1;
		a = (container & maskA);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint_fast16_t r, uint_fast16_t g, uint_fast16_t b, uint_fast16_t a) {
		container = static_cast<uint16_t>( (b << 11) | (g << 6) | (r << 1) | a );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint_fast16_t>(std::round(kernel.x * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.y * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.z * max5F)),
					static_cast<uint_fast16_t>(std::round(kernel.w))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.x,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.y,coords) * max5F)),
		static_cast<uint_fast16_t>(std::round( Dither5::ditherUp(kernel.z,coords) * max5F)),
		static_cast<uint_fast16_t>( Elv::Util::stippleAlpha(kernel.w, coords))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
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
/// @brief Packed 32-bit RGBX format (unused alpha byte).
struct PixelRGBX8888 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8 << 24; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8 << 8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		r = (container & maskR) >> 16;
		g = (container & maskG) >> 8;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (r << 24u) |(g << 16u) | (b << 8u) );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 32-bit BGRX format (unused alpha byte).
struct PixelBGRX8888 {
	static const Format FMT_ID = Format::INVALID; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8 << 24; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8 << 8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (b << 24u) |(g << 16u) | (r << 8u) );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 24-bit RGB format (8 bits per color channel).
struct PixelRGB888 {
	static const Format FMT_ID = Format::RGB8U; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		r = (container & maskR) >> 16;
		g = (container & maskG) >> 8;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (r << 16u) | (g << 8u) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 24-bit BGR format (8 bits per color channel).
struct PixelBGR888 {
	static const Format FMT_ID = Format::BGR8U; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b) const {
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b) {
		container = static_cast<uint32_t>( (b << 16u) | (g << 8u) | r );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b;
		extractParts(r,g,b);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = 1.0f;
	}
};
/// @brief Packed 32-bit ARGB format (8 bits per channel).
struct PixelARGB8888 {
	static const Format FMT_ID = Format::ARGB8U; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskA = max8 << 24; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		a = (container & maskA) >> 24;
		r = (container & maskR) >> 16;
		g = (container & maskG) >> 8;
		b = (container & maskB);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (a << 24u) |(r << 16u) | (g << 8u) | b );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
/// @brief Packed 32-bit RGBA format (8 bits per channel).
struct PixelRGBA8888 {
	static const Format FMT_ID = Format::BGRA8U; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8 << 24; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskA = max8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		r = (container & maskR) >> 24;
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		a = (container & maskA);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (r << 24) |(b << 16) | (g << 8) | a );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
/// @brief Packed 32-bit ABGR format (8 bits per channel).
struct PixelABGR8888 {
	static const Format FMT_ID = Format::ARGB8U; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskA = max8 << 24; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		a = (container & maskA) >> 24;
		b = (container & maskB) >> 16;
		g = (container & maskG) >> 8;
		r = (container & maskR);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (a << 24) |(b << 16) | (g << 8) | r );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
	inline void toKernel(glm::fvec4& kernel) const {
		uint32_t r,g,b,a;
		extractParts(r,g,b,a);
		kernel.x = static_cast<float>(r) * max8F_rec;
		kernel.y = static_cast<float>(g) * max8F_rec;
		kernel.z = static_cast<float>(b) * max8F_rec;
		kernel.w = static_cast<float>(a) * max8F_rec;
	}
};
/// @brief Packed 32-bit BGRA format (8 bits per channel).
struct PixelBGRA8888 {
	static const Format FMT_ID = Format::BGRA8U; /**< Pixel format identifier. */
	typedef Elv::Util::OrderedDither<uint8_t> Dither8; /**< Ordered dithering helper. */
	static constexpr const uint32_t max8 = (1 << 8) - 1; /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F = static_cast<float>(max8); /**< Precomputed packing/normalization constant. */
	static constexpr const float max8F_rec = 1.0f / max8F; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskB = max8 << 24; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskG = max8 << 16; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskR = max8 << 8; /**< Precomputed packing/normalization constant. */
	static constexpr const uint32_t maskA = max8; /**< Precomputed packing/normalization constant. */
	uint32_t container; /**< Packed pixel storage container. */
	/**
	 *  Extracts unpacked channel values from the packed representation.
	 */
	inline void extractParts(uint32_t& r, uint32_t& g, uint32_t& b, uint32_t& a) const {
		b = (container & maskB) >> 24;
		g = (container & maskG) >> 16;
		r = (container & maskR) >> 8;
		a = (container & maskA);
	}
	/**
	 *  Packs channel values into the compact storage representation.
	 */
	inline void packParts(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
		container = static_cast<uint32_t>( (b << 24) |(g << 16) | (r << 8) | a );
	}
	/**
	 *  Converts a normalized kernel color into this pixel representation.
	 */
	inline void fromKernel(const glm::fvec4& kernel) {
		packParts(
					static_cast<uint32_t>(std::round(kernel.x * max8F)),
					static_cast<uint32_t>(std::round(kernel.y * max8F)),
					static_cast<uint32_t>(std::round(kernel.z * max8F)),
					static_cast<uint32_t>(std::round(kernel.w * max8F))
				);
	}
	/**
	 *  Converts a normalized kernel color into this representation using dithering.
	 */
	inline void fromKernelDithered(const glm::fvec4& kernel, const glm::uvec2& coords) {
		packParts(
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.x,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.y,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.z,coords) * max8F)),
		static_cast<uint32_t>(std::round( Dither8::ditherUp(kernel.w,coords) * max8F))
				);
	}
	/**
	 *  Expands this pixel representation to a normalized RGBA kernel color.
	 */
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
}
#endif // EUPHPIXELFORMAT_HPP
