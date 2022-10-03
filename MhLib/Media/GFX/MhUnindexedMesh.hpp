#ifndef MHUNINDEXEDMESH_HPP
#define MHUNINDEXEDMESH_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/GFX/MhVertexSignature.hpp>


namespace MH33 {
namespace GFX {

class UnindexedMesh {
public:
	virtual const VertexAttributeList* getVertexSignature() const = 0;
	virtual uint32_t getVertexCount() const = 0;
	virtual Handle getVertices() const = 0;
};

}
}
#endif // MHUNINDEXEDMESH_HPP
