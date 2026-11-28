#ifndef ELVSTREAMBUFFER_HPP
#define ELVSTREAMBUFFER_HPP
/**
 * @file ElvStreamBuffer.hpp
 * @brief A `std::basic_streambuf` over an @ref Elv::Io::Device.
 *
 * This is the `ElvStreambuff` that used to live in `Sandbox/main.cpp`, moved into the
 * Io module because there is nothing sandbox-specific about it: anything that wants to
 * use `std::istream`/`std::ostream` over a PhysFS, memory or socket device needs the
 * same shim.
 *
 * @section elv_streambuffer_behaviour What the buffer guarantees
 *
 * - The **device is not owned** and must outlive the buffer. `sync()` on destruction
 *   flushes pending writes, so an `std::ostream` is safe to use without an explicit
 *   `flush()`.
 * - Reads and writes share one cursor through the device. Switching direction flushes
 *   the write buffer first / rewinds the read buffer first, so `seekoff()` and
 *   `seekpos()` see a device position that matches the stream position.
 * - `seekoff(0, cur)` is answered from `Device::tell()` without touching the device
 *   cursor, which is what `tellg()`/`tellp()` do on every call.
 * - Large `xsgetn`/`xsputn` transfers bypass the internal buffer and go straight
 *   between the device and the caller's memory.
 * - A `PUTBACK_SIZE`-byte putback area is preserved so `unget()`/`putback()` work
 *   across a refill.
 *
 * @section elv_streambuffer_chartypes Character types
 *
 * Only `char` is supported, and that is deliberate rather than an oversight: a
 * @ref Elv::Io::Device moves *bytes*, while `char16_t`, `char32_t` and `wchar_t`
 * streams move *code units*. Serving those from one device needs a transcoding
 * layer - an encoding policy, a partial-sequence state machine between calls, and
 * an error policy for malformed input - which is a different component. The
 * binary helpers in @ref ElvDataStream.hpp are not it either: they read integers,
 * not text. The `static_assert` below turns a silently wrong implementation into a
 * compile error that points here.
 */
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ios>
#include <streambuf>
#include <vector>

namespace Elv {
namespace Io {

/**
 * @brief `std::basic_streambuf` that reads and writes one Device.
 * @tparam CharT Character type; only `char` is supported (see the file documentation).
 * @tparam Traits Character traits of @p CharT.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class DeviceStreamBuffer : public std::basic_streambuf<CharT, Traits>
{
	static_assert(sizeof(CharT) == 1,
		"DeviceStreamBuffer moves bytes: a wider character type needs a transcoding layer, "
		"not a device wrapper. See the file documentation in ElvStreamBuffer.hpp.");

	using Base = std::basic_streambuf<CharT, Traits>;

public:
	/** @brief Character type of the stream. */
	using typename Base::char_type;
	/** @brief Traits type of the stream. */
	using typename Base::traits_type;
	/** @brief Integer type the base class exchanges. */
	using typename Base::int_type;
	/** @brief Position type of the base class. */
	using typename Base::pos_type;
	/** @brief Offset type of the base class. */
	using typename Base::off_type;

	/** @brief Bytes preserved at the front of the read buffer for `unget()`. */
	static constexpr std::size_t PutbackSize = 8;

	/**
	 * @brief Adopts a device and allocates the buffers its mode needs.
	 * @param dev Device to read from and/or write to; not owned.
	 * @param bufferSize Size of the read and write buffers; `PutbackSize` is added.
	 */
	explicit DeviceStreamBuffer(Device& dev, std::size_t bufferSize = 4096)
		: iodev(dev)
	{
		if (bufferSize <= PutbackSize) {
			bufferSize = PutbackSize + 64;
		}
		if (isReadable()) {
			readBuf.resize(bufferSize);
			char_type* end = readBuf.data() + readBuf.size();
			this->setg(end, end, end);
		}
		if (isWritable()) {
			writeBuf.resize(bufferSize);
			this->setp(writeBuf.data(), writeBuf.data() + writeBuf.size());
		}
	}

	/** @brief Flushes pending writes. */
	~DeviceStreamBuffer() override { this->sync(); }

	// Non-copyable: the device reference and the internal buffer pointers are not values.
	DeviceStreamBuffer(const DeviceStreamBuffer&) = delete;
	DeviceStreamBuffer& operator=(const DeviceStreamBuffer&) = delete;

protected:
	/** @brief Flushes buffered writes and aligns the device with the read cursor. */
	int sync() override
	{
		if (!iodev.isValid()) return -1;
		if (isWritable()) {
			if (!flushWriteBuffer()) return -1;
			if (!iodev.flush()) return -1;
		}
		if (isReadable()) {
			syncReadCursor();
		}
		return 0;
	}

