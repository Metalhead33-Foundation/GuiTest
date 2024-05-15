#include <MhLib/Io/MhProxyWriteStream.hpp>
#include <MhLib/Io/MhDataStream.hpp>
#include <cstring>
namespace MH33 {
namespace Io {

ProxyWriteStream::ProxyWriteStream(size_t inBuffSize, size_t outBuffSize) : outFront(nullptr), inBuff(inBuffSize), outBuff(outBuffSize),
	outBuffCursor(0), inBuffCursor(0)
{

}

Device *ProxyWriteStream::getOutFront() const
{
	return outFront;
}

void ProxyWriteStream::setOutFront(Device *value)
{
	outFront = value;
}

ProxyWriteStream::ProxyWriteStream(Device *inFront, size_t inBuffSize, size_t outBuffSize) : outFront(inFront), inBuff(inBuffSize), outBuff(outBuffSize),
	outBuffCursor(0), inBuffCursor(0)
{

}

bool ProxyWriteStream::flush()
{
	if(!outFront) return false;
	if(inBuffCursor) {
		fillBuffers(inBuff.data(),inBuffCursor,outBuff.data(),outBuffCursor,outBuff.size());
		DataStreamBE(*outFront) << uint32_t(inBuffCursor) << u_int32_t(outBuffCursor);
		outFront->write(outBuff.data(),outBuffCursor);
		outBuffCursor = 0;
		inBuffCursor = 0;
		return outFront->flush();
	} else return true;
	// virtual void fillBuffers(const void* input, size_t inSize, void* outBuff, size_t& outBuffCursor, size_t& outBuffEnd) = 0;
}

bool ProxyWriteStream::seek(SeekOrigin whence, intptr_t offset)
{
	(void)whence;
	(void)offset;
	return false;
}

intptr_t ProxyWriteStream::tell()
{
	return -1;
}

intptr_t ProxyWriteStream::size()
{
	return -1;
}

size_t ProxyWriteStream::write(const void *data, size_t dataSize)
{
	size_t dataWritten = 0;
	while(dataWritten < dataSize) {
		size_t writtenNow = std::min(dataSize-dataWritten,inBuff.size() - inBuffCursor);
		std::memcpy(&inBuff[inBuffCursor],&static_cast<const std::byte*>(data)[dataWritten],writtenNow);
		dataWritten += writtenNow;
		inBuffCursor += writtenNow;
		if(inBuffCursor >= inBuff.size()) {
			if (!flush()) break;
		}
	}
	return dataWritten;
}

size_t ProxyWriteStream::read(void *destination, size_t dataSize)
{
	(void)destination;
	(void)dataSize;
	return 0;
}

Mode ProxyWriteStream::getMode() const
{
	return Mode::READ;
}

bool ProxyWriteStream::isValid() const
{
	if(outFront) return outFront->isValid();
	else return false;
}

}
}
