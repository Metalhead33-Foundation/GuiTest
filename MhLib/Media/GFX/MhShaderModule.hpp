#ifndef MHSHADERMODULE_HPP
#define MHSHADERMODULE_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>

namespace MH33 {
namespace GFX {

class ShaderModule {
public:
	enum class ShaderType : uint8_t
	{
		VERTEX_SHADER,
		TESS_CONTROL_SHADER,
		TESS_EVALUATION_SHADER,
		GEOMETRY_SHADER,
		FRAGMENT_SHADER,
		COMPUTE_SHADER
	};
	virtual Handle getNativeHandle() const = 0;
	virtual ShaderType getType() const = 0;
};

}
}
#endif // MHSHADERMODULE_HPP