	/**
	 * @brief Repositions the stream and the device together.
	 * @param off Offset from @p way, in characters.
	 * @param way Base the offset is measured from.
	 * @param which Direction bits of the caller; a relative read seek compensates for
	 *        the characters still sitting in the read buffer.
	 * @return The new position, or `-1` when the device refuses.
	 */
	pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override
	{
		if (!iodev.isValid()) return pos_type(off_type(-1));
		if (isWritable() && !flushWriteBuffer()) {
			return pos_type(off_type(-1));
		}
		// A relative seek must account for the characters already pulled into the buffer.
		if (way == std::ios_base::cur && (which & std::ios_base::in) && isReadable()) {
			off -= static_cast<off_type>(this->egptr() - this->gptr());
		}
		if (isReadable()) {
			char_type* end = readBuf.data() + readBuf.size();
			this->setg(end, end, end);
		}
		// `tellg()`/`tellp()` arrive here as a zero relative seek; answer them without
		// moving the device cursor at all.
		if (off == 0 && way == std::ios_base::cur) {
			const long pos = iodev.tell();
			return (pos < 0) ? pos_type(off_type(-1)) : pos_type(pos);
		}
		SeekOrigin origin;
		switch (way) {
			case std::ios_base::beg: origin = SeekOrigin::SET; break;
			case std::ios_base::cur: origin = SeekOrigin::CUR; break;
			case std::ios_base::end: origin = SeekOrigin::END; break;
			default: return pos_type(off_type(-1));
		}
		if (iodev.seek(static_cast<long>(off), origin) != 0) {
			return pos_type(off_type(-1));
		}
		const long newPos = iodev.tell();
		return (newPos < 0) ? pos_type(off_type(-1)) : pos_type(newPos);
	}

	/**
	 * @brief Absolute reposition, expressed as `seekoff(..., beg, ...)`.
	 * @param sp Position to seek to.
	 * @param which Direction bits of the caller.
	 * @return The new position, or `-1` when the device refuses.
	 */
	pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
	{
		return seekoff(off_type(sp), std::ios_base::beg, which);
	}

	/** @brief Number of characters that can be read without blocking; see the standard. */
	std::streamsize showmanyc() override
	{
		if (!iodev.isValid() || !isReadable()) return -1;
		const long pos = iodev.tell();
		const std::size_t total = iodev.size();
		if (pos < 0 || static_cast<std::size_t>(pos) >= total) return 0;
		return static_cast<std::streamsize>(total - static_cast<std::size_t>(pos));
	}

	/** @brief Refills the read buffer, preserving the putback area. */
	int_type underflow() override
	{
		if (!iodev.isValid() || !isReadable()) return traits_type::eof();
		// A stream that switched from writing has to commit its buffer first.
		if (isWritable() && this->pptr() > this->pbase()) {
			if (!flushWriteBuffer()) return traits_type::eof();
		}
		if (this->gptr() < this->egptr()) {
			return traits_type::to_int_type(*this->gptr());
		}
		// Keep up to `PutbackSize` consumed characters at the front so `unget()` works
		// after a refill.
		const std::size_t putbackCount =
			std::min<std::size_t>(std::size_t(this->gptr() - this->eback()), PutbackSize);
		if (putbackCount > 0) {
			std::memmove(readBuf.data() + (PutbackSize - putbackCount),
				this->gptr() - putbackCount, putbackCount * sizeof(char_type));
		}
		char_type* readStart = readBuf.data() + PutbackSize;
		const std::size_t maxRead = readBuf.size() - PutbackSize;
		const std::size_t bytesRead = iodev.read(readStart, sizeof(char_type), maxRead);
		if (bytesRead == 0) {
			return traits_type::eof();
		}
		this->setg(readBuf.data() + (PutbackSize - putbackCount), readStart,
			readStart + bytesRead);
		return traits_type::to_int_type(*this->gptr());
	}

	/** @brief Bulk read: drains the buffer, then reads straight into the caller's memory. */
	std::streamsize xsgetn(char_type* s, std::streamsize n) override
	{
		if (!iodev.isValid() || !isReadable() || n <= 0) return 0;
		if (isWritable() && this->pptr() > this->pbase()) {
			if (!flushWriteBuffer()) return 0;
		}
		std::streamsize totalRead = 0;
		const std::streamsize available = this->egptr() - this->gptr();
		if (available > 0) {
			const std::streamsize toCopy = std::min(n, available);
			std::memcpy(s, this->gptr(), static_cast<std::size_t>(toCopy) * sizeof(char_type));
			this->gbump(static_cast<int>(toCopy));
			s += toCopy;
			n -= toCopy;
			totalRead += toCopy;
		}
		// A transfer larger than the buffer is not worth staging through it.
		if (n >= static_cast<std::streamsize>(readBuf.size() - PutbackSize)) {
			const std::size_t directRead =
				iodev.read(s, sizeof(char_type), static_cast<std::size_t>(n));
			return totalRead + static_cast<std::streamsize>(directRead);
		}
		if (n > 0) {
			totalRead += Base::xsgetn(s, n);
		}
		return totalRead;
	}

