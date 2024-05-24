#ifndef GLCONTEXT_HPP
#define GLCONTEXT_HPP

namespace GL {

struct Context {
	virtual ~Context() = default;
	virtual void makeCurrent() = 0;
	virtual void swapBuffers() = 0;
};

}

#endif // GLCONTEXT_HPP
