#ifndef RENDERINGPIPELINE_HPP
#define RENDERINGPIPELINE_HPP
#include <functional>
#include <glm/glm.hpp>
#include <Renderer/Software/SoftTexture.hpp>
#include <Renderer/Software/EdgeFunction.hpp>
#include <span>
#include <MhLib/Media/GFX/MhPipeline.hpp>

namespace Renderer {
namespace Software {
constexpr int getMiddle(int number) {
	return (number & 1) ? ((number/2)+1) : (number/2);
}

struct RenderingContext {
	sTexture framebuffer;
	glm::ivec4 viewport;
	TextureFiltering filtering;
	AlphaBlending blending;
	Wrap wrapping;
	const Texture* texture_units[8];
};
extern RenderingContext CONTEXT;

template<typename VertexInType, typename VertexOutType, typename UniformType> struct RenderingPipeline {
	typedef std::function<VertexOutType(const UniformType&, const VertexInType&)> VertexShader;
	typedef std::function<void(RenderingContext&, const glm::ivec2&, const UniformType&, const VertexOutType&)> FragmentShader;

	const UniformType* uniform;
	VertexShader vert;
	FragmentShader frag;
	RenderingContext* context;

	void fillLine(const VertexOutType& v0, const VertexOutType& v1, int thickness = 1) {
		const int offsetMax = getMiddle(thickness);
		const int offsetMin = offsetMax-thickness;
		const float xdiff = (v1.POS.x - v0.POS.x);
		const float ydiff = (v1.POS.y - v0.POS.y);
		const int xmin = static_cast<int>(std::round(std::min(v0.POS.x,v1.POS.x)));
		const int xmax = static_cast<int>(std::round(std::max(v0.POS.x,v1.POS.x)));
		const int ymin = static_cast<int>(std::round(std::min(v0.POS.y,v1.POS.y)));
		const int ymax = static_cast<int>(std::round(std::max(v0.POS.y,v1.POS.y)));
		if(std::abs(xdiff) > std::abs(ydiff)) {
			const float slope = ydiff / xdiff;
#pragma omp parallel for
			for(int x = xmin; x <= xmax; ++x) {
				const int y = static_cast<int>(std::round(v0.POS.y + ((static_cast<float>(x) - v0.POS.x) * slope)));
				const float w1 = (static_cast<float>(x - xmin) / xdiff);
				const float w0 = 1.0f - w1;
				const int tymin = std::clamp(y+offsetMin,context->viewport.y,context->viewport.w);
				const int tymax = std::clamp(y+offsetMax,context->viewport.y,context->viewport.w);
				const auto pixdata = VertexOutType::interpolate(v0,v1,w0,w1,false);
#pragma omp parallel for
				for(int ty = tymin; ty < tymax; ++ty) frag(*context,glm::ivec2(x,ty),*uniform, pixdata);
			}
		} else {
			const float slope = xdiff / ydiff;
#pragma omp parallel for
			for(int y = ymin; y <= ymax; ++y) {
				const int x = static_cast<int>(std::round(v0.POS.x + ((static_cast<float>(y) - v0.POS.y) * slope)));
				const float w1 = (static_cast<float>(y - ymin) / ydiff);
				const float w0 = 1.0f - w1;
				const int txmin = std::clamp(x+offsetMin,context->viewport.x,context->viewport.z);
				const int txmax = std::clamp(x+offsetMax,context->viewport.x,context->viewport.z);
				const auto pixdata = VertexOutType::interpolate(v0,v1,w0,w1,false);
#pragma omp parallel for
				for(int tx = txmin; tx < txmax; ++tx) frag(*context,glm::ivec2(tx,y),*uniform, pixdata);
			}
		}
	}
	void fillRectangle(const VertexOutType& v0, const VertexOutType& v1) {
		const glm::ivec2 topLeft = glm::ivec2(
						static_cast<int>(std::min(v0.POS.x,v1.POS.x)),
						static_cast<int>(std::min(v0.POS.y,v1.POS.y))
					);
		const glm::ivec2 bottomRight = glm::ivec2(
					static_cast<int>(std::max(v0.POS.x,v1.POS.x)),
					static_cast<int>(std::max(v0.POS.y,v1.POS.y))
					);
		glm::ivec2 diff = glm::fvec2((bottomRight.x - topLeft.x),
										(bottomRight.y - topLeft.y));
		const glm::fvec2 diffRecp = glm::fvec2(1.0f / static_cast<float>(diff.x),
											 1.0f / static_cast<float>(diff.y));

		int startX = 0;
		int startY = 0;
		int endX = diff.x;
		int endY = diff.y;

		if(topLeft.x < 0) startX = std::abs(topLeft.x);
		if(topLeft.y < 0) startY = std::abs(topLeft.y);
		if(bottomRight.x >= context->viewport.z) endX -= bottomRight.x - context->viewport.z;
		if(bottomRight.y >= context->viewport.w) endY -= bottomRight.y - context->viewport.w;

#pragma omp parallel for
		for(int y = startY; y < endY; ++y) {
			const float rowWeight = static_cast<float>(y) * diffRecp.y;
#pragma omp parallel for
			for(int x = startX; x < endX; ++x) {
				const float columnWeight = static_cast<float>(x) * diffRecp.x;
				frag(*context,topLeft + glm::ivec2(x,y),*uniform, VertexOutType::interpolate2D(v0,v1,rowWeight,columnWeight,false));
			}
		}
	}
	void fillBottomFlatTriangle(float areaReciprocal, const VertexOutType& v0, const VertexOutType& v1, const VertexOutType& v2) {
		const float invslope1 = float(v1.POS.x - v0.POS.x) / float(v1.POS.y - v0.POS.y);
		const float invslope2 = float(v2.POS.x - v0.POS.x) / float(v2.POS.y - v0.POS.y);
		const int minY = std::clamp(int(std::trunc(v0.POS.y)),context->viewport.y,context->viewport.w);
		const int maxY = std::clamp(int(std::trunc(v1.POS.y)),context->viewport.y,context->viewport.w);
#pragma omp parallel for
		for(int i = minY; i < maxY;++i) {
			const float dy = float(i) - v0.POS.y;
			const float curx1 = v0.POS.x + (invslope1 * dy);
			const float curx2 = v0.POS.x + (invslope2 * dy);
			renderScanline(areaReciprocal,i,int(std::trunc(curx1)),int(std::trunc(curx2)),v0,v1,v2);
		}
	}
	void fillTopFlatTriangle(float areaReciprocal, const VertexOutType& v0, const VertexOutType& v1, const VertexOutType& v2) {
		const float invslope1 = float(v2.POS.x - v0.POS.x) / float(v2.POS.y - v0.POS.y);
		const float invslope2 = float(v2.POS.x - v1.POS.x) / float(v2.POS.y - v1.POS.y);
		const int minY = std::clamp(int(std::trunc(v0.POS.y)),context->viewport.y,context->viewport.w);
		const int maxY = std::clamp(int(std::trunc(v2.POS.y)),context->viewport.y,context->viewport.w);
#pragma omp parallel for
		for(int i = minY; i < maxY;++i) {
			const float dy = float(i) - v2.POS.y;
			const float curx1 = v2.POS.x + (invslope1 * dy);
			const float curx2 = v2.POS.x + (invslope2 * dy);
			renderScanline(areaReciprocal,i,int(std::trunc(curx1)),int(std::trunc(curx2)),v0,v1,v2);
		}
	}
	void renderScanline(float areaReciprocal, int y, int minX, int maxX, const VertexOutType& v0, const VertexOutType& v1, const VertexOutType& v2) {
		// Clamp the scanline's left and right ends into the viewport
		minX = std::max(minX,context->viewport.x);
		maxX = std::min(maxX,context->viewport.z);
		// Okay, let's render!
#pragma omp parallel for
		for(int x = minX; x < maxX; ++x) {
			const glm::fvec2 p = glm::fvec2(float(x)+0.5f,float(y)+0.5f);
			const float w0 = edgeFunction(v1.POS, v2.POS, p) * areaReciprocal;
			const float w1 = edgeFunction(v2.POS, v0.POS, p) * areaReciprocal;
			const float w2 = edgeFunction(v0.POS, v1.POS, p) * areaReciprocal;
			frag(*context,glm::ivec2(x,y),*uniform, VertexOutType::interpolate(v0,v1,v2,w0,w1,w2,false));
		}
	}

	void rasterizeTriangle(const VertexOutType& v0, const VertexOutType& v1, const VertexOutType& v2) {
		const VertexOutType *t = &v0;
		const VertexOutType *m = &v1;
		const VertexOutType *b = &v2;

		// Sort by Y
		if (t->POS.y > m->POS.y) std::swap(t, m);
		if (m->POS.y > b->POS.y) std::swap(m, b);
		if (t->POS.y > m->POS.y) std::swap(t, m);
		const float dy = (b->POS.y - t->POS.y);
		const float iy = (m->POS.y - t->POS.y);
		if (m->POS.y == t->POS.y)
		{
			const VertexOutType *l = m, *r = t;
			if (l->POS.x > r->POS.x) std::swap(l, r);
			const float area = edgeFunction(*t,*r,*b);
			const float rArea = 1.0f / area;
			fillTopFlatTriangle(rArea, *l, *r, *b);
		}
		else if (m->POS.y == b->POS.y)
		{
		const VertexOutType *l = m, *r = b;
			if (l->POS.x > r->POS.x) std::swap(l, r);
			const float area = edgeFunction(*t,*l,*r);
			const float rArea = 1.0f / area;
			fillBottomFlatTriangle(rArea, *t, *l, *r);
		}
		else
		  {
			// Split the triangle
			VertexOutType v4 = VertexOutType::split(*t,*m,*b,dy,iy);
			const VertexOutType *l = m, *r = &v4;
			if (l->POS.x > r->POS.x) std::swap(l, r);
			const float area1 = edgeFunction(*t,*l,*r);
			const float area2 = edgeFunction(*l,*r,*b);
			const float rArea1 = 1.0f / area1;
			const float rArea2 = 1.0f / area2;
			fillBottomFlatTriangle(rArea1, *t, *l, *r);
			fillTopFlatTriangle(rArea2, *l, *r, *b);
		  }
	}
	void renderTriangle(const VertexInType& i0, const VertexInType& i1, const VertexInType& i2) {
		VertexOutType o0 = vert(*uniform,i0);
		o0.POS.x = (((o0.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o0.POS.y = (((-o0.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		VertexOutType o1 = vert(*uniform,i1);
		o1.POS.x = (((o1.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o1.POS.y = (((-o1.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		VertexOutType o2 = vert(*uniform,i2);
		o2.POS.x = (((o2.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o2.POS.y = (((-o2.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		rasterizeTriangle(o0,o1,o2);
	}
	void renderTriangles(const VertexInType* vertices, size_t vertexCount) {
		for(size_t i = 0; i < vertexCount; i += 3) {
			renderTriangle( vertices[i],vertices[i+1],vertices[i+2]);
		}
	}
	void renderTriangles(const std::span<const VertexInType>& vertices) {
		for(size_t i = 0; i < vertices.size(); i += 3) {
			renderTriangle( vertices[i],vertices[i+1],vertices[i+2]);
		}
	}
	void renderTriangles(const VertexInType* vertices, const unsigned* indices, size_t indexCount) {
		for(size_t i = 0; i < indexCount; i += 3) {
			renderTriangle(vertices[indices[i]],vertices[indices[i+1]],vertices[indices[i+2]]);
		}
	}
	void renderTriangles(const VertexInType* vertices, const std::span<const unsigned>& indices) {
		for(size_t i = 0; i < indices.size(); i += 3) {
			renderTriangle(vertices[indices[i]],vertices[indices[i+1]],vertices[indices[i+2]]);
		}
	}
	void renderTriangles(const std::span<const VertexInType>& vertices, const std::span<const unsigned>& indices) {
		for(size_t i = 0; i < indices.size(); i += 3) {
			renderTriangle(vertices[indices[i]],vertices[indices[i+1]],vertices[indices[i+2]]);
		}
	}
	void renderRectangle(const VertexInType& i0, const VertexInType& i1) {
		VertexOutType o0 = vert(*uniform,i0);
		o0.POS.x = (((o0.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o0.POS.y = (((-o0.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		VertexOutType o1 = vert(*uniform,i1);
		o1.POS.x = (((o1.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o1.POS.y = (((-o1.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		fillRectangle(o0,o1);
	}
	void renderLine(const VertexInType& i0, const VertexInType& i1, int thickness = 1) {
		VertexOutType o0 = vert(*uniform,i0);
		o0.POS.x = (((o0.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o0.POS.y = (((-o0.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		VertexOutType o1 = vert(*uniform,i1);
		o1.POS.x = (((o1.POS.x + 1.0f) * 0.5f) * static_cast<float>(context->viewport.z-context->viewport.x)) + static_cast<float>(context->viewport.x);
		o1.POS.y = (((-o1.POS.y + 1.0f) * 0.5f) * static_cast<float>(context->viewport.w-context->viewport.y)) + static_cast<float>(context->viewport.y);
		fillLine(o0,o1,thickness);
	}
	void renderLines(const VertexInType* vertices, size_t vertexCount, int thickness = 1) {
		for(size_t i = 0; i < vertexCount; i += 2) {
			renderLine( vertices[i],vertices[i+1], thickness);
		}
	}
	void renderLines(const std::span<const VertexInType>& vertices, int thickness = 1) {
		for(size_t i = 0; i < vertices.size(); i += 2) {
			renderLine( vertices[i],vertices[i+1], thickness);
		}
	}
	void renderLines(const VertexInType* vertices, const unsigned* indices, size_t indexCount, int thickness = 1) {
		for(size_t i = 0; i < indexCount; i += 2) {
			renderLine(vertices[indices[i]],vertices[indices[i+1]], thickness);
		}
	}
	void renderLines(const VertexInType* vertices, const std::span<const unsigned>& indices, int thickness = 1) {
		for(size_t i = 0; i < indices.size(); i += 2) {
			renderLine(vertices[indices[i]],vertices[indices[i+1]], thickness);
		}
	}
	void renderLines(const std::span<const VertexInType>& vertices, const std::span<const unsigned>& indices, int thickness = 1) {
		for(size_t i = 0; i < indices.size(); i += 2) {
			renderLine(vertices[indices[i]],vertices[indices[i+1]], thickness);
		}
	}
};

template<typename VertexInType, typename VertexOutType, typename UniformType> class Pipeline : public MH33::GFX::Pipeline {
public:
	typedef RenderingPipeline<VertexInType,VertexOutType,UniformType> _Pipeline;
protected:
	_Pipeline _pipeline;
public:
	MH33::GFX::Handle getNativeHandle() override { return { .ptr = this }; }
	MH33::GFX::ConstHandle getNativeHandle() const override { return { .ptr = this }; }
	virtual void setTexture(int id, const MH33::GFX::Texture2D& texture) = 0;
	virtual void setTexture(int id, const MH33::GFX::Texture3D& texture) = 0;
	virtual void setTexture(int id, const MH33::GFX::ArrayTexture2D& texture) = 0;
	virtual void setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture) = 0;
	virtual int getId(const std::string& uniformName) = 0;
	void drawTriangles(MH33::GFX::UnindexedMesh& mesh) override {
		if(mesh.getVertexDescriptor() == &(VertexInType::DESCRIPTOR))
		{
			const std::span<const VertexInType> vertices(reinterpret_cast<const VertexInType*>(mesh.getNativeHandle().ptr),mesh.getVertexCount());
			_pipeline.renderTriangles(vertices);
		}
	}
	void drawTriangles(MH33::GFX::IndexedMesh& mesh) override {
		if(mesh.getVertexDescriptor() == &(VertexInType::DESCRIPTOR))
		{
			const std::span<const uint32_t> indices(reinterpret_cast<const uint32_t*>(mesh.getIndexNativeHandle().ptr),mesh.getIndexCount());
			const std::span<const VertexInType> vertices(reinterpret_cast<const VertexInType*>(mesh.getVertexNativeHandle().ptr),mesh.getVertexCount());
			_pipeline.renderTriangles(vertices,indices);
		}
	}
	void drawLines(MH33::GFX::UnindexedMesh& mesh, float thickness) override {
		if(mesh.getVertexDescriptor() == &(VertexInType::DESCRIPTOR))
		{
			const std::span<const VertexInType> vertices(reinterpret_cast<const VertexInType*>(mesh.getNativeHandle().ptr),mesh.getVertexCount());
			_pipeline.renderLines(vertices, static_cast<int>( std::round(thickness)) );
		}
	}
	void drawLines(MH33::GFX::IndexedMesh& mesh, float thickness) override {
		if(mesh.getVertexDescriptor() == &(VertexInType::DESCRIPTOR))
		{
			const std::span<const uint32_t> indices(reinterpret_cast<const uint32_t*>(mesh.getIndexNativeHandle().ptr),mesh.getIndexCount());
			const std::span<const VertexInType> vertices(reinterpret_cast<const VertexInType*>(mesh.getVertexNativeHandle().ptr),mesh.getVertexCount());
			_pipeline.renderLines(vertices,indices, static_cast<int>(std::round(thickness)));
		}
	}
};

}
}
#endif // RENDERINGPIPELINE_HPP
