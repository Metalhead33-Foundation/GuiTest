#ifndef MHPIPELINE_HPP
#define MHPIPELINE_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/GFX/MhVertexSignature.hpp>
#include <MhLib/Media/GFX/MhShaderModule.hpp>
#include <MhLib/Media/GFX/MhIndexedMesh.hpp>
#include <MhLib/Media/GFX/MhUnindexedMesh.hpp>
#include <MhLib/Media/GFX/MhTexture1D.hpp>
#include <MhLib/Media/GFX/MhTexture2D.hpp>
#include <MhLib/Media/GFX/MhTexture3D.hpp>
#include <MhLib/Media/GFX/MhTextureArray1D.hpp>
#include <MhLib/Media/GFX/MhTextureArray2D.hpp>
#include <MhLib/Media/GFX/MhCubemap.hpp>

namespace MH33 {
namespace GFX {

class Pipeline {
public:
	virtual Handle getNativeHandle() const = 0;
	virtual const VertexAttributeList* getVertexSignature() const = 0;
	virtual bool attachShaderModule(ShaderModule& module) = 0;
	virtual void assemble() = 0;
	virtual void draw(const UnindexedMesh& mesh) const = 0;
	virtual void draw(const IndexedMesh& mesh) const = 0;
	// Okay, let's setup things
	enum Culling {
		Back, Front
	};
	virtual Culling getCulling() const = 0;
	virtual void setCulling(Culling nculling) = 0;
	// Okay, now let's add stuff!
	virtual void assign(uint32_t id, const Texture1D& tex) = 0;
	virtual void assign(uint32_t id, const Texture2D& tex) = 0;
	virtual void assign(uint32_t id, const Texture3D& tex) = 0;
	virtual void assign(uint32_t id, const TextureArray1D& tex) = 0;
	virtual void assign(uint32_t id, const TextureArray2D& tex) = 0;
	virtual void assign(uint32_t id, const Cubemap& tex) = 0;
	virtual void assign(uint32_t id, const Handle& uniform) = 0;
};

}
}
#endif // MHPIPELINE_HPP
