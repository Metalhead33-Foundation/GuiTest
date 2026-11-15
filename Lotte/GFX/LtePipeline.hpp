#ifndef LTEPIPELINE_H
#define LTEPIPELINE_H
#include <Kaldi/KldCommandBuffer.hpp>
#include <Lotte/LotteLib.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
namespace Lotte {
namespace Gfx {

// TODO: Design decision.
// HLSL-to-SPIRV compiler (via GlSlang and/or DirectXShaderCompiler), shader caching on the disk, etc.
// Also TODO: Trying to think higher-level, because "Pipeline" - which is to say, Vertex Shader + Fragment Shader - is still, quite likely
// A little bit too low-level for Lotte, which intends to be somewhat more higher-level. But I digress.
// Actually, do we even want to compile HLSL to SPIR-V within Lotte? We could force it to be done purely offline too.
// But what if we need to actually generate shaders in some cases?
// What if we, at some point, change our minds and decide to invent some bespoke "generate SPIRV out of graphically Blender-like edited nodes" system?

// Until then, this remains an empty stub class, or a thin wrapper to Kaldi's shader system.

// TODO: Delete this and replace it with something else. It's temporary for easy of writing code.
// Also repeated at KaldiGL/GlSpirvShader.hpp
struct ShaderModuleCreateInfo {
	Kld::ShaderStage shaderType;
	std::vector<std::byte> source;
};
// TODO: Delete this and replace it with something else. It's temporary for easy of writing code.
// Also repeated at KaldiGL/GlSpirvShader.hpp
struct ShaderModuleCreateInfoRef {
	Kld::ShaderStage shaderType;
	std::span<std::byte> source;
};

class MH_LOTTE_API Pipeline
{
private:
	Kld::oop_detail<>::Pipeline kld_pipeline;
public:
	Pipeline();
	// TODO: Need a much better signature. Wtf is even std::span<const std::string>?
	static void prepareShaderModuleFor(Elv::Io::System& iosys, const std::span<ShaderModuleCreateInfo>& output, const std::span<const std::string>& input);
};

}
}

#endif // LTEPIPELINE_H
