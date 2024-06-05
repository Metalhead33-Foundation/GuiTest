#include "MhEdgeFunction.hpp"
namespace MH33 {
namespace Util {

float edgeFunction(const glm::fvec2 &a, const glm::fvec2 &b, const glm::fvec2 &c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}
float edgeFunction(const glm::fvec3 &a, const glm::fvec3 &b, const glm::fvec3 &c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}
float edgeFunction(const glm::fvec3 &a, const glm::fvec3 &b, const glm::fvec2 &c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}
float edgeFunction(const glm::fvec4& a, const glm::fvec4& b, const glm::fvec4& c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}
float edgeFunction(const glm::fvec4& a, const glm::fvec4& b, const glm::fvec2& c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}

}
}
