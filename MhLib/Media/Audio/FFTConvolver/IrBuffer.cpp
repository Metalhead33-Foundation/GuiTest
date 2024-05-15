#include <MhLib/Media/Audio/FFTConvolver/IrBuffer.hpp>
#include <MhLib/Media/Audio/FFTConvolver/Utilities.h>
#include <cmath>
namespace fftconvolver
{
size_t IrBuffer::getBlockSize() const
{
	return blockSize;
}

size_t IrBuffer::getIrLen() const
{
	return irLen;
}

size_t IrBuffer::getSegSize() const
{
	return segSize;
}

size_t IrBuffer::getSegCount() const
{
	return segCount;
}

const float &IrBuffer::operator[](size_t index) const
{
	return buffer[index];
}

float &IrBuffer::operator[](size_t index)
{
	return buffer[index];
}

const float &IrBuffer::at(size_t index) const
{
	return buffer.at(index);
}

float &IrBuffer::at(size_t index)
{
	return buffer.at(index);
}

const float *IrBuffer::data() const
{
	return buffer.data();
}

float *IrBuffer::data()
{
	return buffer.data();
}

MH33::Audio::FrameRate IrBuffer::getFramerate() const
{
	return framerate;
}

const std::vector<float> &IrBuffer::getBuffer() const
{
	return buffer;
}

void IrBuffer::init(const float *ir, size_t irLen)
{
	// Zero out everything
	this->irLen = 0;
	this->blockSize = 0;
	this->segSize = 0;
	this->segCount = 0;
	buffer.clear();
	while (irLen > 0 && std::fabs(ir[irLen-1]) < 0.000001f)
	{
		--irLen;
	}
	if(!irLen) return;
	this->irLen = irLen;
	this->blockSize = NextPowerOf2(std::min(irLen,size_t(20000)));
	this->segSize = 2 * this->blockSize;
	this->segCount = static_cast<size_t>(std::ceil(static_cast<float>(irLen) / static_cast<float>(this->blockSize)));
	buffer.resize(this->segSize);
	for (size_t i=0; i< this->segCount; ++i)
	{
		const size_t remaining = irLen - (i * this->blockSize);
		const size_t sizeCopy = (remaining >= this->blockSize) ? this->blockSize : remaining;
		CopyAndPad(buffer.data(), buffer.size(), &ir[i*this->blockSize], sizeCopy);
	}
}

void IrBuffer::init(MH33::Audio::SoundFile &sfile)
{
	this->framerate = sfile.getFrameRate();
	std::vector<float> tmpBuff(MH33::Audio::framesToSamples(sfile.getFrameNum(),sfile.getChannels()).var);
	sfile.readf(tmpBuff.data(),sfile.getFrameNum());
	init(tmpBuff.data(),tmpBuff.size());
}

IrBuffer::IrBuffer(const float* ir, size_t irLen, MH33::Audio::FrameRate framerate) : framerate(framerate)
{
	init(ir,irLen);
}

IrBuffer::IrBuffer(const MH33::Io::sDevice &iodev)
{
	MH33::Audio::SoundFile sfile(iodev);
	this->framerate = sfile.getFrameRate();
	init(sfile);
}

IrBuffer::IrBuffer(MH33::Io::sDevice &&iodev)
{
	MH33::Audio::SoundFile sfile(iodev);
	this->framerate = sfile.getFrameRate();
	init(sfile);
}

IrBuffer::IrBuffer(MH33::Io::DeviceCreator iodev_creator, MH33::Io::Mode mode)
{
	MH33::Audio::SoundFile sfile(iodev_creator,mode);
	this->framerate = sfile.getFrameRate();
	init(sfile);
}

IrBuffer::IrBuffer(MH33::Io::System &iosys, const char *path, MH33::Io::Mode mode)
{
	MH33::Audio::SoundFile sfile(iosys,path,mode);
	this->framerate = sfile.getFrameRate();
	init(sfile);
}

IrBuffer::IrBuffer(MH33::Io::System &iosys, const std::string &path, MH33::Io::Mode mode)
{
	MH33::Audio::SoundFile sfile(iosys,path,mode);
	this->framerate = sfile.getFrameRate();
	init(sfile);
}
}
