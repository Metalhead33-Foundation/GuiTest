#ifndef MHINDEXEDMESH_HPP
#define MHINDEXEDMESH_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/GFX/MhVertexSignature.hpp>

namespace MH33 {
namespace GFX {

class IndexedMesh {
public:
	virtual const VertexAttributeList* getVertexSignature() const = 0;
	virtual uint32_t getVertexCount() const = 0;
	virtual Handle getVertices() const = 0;
	virtual uint32_t getIndexCount() const = 0;
	virtual Handle getIndices() const = 0;
};

}
}
#endif // MHINDEXEDMESH_HPP
