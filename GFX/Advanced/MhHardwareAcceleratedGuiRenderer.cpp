#include "MhHardwareAcceleratedGuiRenderer.hpp"

namespace MH33 {
namespace GFX {

// ColouredGuiVertex
const MH33::GFX::AttributeDescriptor ColouredGuiVertex::attributes[] = {
	{ .SemanticName = "POSITION", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(ColouredGuiVertex,POS) },
	{ .SemanticName = "COLOR", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x4, .offset = offsetof(ColouredGuiVertex,CLR) },
};

const MH33::GFX::VertexDescriptor ColouredGuiVertex::vertexDescriptor = {
	.stride = sizeof(ColouredGuiVertex),
	.descriptors = attributes
};

// TexturedGuiVertex
const MH33::GFX::AttributeDescriptor TexturedGuiVertex::attributes[] = {
	{ .SemanticName = "POSITION", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(TexturedGuiVertex,POS) },
	{ .SemanticName = "TEXCOORD", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(TexturedGuiVertex,TEX) }
};

const MH33::GFX::VertexDescriptor TexturedGuiVertex::vertexDescriptor = {
	.stride = sizeof(TexturedGuiVertex),
	.descriptors = attributes
};

// ColouredTexturedGuiVertex
const MH33::GFX::AttributeDescriptor ColouredTexturedGuiVertex::attributes[] = {
	{ .SemanticName = "POSITION", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(ColouredTexturedGuiVertex,POS) },
	{ .SemanticName = "TEXCOORD", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(ColouredTexturedGuiVertex,TEX) },
	{ .SemanticName = "COLOR", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x4, .offset = offsetof(ColouredTexturedGuiVertex,CLR) }
};

const MH33::GFX::VertexDescriptor ColouredTexturedGuiVertex::vertexDescriptor = {
	.stride = sizeof(ColouredTexturedGuiVertex),
	.descriptors = attributes
};

GuiRenderer::GuiRenderer(ResourceFactory& resFac, const sPipeline& colouredPipeline, const sPipeline& texturedPipeline, const sPipeline& colouredTexturedPipeline)
	: ColouredLineCache(resFac,colouredPipeline,&ColouredGuiVertex::vertexDescriptor,256),
	  ColouredTriQuadCache(resFac,colouredPipeline,&ColouredGuiVertex::vertexDescriptor,256),
	  TexturedQuadCache(resFac,texturedPipeline,&TexturedGuiVertex::vertexDescriptor,256,"spriteTexture"),
	  ColouredTexturedQuadCache(resFac,colouredTexturedPipeline,&ColouredTexturedGuiVertex::vertexDescriptor,256,"spriteTexture"),
	  lastTexture(0), lastRenderType(0), needsToFlush(false)
{

}

static const unsigned quadIds[] = { 0, 1, 2, 1, 2, 3 };

void GuiRenderer::render(const GUI::ColouredQuad& target)
{
	if(lastRenderType != 1) {
		flush();
	}
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.topLeft.y },
			.CLR = target.clr
	});
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.topLeft.y },
			.CLR = target.clr
	});
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.bottomRight.y },
			.CLR = target.clr
	});
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.bottomRight.y },
			.CLR = target.clr
	});
	for(const auto& it : quadIds) {
		ColouredTriQuadCache.indexCache.push_back(ColouredTriQuadCache.maxIndex + it);
	}
	ColouredTriQuadCache.maxIndex += 4;
	lastRenderType = 1;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::GradientQuad& target)
{
	if(lastRenderType != 1) {
		flush();
	}
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.topLeft.y },
			.CLR = target.clr[0]
	});
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.topLeft.y },
			.CLR = target.clr[1]
	});
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.bottomRight.y },
			.CLR = target.clr[2]
	});
	ColouredTriQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.bottomRight.y },
			.CLR = target.clr[3]
	});
	for(const auto& it : quadIds) {
		ColouredTriQuadCache.indexCache.push_back(ColouredTriQuadCache.maxIndex + it);
	}
	ColouredTriQuadCache.maxIndex += 4;
	lastRenderType = 1;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::ColouredTriangle& target)
{
	if(lastRenderType != 1) {
		flush();
	}
	for(const auto& it : target.pos) {
		ColouredTriQuadCache.vertexCache.push_back( {
				.POS = it,
				.CLR = target.clr
		});
		ColouredTriQuadCache.indexCache.push_back(ColouredTriQuadCache.maxIndex);
		++ColouredTriQuadCache.maxIndex;
	}
	lastRenderType = 1;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::GradientTriangle& target)
{
	if(lastRenderType != 1) {
		flush();
	}
	for(int i = 0; i < 3; ++i) {
		ColouredTriQuadCache.vertexCache.push_back( {
				.POS = target.pos[i],
				.CLR = target.clr[i]
		});
		ColouredTriQuadCache.indexCache.push_back(ColouredTriQuadCache.maxIndex);
		++ColouredTriQuadCache.maxIndex;
	}
	lastRenderType = 1;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::ColouredLine& target)
{
	if(lastRenderType != 2) {
		flush();
	}
	for(int i = 0; i < 2; ++i) {
		ColouredLineCache.vertexCache.push_back( {
				.POS = target.pos[i],
				.CLR = target.clr
		});
	}
	lastRenderType = 2;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::GradientLine& target)
{
	if(lastRenderType != 2) {
		flush();
	}
	for(int i = 0; i < 2; ++i) {
		ColouredLineCache.vertexCache.push_back( {
				.POS = target.pos[i],
				.CLR = target.clr[i]
		});
	}
	lastRenderType = 2;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::TexturedQuad& target)
{
	if(lastRenderType != 3) {
		flush();
	}
	if(lastTexture != target.texturePtr) {
		flush();
		lastTexture = target.texturePtr;
	}
	TexturedQuadCache.texture = reinterpret_cast<pTexture2D>(target.texturePtr);
	TexturedQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.topLeft.y },
			.TEX = { target.tcoord[0].x, target.tcoord[0].y }
	});
	TexturedQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.topLeft.y },
			.TEX = { target.tcoord[1].x, target.tcoord[0].y }
	});
	TexturedQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.bottomRight.y },
			.TEX = { target.tcoord[0].x, target.tcoord[1].y }
	});
	TexturedQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.bottomRight.y },
			.TEX = { target.tcoord[1].x, target.tcoord[1].y }
	});
	for(const auto& it : quadIds) {
		TexturedQuadCache.indexCache.push_back(TexturedQuadCache.maxIndex + it);
	}
	TexturedQuadCache.maxIndex += 4;
	lastRenderType = 3;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::ColouredTexturedQuad& target)
{
	if(lastRenderType != 4) {
		flush();
	}
	if(lastTexture != target.texturePtr) {
		flush();
		lastTexture = target.texturePtr;
	}
	ColouredTexturedQuadCache.texture = reinterpret_cast<pTexture2D>(target.texturePtr);
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.topLeft.y },
			.TEX = { target.tcoord[0].x, target.tcoord[0].y },
			.CLR = target.clr
	});
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.topLeft.y },
			.TEX = { target.tcoord[1].x, target.tcoord[0].y },
			.CLR = target.clr
	});
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.bottomRight.y },
			.TEX = { target.tcoord[0].x, target.tcoord[1].y },
			.CLR = target.clr
	});
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.bottomRight.y },
			.TEX = { target.tcoord[1].x, target.tcoord[1].y },
			.CLR = target.clr
	});
	for(const auto& it : quadIds) {
		ColouredTexturedQuadCache.indexCache.push_back(ColouredTexturedQuadCache.maxIndex + it);
	}
	ColouredTexturedQuadCache.maxIndex += 4;
	lastRenderType = 4;
	needsToFlush = true;
}

