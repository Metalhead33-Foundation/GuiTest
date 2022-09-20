#ifndef GLSAMPLERARRAY_HPP
#define GLSAMPLERARRAY_HPP
#include <GL/GlSampler.hpp>
namespace GL {

typedef std::function<void(size_t,ISampler&)> SamplerPlusIndexAccessor;
typedef std::function<void(size_t,const ISampler&)> SamplerPlusIndexConstAccessor;
typedef std::function<void(size_t,GLuint)> SamplerIdPlusIndexAccessor1;

template <size_t N> class SamplerArray {
public:
	typedef std::array<GLuint,N> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufarr;
	SamplerArray(const SamplerArray& cpy) = delete;
	SamplerArray& operator=(const SamplerArray& cpy) = delete;
	SamplerArray(SamplerArray&& mov) = delete;
	SamplerArray& operator=(SamplerArray&& mov) = delete;
public:
	iterator begin() { return bufarr.begin(); }
	const_iterator begin() const { return bufarr.begin(); }
	iterator end() { return bufarr.end(); }
	const_iterator end() const { return bufarr.end(); }
	reverse_iterator rbegin() { return bufarr.rbegin(); }
	const_reverse_iterator rbegin() const { return bufarr.rbegin(); }
	reverse_iterator rend() { return bufarr.rend(); }
	const_reverse_iterator rend() const { return bufarr.rend(); }
	SamplerArray() {
		glGenSamplers(N,bufarr.data());
	}
	~SamplerArray() {
		glDeleteSamplers(N,bufarr.data());
	}
	SamplerArray get(size_t i) const {
		return SamplerRef(bufarr[i]);
	}
	SamplerArray operator[](size_t i) const {
		return SamplerRef(bufarr[i]);
	}
	SamplerArray front() const {
		return SamplerRef(bufarr.front());
	}
	SamplerArray back() const {
		return SamplerRef(bufarr.back());
	}
	void doActions(const ISampler::SamplerUser& fun) {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(auto& it : bufarr) {
			auto bufbind = SamplerRef(it);
			fun(bufbind);
		}
	}
	void doActions(const ISampler::SamplerConstUser& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(auto& it : bufarr) {
			auto bufbind = SamplerRef(it);
			fun(bufbind);
		}
	}
	void doActions(const ISampler::SamplerIdUser& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(auto& it : bufarr) {
			fun(it);
		}
	}
	void doActions(const SamplerPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			auto bufbind = SamplerRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const SamplerPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			auto bufbind = SamplerRef(bufarr[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const SamplerIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			fun(i,bufarr[i]);
		}
	}
	size_t size() const { return bufarr.size(); }
};

template <GLenum _target, GLenum _usage, class Allocator = std::allocator<GLuint>> class SamplerVector {
public:
	typedef std::vector<GLuint,Allocator> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufvec;
	SamplerVector(const SamplerVector& cpy) = delete;
	SamplerVector& operator=(const SamplerVector& cpy) = delete;
public:
	iterator begin() { return bufvec.begin(); }
	const_iterator begin() const { return bufvec.begin(); }
	iterator end() { return bufvec.end(); }
	const_iterator end() const { return bufvec.end(); }
	reverse_iterator rbegin() { return bufvec.rbegin(); }
	const_reverse_iterator rbegin() const { return bufvec.rbegin(); }
	reverse_iterator rend() { return bufvec.rend(); }
	const_reverse_iterator rend() const { return bufvec.rend(); }
	SamplerVector(size_t size = 0) : bufvec(size) {
		if(size) glGenSamplers(size,bufvec.data());
	}
	~SamplerVector() {
		if(bufvec.size()) glDeleteSamplers(bufvec.size(),bufvec.data());
	}
	SamplerVector(SamplerVector&& mov) : bufvec(std::move(mov.bufvec)) {

	}
	SamplerVector& operator=(SamplerVector&& mov) {
		this->bufvec = std::move(mov.bufvec);
		return *this;
	}
	SamplerRef get(size_t i) const {
		return SamplerRef(bufvec[i],_target,_usage);
	}
	SamplerRef operator[](size_t i) const {
		return SamplerRef(bufvec[i],_target,_usage);
	}
	SamplerRef front() const {
		return SamplerRef(bufvec.front(),_target,_usage);
	}
	SamplerRef back() const {
		return SamplerRef(bufvec.back(),_target,_usage);
	}
	void doActions(const ISampler::SamplerUser& fun) {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(auto& it : bufvec) {
			auto bufbind = SamplerRef(it);
			fun(bufbind);
		}
	}
	void doActions(const ISampler::SamplerConstUser& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(auto& it : bufvec) {
			auto bufbind = SamplerRef(it);
			fun(bufbind);
		}
	}
	void doActions(const ISampler::SamplerIdUser& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(auto& it : bufvec) {
			fun(it);
		}
	}
	void doActions(const SamplerPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			auto bufbind = SamplerRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const SamplerPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			auto bufbind = SamplerRef(bufvec[i]);
			fun(i,bufbind);
		}
	}
	void doActions(const SamplerIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(SamplerEssentials::shadermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			fun(i,bufvec[i]);
		}
	}
	size_t size() const { return bufvec.size(); }
	void push_back() {
		bufvec.push_back(0);
		glGenSamplers(1,&bufvec[bufvec.size()-1]);
	}
	void pop_back() {
		glDeleteSamplers(1,&bufvec[bufvec.size()-1]);
		bufvec.pop_back();
	}
};

}
#endif // GLSAMPLERARRAY_HPP
