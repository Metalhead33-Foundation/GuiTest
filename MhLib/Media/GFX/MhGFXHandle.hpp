#ifndef MHTGFXHANDLE_HPP
#define MHTGFXHANDLE_HPP
#include <MhLib/Media/GFX/MhGFX.hpp>
#include <cstdint>

namespace MH33 {
namespace GFX {

typedef void* NativePointer;
typedef void* D3dHandle;
typedef int32_t OpenGLint;
typedef uint32_t OpenGLUint;
typedef void* VulkanHandle;
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
typedef void* VulkanNonDispatchableHandle;
#else
typedef uint64_t VulkanNonDispatchableHandle;
#endif

union Handle {
	NativePointer ptr;
	D3dHandle d3dHandle;
	OpenGLint oglInt;
	OpenGLUint oglUint;
	VulkanHandle vkHndl;
	VulkanNonDispatchableHandle vkNdHndl;
};

}
}
#endif // MHTGFXHANDLE_HPP
