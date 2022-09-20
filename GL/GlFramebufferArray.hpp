#ifndef GLFRAMEBUFFERARRAY_HPP
#define GLFRAMEBUFFERARRAY_HPP
#include <GL/GlFramebuffer.hpp>
namespace GL {

typedef std::function<void(size_t,IFramebuffer&)> FramebufferPlusIndexAccessor;
typedef std::function<void(size_t,const IFramebuffer&)> FramebufferPlusIndexConstAccessor;
typedef std::function<void(size_t,GLuint)> FramebufferIdPlusIndexAccessor1;

template <size_t N> class FramebufferArray {
public:
	typedef std::array<GLuint,N> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufarr;
	FramebufferArray(const FramebufferArray& cpy) = delete;
	FramebufferArray& operator=(const FramebufferArray& cpy) = delete;
	FramebufferArray(FramebufferArray&& mov) = delete;
	FramebufferArray& operator=(FramebufferArray&& mov) = delete;
public:
	iterator begin() { return bufarr.begin(); }
	const_iterator begin() const { return bufarr.begin(); }
	iterator end() { return bufarr.end(); }
	const_iterator end() const { return bufarr.end(); }
	reverse_iterator rbegin() { return bufarr.rbegin(); }
	const_reverse_iterator rbegin() const { return bufarr.rbegin(); }
	reverse_iterator rend() { return bufarr.rend(); }
	const_reverse_iterator rend() const { return bufarr.rend(); }
	FramebufferArray() {
		glGenFramebuffers(N,bufarr.data());
	}
	~FramebufferArray() {
		glDeleteFramebuffers(N,bufarr.data());
	}
	FramebufferArray get(size_t i) const {
		return FramebufferRef(bufarr[i]);
	}
	FramebufferArray operator[](size_t i) const {
		return FramebufferRef(bufarr[i]);
	}
	FramebufferArray front() const {
		return FramebufferRef(bufarr.front());
	}
	FramebufferArray back() const {
		return FramebufferRef(bufarr.back());
	}
	void doActions(const IFramebuffer::FramebufferAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			auto bufbind = FramebufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IFramebuffer::FramebufferConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			auto bufbind = FramebufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IFramebuffer::FramebufferIdAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			fun(it);
		}
	}
	void doActions(const FramebufferPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			auto bufbind = FramebufferRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const FramebufferPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			auto bufbind = FramebufferRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const FramebufferIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			fun(i,bufarr[i]);
		}
	}
	size_t size() const { return bufarr.size(); }
};

template <GLenum _target, GLenum _usage, class Allocator = std::allocator<GLuint>> class FramebufferVector {
public:
	typedef std::vector<GLuint,Allocator> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufvec;
	FramebufferVector(const FramebufferVector& cpy) = delete;
	FramebufferVector& operator=(const FramebufferVector& cpy) = delete;
public:
	iterator begin() { return bufvec.begin(); }
	const_iterator begin() const { return bufvec.begin(); }
	iterator end() { return bufvec.end(); }
	const_iterator end() const { return bufvec.end(); }
	reverse_iterator rbegin() { return bufvec.rbegin(); }
	const_reverse_iterator rbegin() const { return bufvec.rbegin(); }
	reverse_iterator rend() { return bufvec.rend(); }
	const_reverse_iterator rend() const { return bufvec.rend(); }
	FramebufferVector(size_t size = 0) : bufvec(size) {
		if(size) glGenFramebuffers(size,bufvec.data());
	}
	~FramebufferVector() {
		if(bufvec.size()) glDeleteFramebuffers(bufvec.size(),bufvec.data());
	}
	FramebufferVector(FramebufferVector&& mov) : bufvec(std::move(mov.bufvec)) {

	}
	FramebufferVector& operator=(FramebufferVector&& mov) {
		this->bufvec = std::move(mov.bufvec);
		return *this;
	}
	FramebufferRef get(size_t i) const {
		return FramebufferRef(bufvec[i],_target,_usage);
	}
	FramebufferRef operator[](size_t i) const {
		return FramebufferRef(bufvec[i],_target,_usage);
	}
	FramebufferRef front() const {
		return FramebufferRef(bufvec.front(),_target,_usage);
	}
	FramebufferRef back() const {
		return FramebufferRef(bufvec.back(),_target,_usage);
	}
	void doActions(const IFramebuffer::FramebufferAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			auto bufbind = FramebufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IFramebuffer::FramebufferConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			auto bufbind = FramebufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IFramebuffer::FramebufferIdAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			fun(it);
		}
	}
	void doActions(const FramebufferPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			auto bufbind = FramebufferRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const FramebufferPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			auto bufbind = FramebufferRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const FramebufferIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(FramebufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			fun(i,bufvec[i]);
		}
	}
	size_t size() const { return bufvec.size(); }
	void push_back() {
		bufvec.push_back(0);
		glGenFramebuffers(1,&bufvec[bufvec.size()-1]);
	}
	void pop_back() {
		glDeleteFramebuffers(1,&bufvec[bufvec.size()-1]);
		bufvec.pop_back();
	}
};

}
#endif // GLFRAMEBUFFERARRAY_HPP
