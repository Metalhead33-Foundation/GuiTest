#include <MhLib/Io/MhFile.hpp>
#include <cassert>
namespace MH33 {
namespace Io {

File::~File()
{
	if(fdev) fclose(fdev);
}

File::File(File &&mov)
	: fdev(mov.fdev), path(std::move(mov.path))
{
	mov.fdev = nullptr;
}

const std::string &File::getPath() const
{
	return path;
}

File &File::operator=(File &&mov)
{
	if(fdev) fclose(fdev);
	this->fdev = mov.fdev;
	mov.fdev = nullptr;
	this->path = std::move(mov.path);
	return *this;
}

File::File()
	: fdev(nullptr)
{

}

File::File(const char *path, Mode mode)
	: fdev(nullptr), mode(mode), path(path)
{
	switch (mode) {
	case Mode::READ:
		fdev = fopen(path,"rb");
		break;
	case Mode::WRITE:
		fdev = fopen(path,"wb");
		break;
	case Mode::APPEND:
		fdev = fopen(path,"ab");
		break;
	case Mode::READ_WRITE:
		if(!(fdev = fopen(path,"r+b"))) fdev = fopen(path,"w+b");
		break;
	case Mode::READ_APPEND:
		fdev = fopen(path,"a+b");
		break;
	}
}

File::File(const std::string &path, Mode mode)
	: fdev(nullptr), mode(mode), path(path)
{
	switch (mode) {
	case Mode::READ:
		fdev = fopen(path.c_str(),"rb");
		break;
	case Mode::WRITE:
		fdev = fopen(path.c_str(),"wb");
		break;
	case Mode::APPEND:
		fdev = fopen(path.c_str(),"ab");
		break;
	case Mode::READ_WRITE:
		if(!(fdev = fopen(path.c_str(),"r+b"))) fdev = fopen(path.c_str(),"w+b");
		break;
	case Mode::READ_APPEND:
		fdev = fopen(path.c_str(),"a+b");
		break;
	}
}

bool File::flush()
{
	assert(fdev);
	return fflush(fdev) == 0;
}

bool File::seek(SeekOrigin whence, intptr_t offset)
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

intptr_t File::tell()
{
	assert(fdev);
	return ftell(fdev);
}

intptr_t File::size()
{
	assert(fdev);
	auto originalPos = ftell(fdev);
	fseek(fdev,0,SEEK_END);
	auto theEnd = ftell(fdev);
	fseek(fdev,originalPos,SEEK_SET);
	return theEnd;
}

size_t File::write(const void *data, size_t dataSize)
{
	assert(fdev);
	return fwrite(data,1,dataSize,fdev);
}

size_t File::read(void *destination, size_t dataSize)
{
	assert(fdev);
	return fread(destination,1,dataSize,fdev);
}

Mode File::getMode() const
{
	return mode;
}

bool File::isValid() const
{
	return fdev != nullptr;
}

}
}
