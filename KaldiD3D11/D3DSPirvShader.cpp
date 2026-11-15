#include "D3DSPirvShader.hpp"
#include <cstring>
namespace D3D11 {

void convertSpirvBackToHlsl(ShaderModuleCreateInfo& output, std::vector<uint32_t>&& vec) {
	spirv_cross::CompilerHLSL hlsl(std::move(vec));
	spirv_cross::ShaderResources resources = hlsl.get_shader_resources();
	// Set some options.
	spirv_cross::CompilerHLSL::Options options;
	spirv_cross::CompilerGLSL::Options gloptions;
	options.shader_model = 30;
	gloptions.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Mediump;
	gloptions.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Mediump;

	hlsl.set_common_options(gloptions);
	hlsl.set_hlsl_options(options);
	// Do we actually need this? I think we needed this for GL only.
	//hlsl.build_combined_image_samplers();
	/*for (auto &remap : hlsl.get_combined_image_samplers())
	{
		hlsl.set_name(remap.combined_id, hlsl.get_name(remap.image_id));
	}*/

	// Compile to GLSL, ready to give to GL driver.
	std::string source = hlsl.compile();
	// Debug
	output.source.resize(source.size() + 1);
	std::memcpy(output.source.data(),source.data(),source.size() + 1);
}

}