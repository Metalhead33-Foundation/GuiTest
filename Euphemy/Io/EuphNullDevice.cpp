#include "EuphNullDevice.hpp"
#include <cstring>
namespace Euph {
namespace Io {

NullDevice::NullDevice()
{

}

size_t NullDevice::read(void* buffer, size_t size, size_t count)
{
	memset(buffer,0,count * size);
	return count;
}

size_t NullDevice::write(const void* buffer, size_t size, size_t count)
{
	(void)buffer;
	(void)size;
	(void)count;
	return 0;
}

int NullDevice::seek(long offset, Elv::Io::SeekOrigin whence)
{
	(void)offset;
	(void)whence;
	return 0;
}

long NullDevice::tell()
{
	return 0;
}

size_t NullDevice::size()
{
	return 0;
}

bool NullDevice::eof()
{
	return false;
}

Elv::Io::Mode NullDevice::getMode() const
{
	return Elv::Io::Mode::READ;
}

bool NullDevice::flush()
{
	return true;
}

bool NullDevice::isValid() const
{
	return true;
}

}
}