	/** @brief Pushes one character back, stepping the device if the buffer is empty. */
	int_type pbackfail(int_type c) override
	{
		if (!isReadable()) return traits_type::eof();
		if (this->gptr() > this->eback()) {
			this->gbump(-1);
			if (!traits_type::eq_int_type(c, traits_type::eof())) {
				*this->gptr() = traits_type::to_char_type(c);
			}
			return traits_type::not_eof(c);
		}
		// Nothing buffered: step the device back one character and rebuffer it. Reading it
		// again leaves the device *after* the character, which is the invariant the rest of
		// the buffer relies on: `syncReadCursor()` rewinds by however many characters are
		// still unread, so the device must be ahead of the stream position by exactly that
		// count. Buffering without re-reading would leave the device aligned with the
		// character and the next direct read would hand it out twice.
		if (iodev.tell() <= 0 || iodev.seek(-1, SeekOrigin::CUR) != 0) {
			return traits_type::eof();
		}
		char_type ch = char_type{};
		if (iodev.read(&ch, sizeof(char_type), 1) != 1) {
			return traits_type::eof();
		}
		if (!traits_type::eq_int_type(c, traits_type::eof())) {
			ch = traits_type::to_char_type(c);
		}
		readBuf[PutbackSize - 1] = ch;
		this->setg(&readBuf[PutbackSize - 1], &readBuf[PutbackSize - 1], &readBuf[PutbackSize]);
		return traits_type::not_eof(c);
	}

	/** @brief Commits the write buffer when it fills, then stores one character. */
	int_type overflow(int_type c) override
	{
		if (!iodev.isValid() || !isWritable()) return traits_type::eof();
		if (isReadable() && this->egptr() > this->gptr()) {
			syncReadCursor();
		}
		if (!flushWriteBuffer()) {
			return traits_type::eof();
		}
		if (!traits_type::eq_int_type(c, traits_type::eof())) {
			*this->pptr() = traits_type::to_char_type(c);
			this->pbump(1);
		}
		return traits_type::not_eof(c);
	}

	/** @brief Bulk write: large transfers go straight from the caller's memory. */
	std::streamsize xsputn(const char_type* s, std::streamsize n) override
	{
		if (!iodev.isValid() || !isWritable() || n <= 0) return 0;
		if (isReadable() && this->egptr() > this->gptr()) {
			syncReadCursor();
		}
		if (n >= static_cast<std::streamsize>(writeBuf.size())) {
			if (!flushWriteBuffer()) return 0;
			return static_cast<std::streamsize>(
				iodev.write(s, sizeof(char_type), static_cast<std::size_t>(n)));
		}
		return Base::xsputn(s, n);
	}

private:
	/** @brief True when the device mode allows reading. */
	bool isReadable() const
	{
		const auto mode = static_cast<std::uint8_t>(iodev.getMode());
		return (mode & static_cast<std::uint8_t>(Mode::READ)) != 0;
	}

	/** @brief True when the device mode allows writing. */
	bool isWritable() const
	{
		const auto mode = static_cast<std::uint8_t>(iodev.getMode());
		return (mode & (static_cast<std::uint8_t>(Mode::WRITE) |
						static_cast<std::uint8_t>(Mode::APPEND))) != 0;
	}

	/** @brief Writes the buffered characters out and resets the put area. */
	bool flushWriteBuffer()
	{
		const std::ptrdiff_t count = this->pptr() - this->pbase();
		if (count <= 0) return true;
		const std::size_t written = iodev.write(this->pbase(), sizeof(char_type),
			static_cast<std::size_t>(count));
		if (written != static_cast<std::size_t>(count)) {
			return false;
		}
		this->setp(writeBuf.data(), writeBuf.data() + writeBuf.size());
		return true;
	}

	/** @brief Drops buffered characters and rewinds the device cursor to match. */
	void syncReadCursor()
	{
		const std::ptrdiff_t unread = this->egptr() - this->gptr();
		if (unread > 0) {
			iodev.seek(-static_cast<long>(unread), SeekOrigin::CUR);
		}
		char_type* end = readBuf.data() + readBuf.size();
		this->setg(end, end, end);
	}

	/** @brief Device the stream reads from and writes to; not owned. */
	Device& iodev;
	/** @brief Read buffer; the first `PutbackSize` characters are the putback area. */
	std::vector<char_type> readBuf;
	/** @brief Write buffer. */
	std::vector<char_type> writeBuf;
};

} // namespace Io
} // namespace Elv

#endif // ELVSTREAMBUFFER_HPP
