#include <Renderer/Shared/GuiVertexTypes.hpp>
namespace Renderer {

static const MH33::GFX::AttributeDescriptor CWVertAttributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(ColouredWidgetVert,POS) }
};
const MH33::GFX::VertexDescriptor ColouredWidgetVert::DESCRIPTOR = {
	.stride = sizeof(ColouredWidgetVert),
	.descriptors = CWVertAttributes
};
static const MH33::GFX::AttributeDescriptor TWVertAttributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(TexturedWidgetVert,POS) },
	{ .SemanticName = "TEXCOORD", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(TexturedWidgetVert,TEXCOORD) }
};
const MH33::GFX::VertexDescriptor TexturedWidgetVert::DESCRIPTOR = {
	.stride = sizeof(TexturedWidgetVert),
	.descriptors = TWVertAttributes
};


/*
struct ColouredWidgetVert {
	glm::vec2 POS;
	static MH33::GFX::VertexDescriptor* DESCRIPTOR;
};
struct TexturedWidgetVert {
	glm::vec2 POS;
	glm::vec2 TEXPOS;
	static MH33::GFX::VertexDescriptor* DESCRIPTOR;
};
*/

}
