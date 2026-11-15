#include "GlSpirvShader.hpp"
#include <cstring>
// This was mostly copied from the old branch's GlPipeline class.
// TODO: We'll need to put this somewhere else, so this can be shared by OpenGL and OpenGL ES
// and set GLES_IMPLEMENTATION somewhere

#ifdef GLES_IMPLEMENTATION
namespace GLES {
#else
namespace GL {
#endif

void unsetDecorations(spirv_cross::CompilerGLSL& glsl, spirv_cross::SmallVector<spirv_cross::Resource>& resource_collection, const char* fmt) {
	for (auto &resource : resource_collection) {
		unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
		unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
		printf(fmt, resource.name.c_str(), set, binding);

		// Modify the decoration to prepare it for GLSL.
		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);
		glsl.unset_decoration(resource.id, spv::DecorationBinding);
		glsl.unset_decoration(resource.id, spv::DecorationLocation);

		// Some arbitrary remapping if we want.
		//glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
	}
}

void convertSpirvBackToGlsl(ShaderModuleCreateInfo& output, std::vector<uint32_t>&& vec)
{
	spirv_cross::CompilerGLSL glsl(std::move(vec));
	spirv_cross::ShaderResources resources = glsl.get_shader_resources();
	// Get all sampled images in the shader.
	/*unsetDecorations(glsl,resources.sampled_images,"Sampled Image %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.push_constant_buffers,"Push constant buffer %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.separate_images,"Separate Image %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.separate_samplers,"Separate Sampler %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.storage_buffers,"Storage Buffer %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.uniform_buffers,"Uniform Buffer %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.gl_plain_uniforms,"Plain uniform %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.stage_inputs,"Stage input %s at set = %u, binding = %u\n");
	unsetDecorations(glsl,resources.stage_outputs,"Stage output %s at set = %u, binding = %u\n");*/
	// Set some options.
	spirv_cross::CompilerGLSL::Options options;
#ifdef GLES_IMPLEMENTATION
	options.version = 320;
	options.es = true;
	options.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Mediump;
	options.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Mediump;
#else
	options.version = 430;
	options.es = false;
#endif
	options.vertex.fixup_clipspace = true;
	options.vulkan_semantics = false;

	glsl.set_common_options(options);
	glsl.build_combined_image_samplers();
	for (auto &remap : glsl.get_combined_image_samplers())
	{
		glsl.set_name(remap.combined_id, glsl.get_name(remap.image_id));
	}

	// Compile to GLSL, ready to give to GL driver.
	std::string source = glsl.compile();
	// Debug
	output.source.resize(source.size() + 1);
	std::memcpy(output.source.data(),source.data(),source.size() + 1);
}

}