#ifndef GLSPIRVSHADER_HPP
#define GLSPIRVSHADER_HPP
// TODO: We'll need to put this somewhere else, so this can be shared by OpenGL and OpenGL ES
#include <Kaldi/KaldiOperation.hpp>
#include <vector>
#include <span>
#include <spirv_cross/spirv_glsl.hpp>
namespace GL {

struct ShaderModuleCreateInfo {
	Kld::ShaderStage shaderType;
	std::vector<std::byte> source;
};
struct ShaderModuleCreateInfoRef {
	Kld::ShaderStage shaderType;
	std::span<std::byte> source;
};

void unsetDecorations(spirv_cross::CompilerGLSL& glsl, spirv_cross::SmallVector<spirv_cross::Resource>& resource_collection, const char* fmt);
void convertSpirvBackToGlsl(ShaderModuleCreateInfo& output, std::vector<uint32_t>&& vec);

// Also TODO: Some kind of on-disk cache management to avoid unnecessary recompilations, if we meet a hashed binary.
// But preferably without connecting this library directly to our I/O modules.
// Maybe via callbacks? Maybe via a sort of system that registers paths, and lets other libraries handle the caches?
// Basically: we register "OpenGL" or "OpenGL ES", outside library goes "A-ha! So <documents folder>/<game folder>/ShaderCache/OpenGL! Lemme find based on the binary's hash"
// ANd if we found it, we use it, if we don't, we generate it via `convertSpirvBackToGlsl`.
// at least, that's the idea, more or less.

}

#endif // GLSPIRVSHADER_HPP
