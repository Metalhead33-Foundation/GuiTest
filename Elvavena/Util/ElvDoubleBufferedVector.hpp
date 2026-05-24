#ifndef ELVDOUBLEBUFFEREDVECTOR_HPP
#define ELVDOUBLEBUFFEREDVECTOR_HPP
#include <vector>
#include <memory>
#include <mutex>
#include <span>
namespace Elv {
namespace Util {

template <typename T, typename Allocator = std::allocator<T>> struct DoubleBufferedVector {
public:
	typedef Allocator ElementAllocator;
	typedef std::vector<T,Allocator> Vector;
	typedef std::mutex Mutex;
	typedef std::scoped_lock<Mutex> ScopedLock;
	typedef std::span<T> Span;
	typedef std::span<const T> ConstSpan;
private:
	Vector readBuffer, writeBuffer;
	mutable Mutex readMutex;
	mutable Mutex writeMutex;
public:
	// Constructors and destructors
	~DoubleBufferedVector() = default;
	DoubleBufferedVector(const DoubleBufferedVector& cpy)
	{
		std::scoped_lock lock(cpy.readMutex, cpy.writeMutex, readMutex, writeMutex);
		this->readBuffer = cpy.readBuffer;
		this->writeBuffer = cpy.writeBuffer;
	}
	DoubleBufferedVector(DoubleBufferedVector&& mov)
	{
		std::scoped_lock lock(mov.readMutex, mov.writeMutex, readMutex, writeMutex);
		this->readBuffer = std::move(mov.readBuffer);
		this->writeBuffer = std::move(mov.writeBuffer);
	}
	DoubleBufferedVector& operator=(const DoubleBufferedVector& cpy) {
		if (this == &cpy) return *this;
		std::scoped_lock lock(cpy.readMutex, cpy.writeMutex, readMutex, writeMutex);
		this->readBuffer = cpy.readBuffer;
		this->writeBuffer = cpy.writeBuffer;
		return *this;
	}
	DoubleBufferedVector& operator=(DoubleBufferedVector&& mov) {
		if (this == &mov) return *this;
		std::scoped_lock lock(mov.readMutex, mov.writeMutex, readMutex, writeMutex);
		this->readBuffer = std::move(mov.readBuffer);
		this->writeBuffer = std::move(mov.writeBuffer);
		return *this;
	}
	DoubleBufferedVector(const ElementAllocator& allocator = ElementAllocator())
		: readBuffer(allocator), writeBuffer(allocator)
	{}
	DoubleBufferedVector(size_t initialReserve, const ElementAllocator& allocator = ElementAllocator())
		: readBuffer(allocator), writeBuffer(allocator)
	{
		readBuffer.reserve(initialReserve);
		writeBuffer.reserve(initialReserve);
	}
	///! these functions are NOT thread-safe, but I am still leaving them here, in case a programmer knows what he is doing
	///! The actual Doxygen-compatible documentation in the future will be issuing explicit warnings about these 4 functions.
	///! Let's just assume that programmers are adults who know how to read, m'kay?
	Vector& getReadBuffer_unsafe() { return readBuffer; }
	Vector& getWriteBuffer_unsafe() { return writeBuffer; }
	const Vector& getReadBuffer_unsafe() const { return readBuffer; }
	const Vector& getWriteBuffer_unsafe() const { return writeBuffer; }
	ConstSpan getReadBufferAsSpan_unsafe() const { return readBuffer; }
	Span getReadBufferAsSpan_unsafe() { return readBuffer; }
	ConstSpan getWriteBufferAsSpan_unsafe() const { return writeBuffer; }
	Span getWriteBufferAsSpan_unsafe() { return writeBuffer; }
	///! These functiosn ARE thread-safe.
	void swapBuffers() {
		std::scoped_lock lock(readMutex, writeMutex);
		std::swap(readBuffer, writeBuffer);
		writeBuffer.clear();
	}
	// To be fair, using the earlier principle, I COULD theoretically expose write operations on read buffers too, as opposed to just write buffers...
	// ... because I trust programmers. But fuck it: the read-buffer is semantically read-only in principle.
	// We won't allow writing into it. Use getReadBuffer() if you really want to. Though you shouldn't.
	// Anyway, this will be the main interface via which the "user" will be interacing with the buffers, plus swapBuffers().
	template <typename Func> void onReadBuffer(Func&& func) const {
		std::scoped_lock lock(readMutex);
		func(readBuffer);
	};
	template <typename Func> void onWriteBuffer(Func&& func) {
		std::scoped_lock lock(writeMutex);
		func(writeBuffer);
	};
	// Some convenience stuff
	void push_back(const T& elem) {
		std::scoped_lock lock(writeMutex);
		writeBuffer.push_back(elem);
	}
	void push_back(T&& elem) {
		std::scoped_lock lock(writeMutex);
		writeBuffer.push_back(std::move(elem));
	}

};

}
}
#endif // ELVDOUBLEBUFFEREDVECTOR_HPP