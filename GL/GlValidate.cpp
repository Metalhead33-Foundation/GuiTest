#include <GL/GlValidate.hpp>
#include <GL/gles2.h>
#include <exception>
#include <sstream>

namespace GL {

class Exception : public std::exception {
private:
	GLenum err;
	std::string str;
public:
	explicit Exception(GLenum err) : err(err) {
		std::stringstream sstrm;
		sstrm << "0x" << std::hex << err << ' ';
		switch (err) {
			case GL_INVALID_ENUM: sstrm << "GL_INVALID_ENUM"; break;
			case GL_INVALID_VALUE: sstrm << "GL_INVALID_VALUE"; break;
			case GL_INVALID_OPERATION: sstrm << "GL_INVALID_OPERATION"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: sstrm << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
			case GL_OUT_OF_MEMORY: sstrm << "GL_OUT_OF_MEMORY"; break;
			default: break;
		}
		str = sstrm.str();
	}
	const char* what() const noexcept override { return str.c_str(); }
};


void Validate::validate()
{
	GLenum err = glGetError();
	if(err != GL_NO_ERROR)
	{
		throw Exception(err);
	}
}



}
