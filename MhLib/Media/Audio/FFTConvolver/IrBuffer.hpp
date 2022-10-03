#ifndef IRBUFFER_HPP
#define IRBUFFER_HPP
#include <MhLib/Media/Audio/MhSoundFile.hpp>
#include <vector>
namespace fftconvolver
{
class IrBuffer;
typedef std::shared_ptr<IrBuffer> sIrBuffer;

class IrBuffer
{
private:
	size_t blockSize;
	size_t irLen;
	size_t segSize;
	size_t segCount;
	MH33::Audio::FrameRate framerate;
	std::vector<float> buffer;
	void init(const float *ir, size_t irLen);
	void init(MH33::Audio::SoundFile& sfile);
public:
	IrBuffer(const float *ir, size_t irLen, MH33::Audio::FrameRate framerate);
	IrBuffer(const MH33::Io::sDevice& iodev);
	IrBuffer(MH33::Io::sDevice&& iodev);
	IrBuffer(MH33::Io::DeviceCreator iodev_creator, MH33::Io::Mode mode);
	IrBuffer( MH33::Io::System& iosys, const char* path, MH33::Io::Mode mode);
	IrBuffer(MH33::Io::System& iosys, const std::string& path, MH33::Io::Mode mode);
	size_t getBlockSize() const;
	size_t getIrLen() const;
	size_t getSegSize() const;
	size_t getSegCount() const;
	const float& operator[](size_t index) const;
	float& operator[](size_t index);
	const float& at(size_t index) const;
	float& at(size_t index);
	const float* data() const;
	float* data();
	MH33::Audio::FrameRate getFramerate() const;
	const std::vector<float>& getBuffer() const;
};
}
#endif // IRBUFFER_HPP
