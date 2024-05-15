#ifndef MHSIMD_HPP
#define MHSIMD_HPP
#include <MhLib/SIMD/private/fallback/MhFallbackVector.hpp>
#ifdef __SSE4_1__
#include <MhLib/SIMD/private/x86/MhFloat32x4_private.hpp>
#include <MhLib/SIMD/private/x86/MhInt32x4_private.hpp>
#define MHFLOAT32X4 SSE
#define MHINT32X4 SSE
#endif
#ifdef __AVX__
#include <MhLib/SIMD/private/x86/MhFloat32x8_private.hpp>
#define MHFLOAT32X8 AVX
#endif
#ifdef __AVX512VL__
#include <MhLib/SIMD/private/x86/MhFloat64x2_private.hpp>
#define MHFLOAT64X2 AVX512
#endif

namespace MH33 {
namespace SIMD {

#ifdef MHFLOAT32X4
typedef Float32x4_private Float32x4;
#else
typedef FallbackVector<float,4> Float32x4;
#endif
#ifdef MHFLOAT32X8
typedef Float32x8_private Float32x8;
#else
typedef FallbackVector<float,8> Float32x8;
#endif
#ifdef MHFLOAT64X2
typedef Float64x2_private Float64x2;
#else
typedef FallbackVector<double,2> Float64x2;
#endif
#ifdef MHINT32X4
typedef Int32x4_private Int32x4;
#else
typedef FallbackVector<int32_t,4> Int32x4;
#endif

}
}
#endif // MHSIMD_HPP
