#ifndef D3DSPIRVSHADER_HPP
#define D3DSPIRVSHADER_HPP
#include <spirv_cross/spirv_hlsl.hpp>
// TODO: We'll need to put this somewhere else, so this can be shared by D3D11 and D3D12
#include <Kaldi/KaldiOperation.hpp>
#include <vector>
#include <span>
namespace D3D11 {

struct ShaderModuleCreateInfo {
	Kld::ShaderStage shaderType;
	std::vector<std::byte> source;
};
struct ShaderModuleCreateInfoRef {
	Kld::ShaderStage shaderType;
	std::span<std::byte> source;
};
void convertSpirvBackToHlsl(ShaderModuleCreateInfo& output, std::vector<uint32_t>&& vec);

}
#endif // D3DSPIRVSHADER_HPP
