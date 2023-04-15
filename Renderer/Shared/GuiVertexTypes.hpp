#ifndef GUIVERTEXTYPES_HPP
#define GUIVERTEXTYPES_HPP
#include <glm/glm.hpp>
#include <MhLib/Media/GFX/MhVertexFormat.hpp>
namespace Renderer {

struct ColouredWidgetVert {
	glm::vec2 POS;
	static const MH33::GFX::VertexDescriptor DESCRIPTOR;
};

struct TexturedWidgetVert {
	glm::vec2 POS;
	glm::vec2 TEXCOORD;
	static const MH33::GFX::VertexDescriptor DESCRIPTOR;
};

}
#endif // GUIVERTEXTYPES_HPP