void GuiRenderer::render(const GUI::GradientTexturedQuad& target)
{
	if(lastRenderType != 4) {
		flush();
	}
	if(lastTexture != target.texturePtr) {
		flush();
		lastTexture = target.texturePtr;
	}
	ColouredTexturedQuadCache.texture = reinterpret_cast<pTexture2D>(target.texturePtr);
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.topLeft.y },
			.TEX = { target.tcoord[0].x, target.tcoord[0].y },
			.CLR = target.clr[0]
	});
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.topLeft.y },
			.TEX = { target.tcoord[1].x, target.tcoord[0].y },
			.CLR = target.clr[1]
	});
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.topLeft.x, target.bottomRight.y },
			.TEX = { target.tcoord[0].x, target.tcoord[1].y },
			.CLR = target.clr[2]
	});
	ColouredTexturedQuadCache.vertexCache.push_back( {
			.POS = { target.bottomRight.x, target.bottomRight.y },
			.TEX = { target.tcoord[1].x, target.tcoord[1].y },
			.CLR = target.clr[3]
	});
	for(const auto& it : quadIds) {
		ColouredTexturedQuadCache.indexCache.push_back(ColouredTexturedQuadCache.maxIndex + it);
	}
	ColouredTexturedQuadCache.maxIndex += 4;
	lastRenderType = 4;
	needsToFlush = true;
}

/*
	CachedPipeline<ColouredGuiVertex, RenderType::LINES> ColouredLineCache;
	CachedPipeline<ColouredGuiVertex, RenderType::TRIANGLES> ColouredTriQuadCache;
	CachedPipeline<TexturedGuiVertex, RenderType::TRIANGLES> TexturedQuadCache;
	CachedPipeline<ColouredTexturedGuiVertex, RenderType::TRIANGLES> ColouredTexturedQuadCache;
*/

void GuiRenderer::flush()
{
	if(needsToFlush) {
		// Coloured triangles
		ColouredTriQuadCache.commitData();
		ColouredTriQuadCache.clearData();
		ColouredTriQuadCache.draw();
		// Textured triangles
		TexturedQuadCache.commitData();
		TexturedQuadCache.clearData();
		TexturedQuadCache.draw();
		// Coloured-textured triangles
		ColouredTexturedQuadCache.commitData();
		ColouredTexturedQuadCache.clearData();
		ColouredTexturedQuadCache.draw();
		// Coloured lines
		ColouredLineCache.commitData();
		ColouredLineCache.clearData();
		ColouredLineCache.draw();
		needsToFlush = false;
	}
}

}
}
