#ifndef EDGEFUNCTION_HPP
#define EDGEFUNCTION_HPP
#include <glm/glm.hpp>

namespace SoftwareRenderer {
float edgeFunction(const glm::fvec2 &a, const glm::fvec2 &b, const glm::fvec2 &c);
float edgeFunction(const glm::fvec3 &a, const glm::fvec3 &b, const glm::fvec3 &c);
float edgeFunction(const glm::fvec3 &a, const glm::fvec3 &b, const glm::fvec2 &c);
float edgeFunction(const glm::fvec4 &a, const glm::fvec4 &b, const glm::fvec2 &c);
float edgeFunction(const glm::fvec4 &a, const glm::fvec4 &b, const glm::fvec4 &c);
template <typename Vertex> float edgeFunction(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
	return edgeFunction(v0.POS,v1.POS,v2.POS);
}
}

#endif // EDGEFUNCTION_HPP
