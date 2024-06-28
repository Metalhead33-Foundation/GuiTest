#ifndef GLHLSLCONVERSION_HPP
#define GLHLSLCONVERSION_HPP
#include <GFX/Abstract/Pipeline.hpp>
#include <MhLib/IoSys/MhIoSystem.hpp>
namespace GL {
namespace HLSL {

void initialize();
void finalize();

void prepareShaderModuleFor(MH33::Io::System& iosys, MH33::GFX::ShaderModuleCreateInfo& output, const std::string& input);
void prepareShaderModuleFor(MH33::Io::System& iosys, const std::span<MH33::GFX::ShaderModuleCreateInfo>& output, const std::span<const std::string>& input);
void convertSpirvBackToGlsl(MH33::GFX::ShaderModuleCreateInfo& output, std::vector<uint32_t>&& vec);

}
}
#endif // GLHLSLCONVERSION_HPP
