#ifndef PHYSFSIO_HPP
#define PHYSFSIO_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <string>
namespace PhysFS {

class MH_IO_API IO : public MH33::Io::Device
{
	void* handle;
	MH33::Io::Mode mode;
	IO(const IO& cpy) = delete; // No copy constructor
	IO& operator=(const IO& cpy) = delete; // No copy assignment operator
public:
	// Destructor
	~IO();
	// Move constructor
	IO(IO&& mov);
	// Move assignment operator
	IO& operator=(IO&& mov);
	// Default constructor
	IO();
	// Actual constructor
	IO(const char* path, MH33::Io::Mode mode);
	IO(const std::string& path, MH33::Io::Mode mode);
	// Actual imnplementation
	bool flush();
	bool seek(MH33::Io::SeekOrigin whence, intptr_t offset);
	intptr_t tell();
	intptr_t size();
	size_t write(const void* data, size_t dataSize);
	size_t read(void* destination, size_t dataSize);
	MH33::Io::Mode getMode() const;
	bool isValid() const;
	void setBuffer(size_t newBuffsize);
};

}

#endif // PHYSFSIO_HPP
