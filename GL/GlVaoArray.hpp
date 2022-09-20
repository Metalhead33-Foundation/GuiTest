#ifndef GLVAOARRAY_HPP
#define GLVAOARRAY_HPP
#include <GL/GlVAO.hpp>
#include <vector>
#include <array>
namespace GL {

typedef std::function<void(size_t,IVao&)> VaoPlusIndexAccessor;
typedef std::function<void(size_t,const IVao&)> VaoPlusIndexConstAccessor;
typedef std::function<void(size_t,GLuint)> VaoIdPlusIndexAccessor;

template <size_t N> class VaoArray {
public:
	typedef std::array<GLuint,N> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufarr;
	VaoArray(const VaoArray& cpy) = delete;
	VaoArray& operator=(const VaoArray& cpy) = delete;
	VaoArray(VaoArray&& mov) = delete;
	VaoArray& operator=(VaoArray&& mov) = delete;
public:
	iterator begin() { return bufarr.begin(); }
	const_iterator begin() const { return bufarr.begin(); }
	iterator end() { return bufarr.end(); }
	const_iterator end() const { return bufarr.end(); }
	reverse_iterator rbegin() { return bufarr.rbegin(); }
	const_reverse_iterator rbegin() const { return bufarr.rbegin(); }
	reverse_iterator rend() { return bufarr.rend(); }
	const_reverse_iterator rend() const { return bufarr.rend(); }
	VaoArray() {
		glGenVaos(N,bufarr.data());
	}
	~VaoArray() {
		glDeleteVaos(N,bufarr.data());
	}
	VaoRef get(size_t i) const {
		return VaoRef(bufarr[i]);
	}
	VaoRef operator[](size_t i) const {
		return VaoRef(bufarr[i]);
	}
	VaoRef front() const {
		return VaoRef(bufarr.front());
	}
	VaoRef back() const {
		return VaoRef(bufarr.back());
	}
	void doActions(const IVao::VaoAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(auto& it : bufarr) {
			glBindVertexArray(it);
			auto bufbind = VaoRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IVao::VaoConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(auto& it : bufarr) {
			glBindVertexArray(it);
			auto bufbind = VaoRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IVao::VaoIdAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(auto& it : bufarr) {
			glBindVertexArray(it);
			fun(it);
		}
	}
	void doActions(const VaoPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindVertexArray(bufarr[i]);
			auto bufbind = VaoRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const VaoPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindVertexArray(bufarr[i]);
			auto bufbind = VaoRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const VaoIdPlusIndexAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindVertexArray(bufarr[i]);
			fun(i,bufarr[i]);
		}
	}
	size_t size() const { return bufarr.size(); }
};

template <GLenum _target, GLenum _usage, class Allocator = std::allocator<GLuint>> class VaoVector {
public:
	typedef std::vector<GLuint,Allocator> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufvec;
	VaoVector(const VaoVector& cpy) = delete;
	VaoVector& operator=(const VaoVector& cpy) = delete;
public:
	iterator begin() { return bufvec.begin(); }
	const_iterator begin() const { return bufvec.begin(); }
	iterator end() { return bufvec.end(); }
	const_iterator end() const { return bufvec.end(); }
	reverse_iterator rbegin() { return bufvec.rbegin(); }
	const_reverse_iterator rbegin() const { return bufvec.rbegin(); }
	reverse_iterator rend() { return bufvec.rend(); }
	const_reverse_iterator rend() const { return bufvec.rend(); }
	VaoVector(size_t size = 0) : bufvec(size) {
		if(size) glGenVertexArrays(size,bufvec.data());
	}
	~VaoVector() {
		if(bufvec.size()) glDeleteVertexArrays(bufvec.size(),bufvec.data());
	}
	VaoVector(VaoVector&& mov) : bufvec(std::move(mov.bufvec)) {

	}
	VaoVector& operator=(VaoVector&& mov) {
		this->bufvec = std::move(mov.bufvec);
		return *this;
	}
	VaoRef get(size_t i) const {
		return VaoRef(bufvec[i]);
	}
	VaoRef operator[](size_t i) const {
		return VaoRef(bufvec[i]);
	}
	VaoRef front() const {
		return VaoRef(bufvec.front());
	}
	VaoRef back() const {
		return VaoRef(bufvec.back());
	}
	void doActions(const IVao::VaoAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(auto& it : bufvec) {
			glBindVertexArray(it);
			auto bufbind = VaoRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IVao::VaoConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(auto& it : bufvec) {
			glBindVertexArray(it);
			auto bufbind = VaoRef(it);
			fun(bufbind);
		}
	}
	void doActions(const IVao::VaoIdAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(auto& it : bufvec) {
			glBindVertexArray(it);
			fun(it);
		}
	}
	void doActions(const VaoPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindVertexArray(bufvec[i]);
			auto bufbind = VaoRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const VaoPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindVertexArray(bufvec[i]);
			auto bufbind = VaoRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const VaoIdPlusIndexAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(VaoEssentials::vaomutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindVertexArray(bufvec[i]);
			fun(i,bufvec[i]);
		}
	}
	size_t size() const { return bufvec.size(); }
	void push_back() {
		bufvec.push_back(0);
		glGenVertexArrays(1,&bufvec[bufvec.size()-1]);
	}
	void pop_back() {
		glDeleteVertexArrays(1,&bufvec[bufvec.size()-1]);
		bufvec.pop_back();
	}
};

}
#endif // GLVAOARRAY_HPP
