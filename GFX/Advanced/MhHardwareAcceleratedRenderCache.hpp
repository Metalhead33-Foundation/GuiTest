#ifndef MHHARDWAREACCELERATEDRENDERCACHE_HPP
#define MHHARDWAREACCELERATEDRENDERCACHE_HPP
#include <GFX/Abstract/Pipeline.hpp>
#include <GFX/Abstract/Texture.hpp>
#include <GFX/Abstract/VertexBuffer.hpp>
#include <GFX/Abstract/GfxResourceFactory.hpp>
namespace MH33 {
namespace GFX {

template <typename VertexType, RenderType TopologyType> struct CachedPipeline {
	uint32_t primitiveCount;
	sPipeline pipeline;
	uUnindexedVertexBuffer vertexBuffer;
	std::vector<VertexType> vertexCache;
	void commitData() {
		if(vertexCache.size()) {
			vertexBuffer->bind();
			vertexBuffer->ensureDataSize(vertexCache.size() * sizeof(VertexType));
			vertexBuffer->setDataT<VertexType>(vertexCache);
			primitiveCount = vertexCache.size();
		}
	}
	void clearData() { vertexCache.clear(); }
	CachedPipeline(ResourceFactory& resFac, const sPipeline& pipeline, const VertexDescriptor* vertDescr, size_t cacheSize)
		: primitiveCount(0), pipeline(pipeline), vertexBuffer(resFac.createUnindexedVertexBuffer(VertexBufferUsageClass::Dynamic,vertDescr, cacheSize)) {
	}
	void draw() {
		if(primitiveCount) {
			pipeline->bind();
			pipeline->draw(*vertexBuffer, TopologyType, 0, primitiveCount);
			vertexBuffer->unbind();
		}
	}
};

template <typename VertexType, RenderType TopologyType> struct CachedPipelineWithTexture {
	uint32_t primitiveCount;
	sPipeline pipeline;
	uUnindexedVertexBuffer vertexBuffer;
	std::vector<VertexType> vertexCache;
	pTexture2D texture;
	uint32_t textureBindingPoint;
	void commitData() {
		if(vertexCache.size()) {
			vertexBuffer->bind();
			vertexBuffer->ensureDataSize(vertexCache.size() * sizeof(VertexType));
			vertexBuffer->setDataT<VertexType>(vertexCache);
			primitiveCount = vertexCache.size();
		}
	}
	void clearData() { vertexCache.clear(); }
	CachedPipelineWithTexture(ResourceFactory& resFac, const sPipeline& pipeline, const VertexDescriptor* vertDescr, size_t cacheSize, const std::string& textureUniformName)
		: primitiveCount(0), pipeline(pipeline), vertexBuffer(resFac.createUnindexedVertexBuffer(VertexBufferUsageClass::Dynamic,vertDescr, cacheSize)), texture(nullptr) {
			pipeline->bind();
			textureBindingPoint = pipeline->getBindingPoint(textureUniformName);
	}
	void draw() {
		if(primitiveCount) {
			pipeline->bind();
			if(texture) pipeline->setUniform(textureBindingPoint,*texture,0);
			pipeline->draw(*vertexBuffer, TopologyType, 0, primitiveCount);
			vertexBuffer->unbind();
		}
	}
};

template <typename VertexType, RenderType TopologyType> struct IndexedCachedPipeline {
	uint32_t primitiveCount;
	sPipeline pipeline;
	uIndexedVertexBuffer vertexBuffer;
	uint32_t maxIndex;
	std::vector<VertexType> vertexCache;
	std::vector<uint32_t> indexCache;
	void commitData() {
		if(vertexCache.size()) {
			vertexBuffer->bindData();
			vertexBuffer->ensureDataSize(vertexCache.size() * sizeof(VertexType));
			vertexBuffer->setDataT<VertexType>(vertexCache);
			vertexBuffer->bindIndices();
			vertexBuffer->ensureIndexCount(indexCache.size());
			vertexBuffer->setIndices(indexCache);
			primitiveCount = indexCache.size();
		}
	}
	void clearData() { vertexCache.clear(); maxIndex = 0; }
	IndexedCachedPipeline(ResourceFactory& resFac, const sPipeline& pipeline, const VertexDescriptor* vertDescr, size_t cacheSize)
		: primitiveCount(0), pipeline(pipeline), vertexBuffer(resFac.createIndexedVertexBuffer(VertexBufferUsageClass::Dynamic,vertDescr, cacheSize, cacheSize)) {
	}
	void draw() {
		if(primitiveCount) {
			pipeline->bind();
			pipeline->draw(*vertexBuffer, TopologyType, 0, primitiveCount);
			vertexBuffer->unbindData();
			vertexBuffer->unbindIndices();
		}
	}
};

template <typename VertexType, RenderType TopologyType> struct IndexedCachedPipelineWithTexture {
	uint32_t primitiveCount;
	sPipeline pipeline;
	uIndexedVertexBuffer vertexBuffer;
	uint32_t maxIndex;
	std::vector<VertexType> vertexCache;
	std::vector<uint32_t> indexCache;
	pTexture2D texture;
	uint32_t textureBindingPoint;
	void commitData() {
		if(vertexCache.size()) {
			vertexBuffer->bindData();
			vertexBuffer->ensureDataSize(vertexCache.size() * sizeof(VertexType));
			vertexBuffer->setDataT<VertexType>(vertexCache);
			vertexBuffer->bindIndices();
			vertexBuffer->ensureIndexCount(indexCache.size());
			vertexBuffer->setIndices(indexCache);
			primitiveCount = indexCache.size();
		}
	}
	void clearData() { vertexCache.clear(); maxIndex = 0; }
	IndexedCachedPipelineWithTexture(ResourceFactory& resFac, const sPipeline& pipeline, const VertexDescriptor* vertDescr, size_t cacheSize, const std::string& textureUniformName)
		: primitiveCount(0), pipeline(pipeline), vertexBuffer(resFac.createIndexedVertexBuffer(VertexBufferUsageClass::Dynamic,vertDescr, cacheSize, cacheSize)), texture(nullptr) {
		pipeline->bind();
		textureBindingPoint = pipeline->getBindingPoint(textureUniformName);
	}
	void draw() {
		if(primitiveCount) {
			pipeline->bind();
			if(texture) pipeline->setUniform(textureBindingPoint,*texture,0);
			pipeline->draw(*vertexBuffer, TopologyType, 0, primitiveCount);
			vertexBuffer->unbindData();
			vertexBuffer->unbindIndices();
		}
	}
};

}
}
#endif // MHHARDWAREACCELERATEDRENDERCACHE_HPP
