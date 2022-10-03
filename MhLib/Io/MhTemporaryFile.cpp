#include <MhLib/Io/MhTemporaryFile.hpp>
#include <cassert>
namespace MH33 {
namespace Io {
TemporaryFile::~TemporaryFile()
{
	if(fdev) fclose(fdev);
}

TemporaryFile::TemporaryFile(TemporaryFile &&mov)
	: fdev(mov.fdev)
{
	mov.fdev = nullptr;
}

TemporaryFile &TemporaryFile::operator=(TemporaryFile &&mov)
{
	if(fdev) fclose(fdev);
	this->fdev = mov.fdev;
	mov.fdev = nullptr;
	return *this;
}

TemporaryFile::TemporaryFile()
	: fdev(tmpfile())
{

}

bool TemporaryFile::flush()
{
	assert(fdev);
	return fflush(fdev) == 0;
}

bool TemporaryFile::seek(SeekOrigin whence, intptr_t offset)
{
	assert(fdev);
	switch (whence) {
	case SeekOrigin::CUR:
		return fseek(fdev,offset,SEEK_CUR) == 0;
	case SeekOrigin::SET:
		return fseek(fdev,offset,SEEK_SET) == 0;
	case SeekOrigin::END:
		return fseek(fdev,offset,SEEK_END) == 0;
	}
	return false;
}

intptr_t TemporaryFile::tell()
{
	assert(fdev);
	return ftell(fdev);
}

intptr_t TemporaryFile::size()
{
	assert(fdev);
	auto originalPos = ftell(fdev);
	fseek(fdev,0,SEEK_END);
	auto theEnd = ftell(fdev);
	fseek(fdev,originalPos,SEEK_SET);
	return theEnd;
}

size_t TemporaryFile::write(const void *data, size_t dataSize)
{
	assert(fdev);
	return fwrite(data,1,dataSize,fdev);
}

size_t TemporaryFile::read(void *destination, size_t dataSize)
{
	assert(fdev);
	return fread(destination,1,dataSize,fdev);
}

Mode TemporaryFile::getMode() const
{
	return Mode::READ_WRITE;
}

bool TemporaryFile::isValid() const
{
	return fdev != nullptr;
}

}
}
