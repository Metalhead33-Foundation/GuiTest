#include "MhCBufferWrapper.hpp"

namespace MH33 {

namespace Io {

CBufferWrapper::CBufferWrapper(const Util::CByteSpan& data)
	: buffer(data)
{

}

CBufferWrapper::CBufferWrapper(const void* data, size_t size)
	: buffer(static_cast<const std::byte*>(data), size)
{

}

const MH33::Util::CByteSpan& CBufferWrapper::getBuffer() const
{
	return buffer;
}

bool CBufferWrapper::flush()
{

}

bool CBufferWrapper::seek(SeekOrigin whence, intptr_t offset)
{
	switch (whence) {
	case SeekOrigin::CUR:
		cursor += offset;
		break;
	case SeekOrigin::END:
		cursor = buffer.size() - offset;
		break;
	case SeekOrigin::SET:
		cursor = offset;
		break;
	}
	if(cursor < 0) {
		cursor = 0;
		return false;
	} else
		if(cursor > buffer.size()) {
		cursor = buffer.size();
		return false;
	} else return true;
}

intptr_t CBufferWrapper::tell()
{
	return cursor;
}

intptr_t CBufferWrapper::size()
{
	return buffer.size();
}

size_t CBufferWrapper::write(const void* data, size_t dataSize)
{
	// READ-ONLY!
	(void)data;
	(void)dataSize;
	return 0;
}

size_t CBufferWrapper::read(void* destination, size_t dataSize)
{
	size_t readData = 0;
	for(;readData < dataSize; ++readData) {
		if(cursor < buffer.size()) static_cast<std::byte*>(destination)[readData] = buffer[cursor];
		else break;
		++cursor;
	}
	return readData;
}

Mode CBufferWrapper::getMode() const
{
	return Mode::READ;
}

bool CBufferWrapper::isValid() const
{
	return true;
}

}

}
