#ifndef RENDERINGPIPELINE_HPP
#define RENDERINGPIPELINE_HPP
#include <functional>
#include <glm/glm.hpp>
#include "../Texture/Texture.hpp"
#include "EdgeFunction.hpp"
#include <span>

constexpr int getMiddle(int number) {
	return (number & 1) ? ((number/2)+1) : (number/2);
}

template<typename VertexInType, typename VertexOutType, typename UniformType> struct RenderingPipeline {
	typedef std::function<VertexOutType(const UniformType&, const VertexInType&)> VertexShader;
	typedef std::function<void(Texture&, const glm::ivec2&, const UniformType&, const VertexOutType&)> FragmentShader;

	UniformType uniform;
	VertexShader vert;
	FragmentShader frag;
	Texture* framebuffer;
	glm::ivec4 viewport;

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
			for(int x = xmin; x <= xmax; ++x) {
				const int y = static_cast<int>(std::round(v0.POS.y + ((static_cast<float>(x) - v0.POS.x) * slope)));
				const float w1 = (static_cast<float>(x - xmin) / xdiff);
				const float w0 = 1.0f - w1;
				const int tymin = std::clamp(y+offsetMin,viewport.y,viewport.w);
				const int tymax = std::clamp(y+offsetMax,viewport.y,viewport.w);
				const auto pixdata = VertexOutType::interpolate(v0,v1,w0,w1,false);
				for(int ty = tymin; ty < tymax; ++ty) frag(*framebuffer,glm::ivec2(x,ty),uniform, pixdata);
			}
		} else {
			const float slope = xdiff / ydiff;
			for(int y = ymin; y <= ymax; ++y) {
				const int x = static_cast<int>(std::round(v0.POS.x + ((static_cast<float>(y) - v0.POS.y) * slope)));
				const float w1 = (static_cast<float>(y - ymin) / ydiff);
				const float w0 = 1.0f - w1;
				const int txmin = std::clamp(x+offsetMin,viewport.x,viewport.z);
				const int txmax = std::clamp(x+offsetMax,viewport.x,viewport.z);
				const auto pixdata = VertexOutType::interpolate(v0,v1,w0,w1,false);
				for(int tx = txmin; tx < txmax; ++tx) frag(*framebuffer,glm::ivec2(tx,y),uniform, pixdata);
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
		if(bottomRight.x >= viewport.z) endX -= bottomRight.x - viewport.z;
		if(bottomRight.y >= viewport.w) endY -= bottomRight.y - viewport.w;

		for(int y = startY; y < endY; ++y) {
			const float rowWeight = static_cast<float>(y) * diffRecp.y;
			for(int x = startX; x < endX; ++x) {
				const float columnWeight = static_cast<float>(x) * diffRecp.x;
				frag(*framebuffer,topLeft + glm::ivec2(x,y),uniform, VertexOutType::interpolate2D(v0,v1,rowWeight,columnWeight,false));
			}
		}
	}
	void fillBottomFlatTriangle(float areaReciprocal, const VertexOutType& v0, const VertexOutType& v1, const VertexOutType& v2) {
		const float invslope1 = float(v1.POS.x - v0.POS.x) / float(v1.POS.y - v0.POS.y);
		const float invslope2 = float(v2.POS.x - v0.POS.x) / float(v2.POS.y - v0.POS.y);
		const int minY = std::clamp(int(std::trunc(v0.POS.y)),viewport[1],viewport[3]);
		const int maxY = std::clamp(int(std::trunc(v1.POS.y)),viewport[1],viewport[3]);
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
		const int minY = std::clamp(int(std::trunc(v0.POS.y)),viewport[1],viewport[3]);
		const int maxY = std::clamp(int(std::trunc(v2.POS.y)),viewport[1],viewport[3]);
		for(int i = minY; i < maxY;++i) {
			const float dy = float(i) - v2.POS.y;
			const float curx1 = v2.POS.x + (invslope1 * dy);
			const float curx2 = v2.POS.x + (invslope2 * dy);
			renderScanline(areaReciprocal,i,int(std::trunc(curx1)),int(std::trunc(curx2)),v0,v1,v2);
		}
	}
	void renderScanline(float areaReciprocal, int y, int minX, int maxX, const VertexOutType& v0, const VertexOutType& v1, const VertexOutType& v2) {
		// Clamp the scanline's left and right ends into the viewport
		minX = std::max(minX,viewport[0]);
		maxX = std::min(maxX,viewport[2]);
		// Okay, let's render!
		for(int x = minX; x < maxX; ++x) {
			const glm::fvec2 p = glm::fvec2(float(x)+0.5f,float(y)+0.5f);
			const float w0 = edgeFunction(v1.POS, v2.POS, p) * areaReciprocal;
			const float w1 = edgeFunction(v2.POS, v0.POS, p) * areaReciprocal;
			const float w2 = edgeFunction(v0.POS, v1.POS, p) * areaReciprocal;
			frag(*framebuffer,glm::ivec2(x,y),uniform, VertexOutType::interpolate(v0,v1,v2,w0,w1,w2,false));
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
		VertexOutType o0 = vert(uniform,i0);
		o0.POS.x = (((o0.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o0.POS.y = (((o0.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		VertexOutType o1 = vert(uniform,i1);
		o1.POS.x = (((o1.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o1.POS.y = (((o1.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		VertexOutType o2 = vert(uniform,i2);
		o2.POS.x = (((o2.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o2.POS.y = (((o2.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		rasterizeTriangle(o0,o1,o2);
	}
	void renderTriangles(const VertexInType* vertices, size_t vertexCount) {
		for(size_t i = 0; i < vertexCount; i += 3) {
			renderTriangle( vertices[i],vertices[i+1],vertices[i+2]);
		}
	}
	void renderTriangles(const std::span<VertexInType>& vertices) {
		for(size_t i = 0; i < vertices.size(); i += 3) {
			renderTriangle( vertices[i],vertices[i+1],vertices[i+2]);
		}
	}
	void renderTriangles(const VertexInType* vertices, const unsigned* indices, size_t indexCount) {
		for(size_t i = 0; i < indexCount; i += 3) {
			renderTriangle(vertices[indices[i]],vertices[indices[i+1]],vertices[indices[i+2]]);
		}
	}
	void renderTriangles(const VertexInType* vertices, const std::span<unsigned>& indices) {
		for(size_t i = 0; i < indices.size(); i += 3) {
			renderTriangle(vertices[indices[i]],vertices[indices[i+1]],vertices[indices[i+2]]);
		}
	}
	void renderRectangle(const VertexInType& i0, const VertexInType& i1) {
		VertexOutType o0 = vert(uniform,i0);
		o0.POS.x = (((o0.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o0.POS.y = (((o0.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		VertexOutType o1 = vert(uniform,i1);
		o1.POS.x = (((o1.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o1.POS.y = (((o1.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		fillRectangle(o0,o1);
	}
	void renderLine(const VertexInType& i0, const VertexInType& i1, int thickness = 1) {
		VertexOutType o0 = vert(uniform,i0);
		o0.POS.x = (((o0.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o0.POS.y = (((o0.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		VertexOutType o1 = vert(uniform,i1);
		o1.POS.x = (((o1.POS.x + 1.0f) * 0.5f) * static_cast<float>(viewport[2]-viewport[0])) + static_cast<float>(viewport[0]);
		o1.POS.y = (((o1.POS.y + 1.0f) * 0.5f) * static_cast<float>(viewport[3]-viewport[1])) + static_cast<float>(viewport[1]);
		fillLine(o0,o1,thickness);
	}
};

#endif // RENDERINGPIPELINE_HPP
