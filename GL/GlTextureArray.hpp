#ifndef GLTEXTUREARRAY_HPP
#define GLTEXTUREARRAY_HPP
#include <GL/GlTexture.hpp>
#include <vector>
#include <array>
namespace GL {

typedef std::function<void(size_t,ITexture&)> TexturePlusIndexAccessor;
typedef std::function<void(size_t,const ITexture&)> TexturePlusIndexConstAccessor;
typedef std::function<void(size_t,GLuint)> TextureIdPlusIndexAccessor1;
typedef std::function<void(size_t,GLuint,GLenum)> TextureIdPlusIndexAccessor2;

template <size_t N, GLenum _target, GLenum _usage> class TextureArray {
public:
	typedef std::array<GLuint,N> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufarr;
	TextureArray(const TextureArray& cpy) = delete;
	TextureArray& operator=(const TextureArray& cpy) = delete;
	TextureArray(TextureArray&& mov) = delete;
	TextureArray& operator=(TextureArray&& mov) = delete;
public:
	iterator begin() { return bufarr.begin(); }
	const_iterator begin() const { return bufarr.begin(); }
	iterator end() { return bufarr.end(); }
	const_iterator end() const { return bufarr.end(); }
	reverse_iterator rbegin() { return bufarr.rbegin(); }
	const_reverse_iterator rbegin() const { return bufarr.rbegin(); }
	reverse_iterator rend() { return bufarr.rend(); }
	const_reverse_iterator rend() const { return bufarr.rend(); }
	TextureArray() {
		glGenTextures(N,bufarr.data());
	}
	~TextureArray() {
		glDeleteTextures(N,bufarr.data());
	}
	TextureRef get(size_t i) const {
		return TextureRef(bufarr[i],_target,_usage);
	}
	TextureRef operator[](size_t i) const {
		return TextureRef(bufarr[i],_target,_usage);
	}
	TextureRef front() const {
		return TextureRef(bufarr.front(),_target,_usage);
	}
	TextureRef back() const {
		return TextureRef(bufarr.back(),_target,_usage);
	}
	void doActions(const ITexture::TextureAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufarr) {
			glBindTexture(_target,it);
			auto bufbind = TextureRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const ITexture::TextureConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufarr) {
			glBindTexture(_target,it);
			auto bufbind = TextureRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const ITexture::TextureIdAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufarr) {
			glBindTexture(_target,it);
			fun(it);
		}
	}
	void doActions(const ITexture::TextureIdAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufarr) {
			glBindTexture(_target,it);
			fun(it,_target);
		}
	}
	void doActions(const TexturePlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindTexture(_target,bufarr[i]);
			auto bufbind = TextureRef(bufarr[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const TexturePlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindTexture(_target,bufarr[i]);
			auto bufbind = TextureRef(bufarr[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const TextureIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindTexture(_target,bufarr[i]);
			fun(i,bufarr[i]);
		}
	}
	void doActions(const TextureIdPlusIndexAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufarr.size(); ++i) {
			glBindTexture(_target,bufarr[i]);
			fun(i,bufarr[i],_target);
		}
	}
	size_t size() const { return bufarr.size(); }
};

template <GLenum _target, GLenum _usage, class Allocator = std::allocator<GLuint>> class TextureVector {
public:
	typedef std::vector<GLuint,Allocator> VectorType;
	typedef typename VectorType::iterator iterator;
	typedef typename VectorType::const_iterator const_iterator;
	typedef typename VectorType::reverse_iterator reverse_iterator;
	typedef typename VectorType::const_reverse_iterator const_reverse_iterator;
private:
	VectorType bufvec;
	TextureVector(const TextureVector& cpy) = delete;
	TextureVector& operator=(const TextureVector& cpy) = delete;
public:
	iterator begin() { return bufvec.begin(); }
	const_iterator begin() const { return bufvec.begin(); }
	iterator end() { return bufvec.end(); }
	const_iterator end() const { return bufvec.end(); }
	reverse_iterator rbegin() { return bufvec.rbegin(); }
	const_reverse_iterator rbegin() const { return bufvec.rbegin(); }
	reverse_iterator rend() { return bufvec.rend(); }
	const_reverse_iterator rend() const { return bufvec.rend(); }
	TextureVector(size_t size = 0) : bufvec(size) {
		if(size) glGenTextures(size,bufvec.data());
	}
	~TextureVector() {
		if(bufvec.size()) glDeleteTextures(bufvec.size(),bufvec.data());
	}
	TextureVector(TextureVector&& mov) : bufvec(std::move(mov.bufvec)) {

	}
	TextureVector& operator=(TextureVector&& mov) {
		this->bufvec = std::move(mov.bufvec);
		return *this;
	}
	TextureRef get(size_t i) const {
		return TextureRef(bufvec[i],_target,_usage);
	}
	TextureRef operator[](size_t i) const {
		return TextureRef(bufvec[i],_target,_usage);
	}
	TextureRef front() const {
		return TextureRef(bufvec.front(),_target,_usage);
	}
	TextureRef back() const {
		return TextureRef(bufvec.back(),_target,_usage);
	}
	void doActions(const ITexture::TextureAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufvec) {
			glBindTexture(_target,it);
			auto bufbind = TextureRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const ITexture::TextureConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufvec) {
			glBindTexture(_target,it);
			auto bufbind = TextureRef(it,_target,_usage);
			fun(bufbind);
		}
	}
	void doActions(const ITexture::TextureIdAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufvec) {
			glBindTexture(_target,it);
			fun(it);
		}
	}
	void doActions(const ITexture::TextureIdAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(auto& it : bufvec) {
			glBindTexture(_target,it);
			fun(it,_target);
		}
	}
	void doActions(const TexturePlusIndexAccessor& fun) {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindTexture(_target,bufvec[i]);
			auto bufbind = TextureRef(bufvec[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const TexturePlusIndexConstAccessor& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindTexture(_target,bufvec[i]);
			auto bufbind = TextureRef(bufvec[i],_target,_usage);
			fun(i,bufbind);
		}
	}
	void doActions(const TextureIdPlusIndexAccessor1& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindTexture(_target,bufvec[i]);
			fun(i,bufvec[i]);
		}
	}
	void doActions(const TextureIdPlusIndexAccessor2& fun) const {
		std::lock_guard<std::recursive_mutex> lock(TextureEssentials::texturemutex);
		for(size_t i = 0; i < bufvec.size(); ++i) {
			glBindTexture(_target,bufvec[i]);
			fun(i,bufvec[i],_target);
		}
	}
	size_t size() const { return bufvec.size(); }
	void push_back() {
		bufvec.push_back(0);
		glGenTextures(1,&bufvec[bufvec.size()-1]);
	}
	void pop_back() {
		glDeleteTextures(1,&bufvec[bufvec.size()-1]);
		bufvec.pop_back();
	}
};

}
#endif // GLTEXTUREARRAY_HPP
