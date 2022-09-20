#ifndef GLBUFFERARRAY_HPP
#define GLBUFFERARRAY_HPP
#include <GL/GlBuffer.hpp>
#include <vector>
#include <array>
namespace GL {

typedef std::function<void(size_t,IBuffer&)> BufferPlusIndexAccessor;
typedef std::function<void(size_t,const IBuffer&)> BufferPlusIndexConstAccessor;
typedef std::function<void(size_t,GLuint)> BufferIdPlusIndexAccessor1;
typedef std::function<void(size_t,GLuint,GLenum)> BufferIdPlusIndexAccessor2;
typedef std::function<void(size_t,GLuint,GLenum,GLenum)> BufferIdPlusIndexAccessor3;

template <size_t N, GLenum _target, GLenum _usage> class BufferArray {
public:
	typedef std::array<GLuint,N> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufarr;
	BufferArray(const BufferArray& cpy) = delete;
	BufferArray& operator=(const BufferArray& cpy) = delete;
	BufferArray(BufferArray&& mov) = delete;
	BufferArray& operator=(BufferArray&& mov) = delete;
public:
	iterator begin() { return bufarr.begin(); }
	const_iterator begin() const { return bufarr.begin(); }
	iterator end() { return bufarr.end(); }
	const_iterator end() const { return bufarr.end(); }
	reverse_iterator rbegin() { return bufarr.rbegin(); }
	const_reverse_iterator rbegin() const { return bufarr.rbegin(); }
	reverse_iterator rend() { return bufarr.rend(); }
	const_reverse_iterator rend() const { return bufarr.rend(); }
	BufferArray() {
		glGenBuffers(N,bufarr.data());
	}
	~BufferArray() {
		glDeleteBuffers(N,bufarr.data());
	}
	BufferRef get(size_t i) const {
		return BufferRef(bufarr[i],_target,_usage);
	}
	BufferRef operator[](size_t i) const {
		return BufferRef(bufarr[i],_target,_usage);
	}
	BufferRef front() const {
		return BufferRef(bufarr.front(),_target,_usage);
	}
	BufferRef back() const {
		return BufferRef(bufarr.back(),_target,_usage);
	}
	void doActions(const IBuffer::BufferAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			glBindBuffer(_target,it);
			auto bufbind = BufferRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const IBuffer::BufferConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			glBindBuffer(_target,it);
			auto bufbind = BufferRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const IBuffer::BufferIdAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			glBindBuffer(_target,it);
			fun(it);
		}
	}
	void doActions(const IBuffer::BufferIdAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			glBindBuffer(_target,it);
			fun(it,_target);
		}
	}
	void doActions(const IBuffer::BufferIdAccessor3& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufarr) {
			glBindBuffer(_target,it);
			fun(it,_target,_usage);
		}
	}
	void doActions(const BufferPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindBuffer(_target,bufarr[i]);
			auto bufbind = BufferRef(bufarr[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const BufferPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindBuffer(_target,bufarr[i]);
			auto bufbind = BufferRef(bufarr[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const BufferIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindBuffer(_target,bufarr[i]);
			fun(i,bufarr[i]);
		}
	}
	void doActions(const BufferIdPlusIndexAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindBuffer(_target,bufarr[i]);
			fun(i,bufarr[i],_target);
		}
	}
	void doActions(const BufferIdPlusIndexAccessor3& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindBuffer(_target,bufarr[i]);
			fun(i,bufarr[i],_target,_usage);
		}
	}
	size_t size() const { return bufarr.size(); }
};

template <GLenum _target, GLenum _usage, class Allocator = std::allocator<GLuint>> class BufferVector {
public:
	typedef std::vector<GLuint,Allocator> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufvec;
	BufferVector(const BufferVector& cpy) = delete;
	BufferVector& operator=(const BufferVector& cpy) = delete;
public:
	iterator begin() { return bufvec.begin(); }
	const_iterator begin() const { return bufvec.begin(); }
	iterator end() { return bufvec.end(); }
	const_iterator end() const { return bufvec.end(); }
	reverse_iterator rbegin() { return bufvec.rbegin(); }
	const_reverse_iterator rbegin() const { return bufvec.rbegin(); }
	reverse_iterator rend() { return bufvec.rend(); }
	const_reverse_iterator rend() const { return bufvec.rend(); }
	BufferVector(size_t size = 0) : bufvec(size) {
		if(size) glGenBuffers(size,bufvec.data());
	}
	~BufferVector() {
		if(bufvec.size()) glDeleteBuffers(bufvec.size(),bufvec.data());
	}
	BufferVector(BufferVector&& mov) : bufvec(std::move(mov.bufvec)) {

	}
	BufferVector& operator=(BufferVector&& mov) {
		this->bufvec = std::move(mov.bufvec);
		return *this;
	}
	BufferRef get(size_t i) const {
		return BufferRef(bufvec[i],_target,_usage);
	}
	BufferRef operator[](size_t i) const {
		return BufferRef(bufvec[i],_target,_usage);
	}
	BufferRef front() const {
		return BufferRef(bufvec.front(),_target,_usage);
	}
	BufferRef back() const {
		return BufferRef(bufvec.back(),_target,_usage);
	}
	void doActions(const IBuffer::BufferAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			glBindBuffer(_target,it);
			auto bufbind = BufferRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const IBuffer::BufferConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			glBindBuffer(_target,it);
			auto bufbind = BufferRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const IBuffer::BufferIdAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			glBindBuffer(_target,it);
			fun(it);
		}
	}
	void doActions(const IBuffer::BufferIdAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			glBindBuffer(_target,it);
			fun(it,_target);
		}
	}
	void doActions(const IBuffer::BufferIdAccessor3& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(auto& it : bufvec) {
			glBindBuffer(_target,it);
			fun(it,_target,_usage);
		}
	}
	void doActions(const BufferPlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindBuffer(_target,bufvec[i]);
			auto bufbind = BufferRef(bufvec[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const BufferPlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindBuffer(_target,bufvec[i]);
			auto bufbind = BufferRef(bufvec[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const BufferIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindBuffer(_target,bufvec[i]);
			fun(i,bufvec[i]);
		}
	}
	void doActions(const BufferIdPlusIndexAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindBuffer(_target,bufvec[i]);
			fun(i,bufvec[i],_target);
		}
	}
	void doActions(const BufferIdPlusIndexAccessor3& fun) const {
		std::lock_guard<std::recursive_mutex> lock(BufferEssentials::buffermutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindBuffer(_target,bufvec[i]);
			fun(i,bufvec[i],_target,_usage);
		}
	}
	size_t size() const { return bufvec.size(); }
	void push_back() {
		bufvec.push_back(0);
		glGenBuffers(1,&bufvec[bufvec.size()-1]);
	}
	void pop_back() {
		glDeleteBuffers(1,&bufvec[bufvec.size()-1]);
		bufvec.pop_back();
	}
};

}
#endif // GLBUFFERARRAY_HPP
