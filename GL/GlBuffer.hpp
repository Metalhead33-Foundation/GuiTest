#ifndef GLBUFFER_HPP
#define GLBUFFER_HPP
#include <GL/gles2.h>
#include <mutex>
#include <span>
#include <vector>
#include <functional>

namespace GL {

struct BufferEssentials {
	static std::recursive_mutex buffermutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id, GLenum target);
};

class BufferMappedRange;
class IBuffer
{
public:
	typedef std::function<void(IBuffer&)> BufferAccessor;
	typedef std::function<void(const IBuffer&)> BufferConstAccessor;
	typedef std::function<void(GLuint)> BufferIdAccessor1;
	typedef std::function<void(GLuint,GLenum)> BufferIdAccessor2;
	typedef std::function<void(GLuint,GLenum,GLenum)> BufferIdAccessor3;
	friend class BufferMappedRange;
	virtual ~IBuffer() = default;
	void bufferData(const void* data, size_t size);
	void bufferSubData(size_t offset, const void* data, size_t size);
	void copy(IBuffer& target, size_t readOffset, size_t writeOffset, size_t size) const;
	BufferMappedRange map(GLbitfield access, size_t offset, size_t size);
	// Getter
	size_t getSize() const;
	// Bind
	void bind();
	void bind(GLenum target);
	void doActions(const BufferAccessor& fun);
	void doActions(const BufferConstAccessor& fun) const;
	void doActions(const BufferIdAccessor1& fun) const;
	void doActions(const BufferIdAccessor2& fun) const;
	void doActions(const BufferIdAccessor3& fun) const;
	// Draw
	void draw(GLenum mode, GLsizei count, GLenum type, const void * indices);
	void draw(GLenum mode, GLsizei count, GLenum type, uintptr_t indicesOffset = 0);
	// Virtual interface
	virtual GLuint getBuffId() const = 0;
	virtual GLenum getTarget() const = 0;
	virtual GLenum getUsage() const = 0;
};
class BufferMappedRange {
private:
	IBuffer* buffer;
	void* _data;
	size_t _size, _offset;
	BufferMappedRange(const BufferMappedRange& cpy) = delete;
	BufferMappedRange& operator=(const BufferMappedRange& cpy) = delete;
	BufferMappedRange(IBuffer* buffer, size_t size, size_t offset, GLbitfield access);
public:
	friend class IBuffer;
	~BufferMappedRange();
	BufferMappedRange(BufferMappedRange&& mov);
	BufferMappedRange& operator=(BufferMappedRange&& mov);
	size_t size() const;
	size_t offset() const;
	void* data();
	const void* data() const;
	std::byte& get(size_t i);
	const std::byte& get(size_t i) const;
	std::byte& operator[](size_t i);
	const std::byte& operator[](size_t i) const;
	std::span<std::byte> toSpan();
	std::span<const std::byte> toSpan() const;
	void flush();
};
class BufferRef;
class Buffer : public IBuffer
{
private:
	GLuint buffId;
	GLenum target;
	GLenum usage;
public:
	friend class BufferRef;
	explicit Buffer(GLenum target, GLenum usage);
	~Buffer() override;
	Buffer(const Buffer& cpy);
	Buffer& operator=(const Buffer& cpy);
	Buffer(Buffer&& mov);
	Buffer& operator=(Buffer&& mov);
	GLuint getBuffId() const override;
	GLenum getTarget() const override;
	GLenum getUsage() const override;
	BufferRef toRef();
};
class BufferRef : public IBuffer
{
private:
	GLuint buffId;
	GLenum target;
	GLenum usage;
public:
	explicit BufferRef(const GLuint buffId, const GLenum target, const GLenum usage);
	~BufferRef() override;
	BufferRef(const BufferRef& cpy);
	BufferRef& operator=(const BufferRef& cpy);
	GLuint getBuffId() const override;
	GLenum getTarget() const override;
	GLenum getUsage() const override;
};

template <typename T, GLenum _target, GLenum _usage> class MappedBufferT;
template <typename T, GLenum _target, GLenum _usage> class BufferT {
private:
	Buffer buffer;
	size_t _size;
public:
	static const GLenum target = _target;
	static const GLenum usage = _usage;
	friend class MappedBufferT<T,_target,_usage>;
	BufferT() : buffer(_target,_usage), _size(0) {

	}
	BufferT(const BufferT& cpy) : buffer(cpy.buffer), _size(cpy._size) {

	}
	BufferT(BufferT&& mov) : buffer(std::move(mov.buffer)), _size(mov._size) {
		mov._size = 0;
	}
	BufferT& operator=(const BufferT& cpy) { this->buffer = cpy.buffer; _size = cpy.size; return *this; }
	BufferT& operator=(BufferT&& mov) { this->buffer = std::move(mov.buffer); _size = mov._size; mov._size = 0; return *this; }
	explicit BufferT(const T* data, size_t num) : buffer(_target,_usage), _size(num) {
		buffer.bufferData(data,num * sizeof(T));
	}
	explicit BufferT(const std::span<T>& data) : buffer(_target,_usage), _size(data.size()) {
		buffer.bufferData(data.data(),data.size() * sizeof(T));
	}
	explicit BufferT(const std::span<const T>& data) : buffer(_target,_usage), _size(data.size()) {
		buffer.bufferData(data.data(),data.size() * sizeof(T));
	}
	template<class Allocator = std::allocator<T>> explicit BufferT(const std::vector<T,Allocator>& data) : buffer(_target,_usage), _size(data.size()) {
		buffer.bufferData(data.data(),data.size() * sizeof(T));
	}
	inline void copy(BufferT& targ, size_t readOffset, size_t writeOffset, size_t size) const {
		buffer.copy(targ.buffer,readOffset * sizeof(T), writeOffset * sizeof(T), size * sizeof(T));
	}
	//BufferMappedRange map(GLbitfield access, size_t offset, size_t size);
	inline void bufferData(const T* data, size_t size) {
		buffer.bufferData(data,size * sizeof(T));
		_size = size;
	}
	inline void bufferData(const std::span<T>& data) {
		buffer.bufferData(data.data(),data.size() * sizeof(T));
		_size = data.size();
	}
	inline void bufferData(const std::span<const T>& data) {
		buffer.bufferData(data.data(),data.size() * sizeof(T));
		_size = data.size();
	}
	inline void bufferData(const std::vector<const T>& data) {
		buffer.bufferData(data.data(),data.size() * sizeof(T));
		_size = data.size();
	}
	inline void bufferSubData(size_t offset, const T* data, size_t size) {
		buffer.bufferSubData(offset * sizeof(T), data, size * sizeof(T));
	}
	inline void bufferSubData(size_t offset, const std::span<T>& data) {
		buffer.bufferSubData(offset * sizeof(T), data.data(), data.size() * sizeof(T));
	}
	inline void bufferSubData(size_t offset, const std::span<const T>& data) {
		buffer.bufferSubData(offset * sizeof(T), data.data(), data.size() * sizeof(T));
	}
	inline void bufferSubData(size_t offset, const std::vector<T>& data) {
		buffer.bufferSubData(offset * sizeof(T), data.data(), data.size() * sizeof(T));
	}
	inline size_t getSize() const { return _size; }
	inline void bind() { buffer.bind(); }
	inline void bind(GLenum atarget) { buffer.bind(atarget); }
	inline GLuint getBuffId() const { return buffer.getBuffId(); }
};

}
#endif // GLBUFFER_HPP
