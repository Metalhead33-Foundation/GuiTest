#ifndef MHCOLOURHELPER_HPP
#define MHCOLOURHELPER_HPP
#include <string>
#include <glm/glm.hpp>
namespace MH33 {
namespace Util {
void htmlColour(const std::string& colourname, glm::fvec4& colourKernel);
glm::fvec4 htmlColour(const std::string& colourname);
}
}
#endif // COLOURHELPER_HPP
