#include "EuphBufferDevice.hpp"
#include <cstring>
namespace Euph {
namespace Io {

ReadOnlyBufferWrapper::ReadOnlyBufferWrapper(const void* ptr, size_t buffSize)
	: ptr(static_cast<const std::byte*>(ptr)), cursor(0), buffSize(buffSize)
{

}

ReadOnlyBufferWrapper::ReadOnlyBufferWrapper(const std::span<const std::byte>& span)
	: ptr(span.data()), cursor(0), buffSize(span.size_bytes())
{

}

size_t ReadOnlyBufferWrapper::read(void* buffer, size_t size, size_t count)
{
	const size_t bytesToRead = std::min(size * count, buffSize - cursor);
	std::memcpy(buffer, &ptr[cursor], bytesToRead);
	cursor += bytesToRead;
	return bytesToRead / size;
}

size_t ReadOnlyBufferWrapper::write(const void* buffer, size_t size, size_t count)
{
	(void)buffer;
	(void)size;
	(void)count;
	return 0;
}

int ReadOnlyBufferWrapper::seek(long offset, Elv::Io::SeekOrigin whence)
{
	switch (whence) {
		case Elv::Io::SeekOrigin::SET:
			cursor = offset;
			break;
		case Elv::Io::SeekOrigin::CUR:
			cursor += offset;
			break;
		case Elv::Io::SeekOrigin::END:
			cursor = static_cast<intptr_t>(buffSize) + offset;
			break;
	}
	return 0;
}

long ReadOnlyBufferWrapper::tell()
{
	return static_cast<long>(cursor);
}

size_t ReadOnlyBufferWrapper::size()
{
	return buffSize;
}

bool ReadOnlyBufferWrapper::eof()
{
	return cursor == static_cast<intptr_t>(buffSize);
}

Elv::Io::Mode ReadOnlyBufferWrapper::getMode() const
{
	return Elv::Io::Mode::READ;
}

bool ReadOnlyBufferWrapper::flush()
{
	return true;
}

bool ReadOnlyBufferWrapper::isValid() const
{
	return true;
}

StaticBufferWrapper::StaticBufferWrapper(void* ptr, size_t buffSize)
	: ptr(static_cast<std::byte*>(ptr)), cursor(0), buffSize(buffSize)
{

}

size_t StaticBufferWrapper::read(void* buffer, size_t size, size_t count)
{
	const size_t bytesToRead = std::min(size * count, buffSize - cursor);
	std::memcpy(buffer, &ptr[cursor], bytesToRead);
	cursor += bytesToRead;
	return bytesToRead / size;
}

size_t StaticBufferWrapper::write(const void* buffer, size_t size, size_t count)
{
	const size_t bytesToWrite = size * count;
	std::memcpy(&ptr[cursor], buffer, bytesToWrite);
	cursor += bytesToWrite;
	return count;
}

int StaticBufferWrapper::seek(long offset, Elv::Io::SeekOrigin whence)
{
	switch (whence) {
		case Elv::Io::SeekOrigin::SET:
			cursor = offset;
			break;
		case Elv::Io::SeekOrigin::CUR:
			cursor += offset;
			break;
		case Elv::Io::SeekOrigin::END:
			cursor = static_cast<intptr_t>(buffSize) + offset;
			break;
	}
	return 0;
}

long StaticBufferWrapper::tell()
{
	return static_cast<long>(cursor);
}

size_t StaticBufferWrapper::size()
{
	return buffSize;
}

bool StaticBufferWrapper::eof()
{
	return cursor == static_cast<intptr_t>(buffSize);
}

Elv::Io::Mode StaticBufferWrapper::getMode() const
{
	return Elv::Io::Mode::READ_WRITE;
}

bool StaticBufferWrapper::flush()
{
	return true;
}

bool StaticBufferWrapper::isValid() const
{
	return true;
}

}
}
