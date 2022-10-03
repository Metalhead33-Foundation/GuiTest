#ifndef COLOURHELPER_HPP
#define COLOURHELPER_HPP
#include <string>
#include <glm/glm.hpp>

void htmlColour(const std::string& colourname, glm::fvec4& colourKernel);
glm::fvec4 htmlColour(const std::string& colourname);

#endif // COLOURHELPER_HPP
