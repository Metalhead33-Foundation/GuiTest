#ifndef STDFILEWRAPPER_HPP
#define STDFILEWRAPPER_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstdio>
namespace Euph {
namespace Io {
class StdfileWrapper : public Elv::Io::Device
{
private:
	FILE* cfile;
	Elv::Io::Mode mode;
public:
	StdfileWrapper(FILE* cfile, Elv::Io::Mode mode);
	StdfileWrapper(const StdfileWrapper& cpy);
	StdfileWrapper& operator=(const StdfileWrapper& cpy);
	size_t read(void* buffer, size_t size, size_t count) override;
	size_t write(const void* buffer, size_t size, size_t count) override;
	int seek(long offset, Elv::Io::SeekOrigin whence) override;
	long tell() override;
	size_t size() override;
	bool eof() override;
	Elv::Io::Mode getMode() const override;
	bool flush() override;
	bool isValid() const override;
};
#ifndef NODECLARE_STANDARD_OUTPUTS
extern StdfileWrapper STDIN, STDOUT, STDERR;
#endif
}
}
#endif // STDFILEWRAPPER_HPP
