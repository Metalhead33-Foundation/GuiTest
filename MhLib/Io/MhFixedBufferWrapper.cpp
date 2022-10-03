#include <MhLib/Io/MhFixedBufferWrapper.hpp>
#include <cstring>
namespace MH33 {
namespace Io {


bool FixedBufferWrapper::flush()
{
	return true;
}

bool FixedBufferWrapper::seek(SeekOrigin whence, intptr_t offset)
{
	switch (whence) {
	case SeekOrigin::CUR:
		cursor += offset;
		break;
	case SeekOrigin::END:
		cursor = _size - offset;
		break;
	case SeekOrigin::SET:
		cursor = offset;
		break;
	}
	if(cursor < 0) {
		cursor = 0;
		return false;
	} else
		if(cursor > _size) {
		cursor = _size;
		return false;
	} else return true;
}

intptr_t FixedBufferWrapper::tell()
{
	return cursor;
}

intptr_t FixedBufferWrapper::size()
{
	return _size;
}

size_t FixedBufferWrapper::write(const void *data, size_t dataSize)
{
	size_t readData = std::min(dataSize,size_t(_size-cursor));
	std::memcpy(&ptr[cursor],data,readData);
	cursor += readData;
	return readData;
}

size_t FixedBufferWrapper::read(void *destination, size_t dataSize)
{
	size_t readData = std::min(dataSize,size_t(_size-cursor));
	std::memcpy(destination,&ptr[cursor],readData);
	cursor += readData;
	return readData;
}

Mode FixedBufferWrapper::getMode() const
{
	return Mode::READ_WRITE;
}

bool FixedBufferWrapper::isValid() const
{
	return true;
}

FixedBufferWrapper::FixedBufferWrapper(void *pointer, intptr_t maxSize) : ptr(reinterpret_cast<std::byte*>(pointer)), cursor(0), _size(maxSize)
{

}

FixedBufferWrapper::FixedBufferWrapper(const FixedBufferWrapper &cpy)
	: ptr(cpy.ptr), cursor(cpy.cursor), _size(cpy._size)
{

}

FixedBufferWrapper::FixedBufferWrapper(FixedBufferWrapper &&mov)
	: ptr(mov.ptr), cursor(mov.cursor), _size(mov._size)
{

}

FixedBufferWrapper &FixedBufferWrapper::operator=(const FixedBufferWrapper &cpy)
{
	this->ptr = cpy.ptr;
	this->cursor = cpy.cursor;
	this->_size = cpy._size;
	return *this;
}

FixedBufferWrapper &FixedBufferWrapper::operator=(FixedBufferWrapper &&mov)
{
	this->ptr = mov.ptr;
	this->cursor = mov.cursor;
	this->_size = mov._size;
	return *this;
}


}
}
