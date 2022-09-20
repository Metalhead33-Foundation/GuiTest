#ifndef GLRENDERBUFFERARRAY_HPP
#define GLRENDERBUFFERARRAY_HPP
#include <GL/GlRenderbuffer.hpp>
namespace GL {

typedef std::function<void(size_t,IRenderbuffer&)> RenderbufferPlusIndexAccessor;
typedef std::function<void(size_t,const IRenderbuffer&)> RenderbufferPlusIndexConstAccessor;
typedef std::function<void(size_t,GLuint)> RenderbufferIdPlusIndexAccessor1;

template <size_t N> class RenderbufferArray {
public:
	typedef std::array<GLuint,N> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufarr;
	RenderbufferArray(const RenderbufferArray& cpy) = delete;
	RenderbufferArray& operator=(const RenderbufferArray& cpy) = delete;
	RenderbufferArray(RenderbufferArray&& mov) = delete;
	RenderbufferArray& operator=(RenderbufferArray&& mov) = delete;
public:
	iterator begin() { return bufarr.begin(); }
	const_iterator begin() const { return bufarr.begin(); }
	iterator end() { return bufarr.end(); }
	const_iterator end() const { return bufarr.end(); }
	reverse_iterator rbegin() { return bufarr.rbegin(); }
	const_reverse_iterator rbegin() const { return bufarr.rbegin(); }
	reverse_iterator rend() { return bufarr.rend(); }
	const_reverse_iterator rend() const { return bufarr.rend(); }
	RenderbufferArray() {
		glGenRenderbuffers(N,bufarr.data());
	}
	~RenderbufferArray() {
		glDeleteRenderbuffers(N,bufarr.data());
	}
	RenderbufferArray get(size_t i) const {
		return RenderbufferRef(bufarr[i]);
	}
	RenderbufferArray operator[](size_t i) const {
		return RenderbufferRef(bufarr[i]);
	}
	RenderbufferArray front() const {
		return RenderbufferRef(bufarr.front());
	}
	RenderbufferArray back() const {
		return RenderbufferRef(bufarr.back());
	}
	void doActions(const IRenderbuffer::RenderbufferAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			auto bufbind = RenderbufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IRenderbuffer::RenderbufferConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			auto bufbind = RenderbufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IRenderbuffer::RenderbufferIdAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			fun(it);
		}
	}
	void doActions(const RenderbufferPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			auto bufbind = RenderbufferRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const RenderbufferPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			auto bufbind = RenderbufferRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const RenderbufferIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			fun(i,bufarr[i]);
		}
	}
	size_t size() const { return bufarr.size(); }
};

template <GLenum _target, GLenum _usage, class Allocator = std::allocator<GLuint>> class RenderbufferVector {
public:
	typedef std::vector<GLuint,Allocator> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufvec;
	RenderbufferVector(const RenderbufferVector& cpy) = delete;
	RenderbufferVector& operator=(const RenderbufferVector& cpy) = delete;
public:
	iterator begin() { return bufvec.begin(); }
	const_iterator begin() const { return bufvec.begin(); }
	iterator end() { return bufvec.end(); }
	const_iterator end() const { return bufvec.end(); }
	reverse_iterator rbegin() { return bufvec.rbegin(); }
	const_reverse_iterator rbegin() const { return bufvec.rbegin(); }
	reverse_iterator rend() { return bufvec.rend(); }
	const_reverse_iterator rend() const { return bufvec.rend(); }
	RenderbufferVector(size_t size = 0) : bufvec(size) {
		if(size) glGenRenderbuffers(size,bufvec.data());
	}
	~RenderbufferVector() {
		if(bufvec.size()) glDeleteRenderbuffers(bufvec.size(),bufvec.data());
	}
	RenderbufferVector(RenderbufferVector&& mov) : bufvec(std::move(mov.bufvec)) {

	}
	RenderbufferVector& operator=(RenderbufferVector&& mov) {
		this->bufvec = std::move(mov.bufvec);
		return *this;
	}
	RenderbufferRef get(size_t i) const {
		return RenderbufferRef(bufvec[i],_target,_usage);
	}
	RenderbufferRef operator[](size_t i) const {
		return RenderbufferRef(bufvec[i],_target,_usage);
	}
	RenderbufferRef front() const {
		return RenderbufferRef(bufvec.front(),_target,_usage);
	}
	RenderbufferRef back() const {
		return RenderbufferRef(bufvec.back(),_target,_usage);
	}
	void doActions(const IRenderbuffer::RenderbufferAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			auto bufbind = RenderbufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IRenderbuffer::RenderbufferConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			auto bufbind = RenderbufferRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IRenderbuffer::RenderbufferIdAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			fun(it);
		}
	}
	void doActions(const RenderbufferPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			auto bufbind = RenderbufferRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const RenderbufferPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			auto bufbind = RenderbufferRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const RenderbufferIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(RenderbufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			fun(i,bufvec[i]);
		}
	}
	size_t size() const { return bufvec.size(); }
	void push_back() {
		bufvec.push_back(0);
		glGenRenderbuffers(1,&bufvec[bufvec.size()-1]);
	}
	void pop_back() {
		glDeleteRenderbuffers(1,&bufvec[bufvec.size()-1]);
		bufvec.pop_back();
	}
};

}
#endif // GLRENDERBUFFERARRAY_HPP
