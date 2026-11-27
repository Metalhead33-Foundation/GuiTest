#include "sandbox.hpp"
#include "GameSystem.hpp"
#include "ThemeDemo.hpp"
#include <iostream>
#include <Euphemy/Io/PhysFS.hpp>
#include <sstream>
#include "JS/JsCore.hpp"

class ElvStreambuff : public std::streambuf {
private:
	Elv::Io::Device& iodev;
	std::vector<char_type> readBuf;
	std::vector<char_type> writeBuf;
	static constexpr std::size_t PUTBACK_SIZE = 8;

	bool isReadable() const {
		auto m = static_cast<uint8_t>(iodev.getMode());
		return (m & static_cast<uint8_t>(Elv::Io::Mode::READ)) != 0;
	}

	bool isWritable() const {
		auto m = static_cast<uint8_t>(iodev.getMode());
		return (m & (static_cast<uint8_t>(Elv::Io::Mode::WRITE) |
					 static_cast<uint8_t>(Elv::Io::Mode::APPEND))) != 0;
	}

	// Flushes the internal write buffer to the underlying device
	bool flushWriteBuffer() {
		std::ptrdiff_t n = pptr() - pbase();
		if (n <= 0) return true;

		std::size_t written = iodev.write(pbase(), sizeof(char_type), static_cast<std::size_t>(n));
		if (written != static_cast<std::size_t>(n)) {
			return false;
		}
		setp(writeBuf.data(), writeBuf.data() + writeBuf.size());
		return true;
	}

	// Discards buffered unread characters and rewinds the device cursor to match gptr()
	void syncReadCursor() {
		std::ptrdiff_t unread = egptr() - gptr();
		if (unread > 0) {
			iodev.seek(-static_cast<long>(unread), Elv::Io::SeekOrigin::CUR);
		}
		char_type* end = readBuf.data() + readBuf.size();
		setg(end, end, end);
	}

public:
	explicit ElvStreambuff(Elv::Io::Device& dev, std::size_t bufferSize = 4096)
		: iodev(dev)
	{
		if (bufferSize <= PUTBACK_SIZE) {
			bufferSize = PUTBACK_SIZE + 64;
		}

		if (isReadable()) {
			readBuf.resize(bufferSize);
			char_type* end = readBuf.data() + readBuf.size();
			setg(end, end, end);
		}

		if (isWritable()) {
			writeBuf.resize(bufferSize);
			setp(writeBuf.data(), writeBuf.data() + writeBuf.size());
		}
	}

	~ElvStreambuff() override {
		sync();
	}

	// Non-copyable because of reference member and internal buffer pointers
	ElvStreambuff(const ElvStreambuff&) = delete;
	ElvStreambuff& operator=(const ElvStreambuff&) = delete;

protected:
	int sync() override {
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

	pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override {
		if (!iodev.isValid()) return pos_type(off_type(-1));

		// Ensure pending writes are committed before changing device position
		if (isWritable() && !flushWriteBuffer()) {
			return pos_type(off_type(-1));
		}

		// Account for buffered read data when seeking relative to current position
		if (way == std::ios_base::cur && (which & std::ios_base::in) && isReadable()) {
			off -= static_cast<off_type>(egptr() - gptr());
		}

		if (isReadable()) {
			char_type* end = readBuf.data() + readBuf.size();
			setg(end, end, end);
		}

		// Fast-path tellg() / tellp() calls (seekoff(0, cur))
		if (off == 0 && way == std::ios_base::cur) {
			long pos = iodev.tell();
			return (pos < 0) ? pos_type(off_type(-1)) : pos_type(pos);
		}

		Elv::Io::SeekOrigin origin;
		switch (way) {
			case std::ios_base::beg: origin = Elv::Io::SeekOrigin::SET; break;
			case std::ios_base::cur: origin = Elv::Io::SeekOrigin::CUR; break;
			case std::ios_base::end: origin = Elv::Io::SeekOrigin::END; break;
			default: return pos_type(off_type(-1));
		}

		if (iodev.seek(static_cast<long>(off), origin) != 0) {
			return pos_type(off_type(-1));
		}

		long newPos = iodev.tell();
		return (newPos < 0) ? pos_type(off_type(-1)) : pos_type(newPos);
	}

	pos_type seekpos(pos_type sp, std::ios_base::openmode which) override {
		return seekoff(off_type(sp), std::ios_base::beg, which);
	}

	std::streamsize showmanyc() override {
		if (!iodev.isValid() || !isReadable()) return -1;
		long pos = iodev.tell();
		std::size_t total = iodev.size();
		if (pos < 0 || static_cast<std::size_t>(pos) >= total) return 0;
		return static_cast<std::streamsize>(total - static_cast<std::size_t>(pos));
	}

	int_type underflow() override {
		if (!iodev.isValid() || !isReadable()) return traits_type::eof();

		// If switching from writing to reading, flush pending writes first
		if (isWritable() && pptr() > pbase()) {
			if (!flushWriteBuffer()) return traits_type::eof();
		}

		if (gptr() < egptr()) {
			return traits_type::to_int_type(*gptr());
		}

		// Preserve up to PUTBACK_SIZE characters at the start of readBuf for unget()/putback()
		std::size_t putbackCount = std::min<std::size_t>(gptr() - eback(), PUTBACK_SIZE);
		if (putbackCount > 0) {
			std::memmove(
				readBuf.data() + (PUTBACK_SIZE - putbackCount),
				gptr() - putbackCount,
				putbackCount * sizeof(char_type)
				);
		}

		char_type* readStart = readBuf.data() + PUTBACK_SIZE;
		std::size_t maxRead = readBuf.size() - PUTBACK_SIZE;
		std::size_t bytesRead = iodev.read(readStart, sizeof(char_type), maxRead);

		if (bytesRead == 0) {
			return traits_type::eof();
		}

		setg(
			readBuf.data() + (PUTBACK_SIZE - putbackCount),
			readStart,
			readStart + bytesRead
			);

		return traits_type::to_int_type(*gptr());
	}

	std::streamsize xsgetn(char_type* s, std::streamsize n) override {
		if (!iodev.isValid() || !isReadable() || n <= 0) return 0;

		if (isWritable() && pptr() > pbase()) {
			if (!flushWriteBuffer()) return 0;
		}

		std::streamsize totalRead = 0;

		// 1. Drain whatever is already in the get area
		std::streamsize avail = egptr() - gptr();
		if (avail > 0) {
			std::streamsize toCopy = std::min(n, avail);
			std::memcpy(s, gptr(), static_cast<std::size_t>(toCopy) * sizeof(char_type));
			gbump(static_cast<int>(toCopy));
			s += toCopy;
			n -= toCopy;
			totalRead += toCopy;
		}

		// 2. For large remaining reads, bypass the internal buffer directly into user memory
		if (n >= static_cast<std::streamsize>(readBuf.size() - PUTBACK_SIZE)) {
			std::size_t directRead = iodev.read(s, sizeof(char_type), static_cast<std::size_t>(n));
			totalRead += static_cast<std::streamsize>(directRead);
			return totalRead;
		}

		// 3. Otherwise fall back to standard buffered reads
		if (n > 0) {
			totalRead += std::streambuf::xsgetn(s, n);
		}

		return totalRead;
	}

	int_type pbackfail(int_type c) override {
		if (!isReadable()) return traits_type::eof();

		if (gptr() > eback()) {
			gbump(-1);
			if (!traits_type::eq_int_type(c, traits_type::eof())) {
				*gptr() = traits_type::to_char_type(c);
			}
			return traits_type::not_eof(c);
		}

		// If the putback buffer is exhausted, step back 1 byte on the device
		if (iodev.seek(-1, Elv::Io::SeekOrigin::CUR) == 0) {
			char_type ch = traits_type::eq_int_type(c, traits_type::eof())
			? char_type{}
			: traits_type::to_char_type(c);

			if (traits_type::eq_int_type(c, traits_type::eof())) {
				if (iodev.read(&ch, sizeof(char_type), 1) != 1) return traits_type::eof();
				iodev.seek(-1, Elv::Io::SeekOrigin::CUR);
			}

			readBuf[PUTBACK_SIZE - 1] = ch;
			setg(&readBuf[PUTBACK_SIZE - 1], &readBuf[PUTBACK_SIZE - 1], &readBuf[PUTBACK_SIZE]);
			return traits_type::not_eof(c);
		}

		return traits_type::eof();
	}

	int_type overflow(int_type c) override {
		if (!iodev.isValid() || !isWritable()) return traits_type::eof();

		// If switching from reading to writing, align device cursor first
		if (isReadable() && egptr() > gptr()) {
			syncReadCursor();
		}

		if (!flushWriteBuffer()) {
			return traits_type::eof();
		}

		if (!traits_type::eq_int_type(c, traits_type::eof())) {
			*pptr() = traits_type::to_char_type(c);
			pbump(1);
		}

		return traits_type::not_eof(c);
	}

	std::streamsize xsputn(const char_type* s, std::streamsize n) override {
		if (!iodev.isValid() || !isWritable() || n <= 0) return 0;

		if (isReadable() && egptr() > gptr()) {
			syncReadCursor();
		}

		// For large writes, flush pending buffer and write directly from user memory
		if (n >= static_cast<std::streamsize>(writeBuf.size())) {
			if (!flushWriteBuffer()) return 0;
			std::size_t written = iodev.write(s, sizeof(char_type), static_cast<std::size_t>(n));
			return static_cast<std::streamsize>(written);
		}

		return std::streambuf::xsputn(s, n);
	}
};

int main(int argc, char *argv[])
{
	PhysFS::System::init(argv[0]);
	JS::Core::inintializeJs();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	PhysFS::System physfsSystem;
	std::string writepath = std::string(physfsSystem.getBaseDir()) + physfsSystem.separator() + "overwrite";
	physfsSystem.setWriteDir(writepath.c_str());
	physfsSystem.mount(physfsSystem.getBaseDir(),"/");
	physfsSystem.mount(writepath.c_str(),"/");
	Euph::Conf::Configuration config;
	{
		auto inifile = physfsSystem.open("config.ini", Elv::Io::Mode::READ);

		if(inifile->isValid()) {
			ElvStreambuff buf(*inifile);
			std::istream stream(&buf);
			stream.seekg(0);
			config.fromString(stream);
		}
	}
	{
		std::string paths = std::string(config.getString("Game","ModPaths"));
		std::string token;
		std::stringstream ss(paths);
		// Extract tokens separated by semicolons
		while (getline(ss, token, ';')) {
			physfsSystem.mount(token.c_str(),"");
		}

	}
	physfsSystem.mount("/home/legacy/blenda","/");
	physfsSystem.mount("/home/legacy/zene/GameMusic/Unreal Tournament/mod","/music");
	// A terminal-only path: parse the theme document, generate its procedural atlas,
	// dump the PNG next to the working directory and exit. It needs no window, no GPU
	// and no audio device, which is what makes it usable from CI.
	if (argc > 1 && std::string(argv[1]) == "--theme") {
		const int exitCode = Sandbox::runThemeDemo(physfsSystem, "/gfx/theme_000.json", "theme_out");
		JS::Core::shutdownJs();
		PhysFS::System::deinit();
		return exitCode;
	}
	//testConfiguration();
	//testCustomAllocator();
	/*testAssetRuntime();
	testGpuAssetStreamer();
	testAsyncIo();
	testTGA();
	testPNG();
	testMemoryArenas();
	testFontRoundTrip();
	testAudioEffects();
	testTextPipeline();
	testSignals();
	testGeometryBatches();
	testNetLoopback();
	testFontParser();*/
	{
		GameSystem gamesys(config, physfsSystem);
		gamesys.run();
	}
	// The game's module streamer owns a PhysFS device. It must be destroyed while
	// PhysicsFS is still initialized, otherwise PHYSFS_close() runs on freed state.
	{
		auto inifile = physfsSystem.open("config.ini", Elv::Io::Mode::WRITE);

		if(inifile->isValid()) {
			ElvStreambuff buf(*inifile);
			std::ostream stream(&buf);
			stream << "; header test\n";
			//stream.seekg(0);
			stream << config << std::endl;
		}
		else {
			std::cerr << "Failed to open INI config for writing!" << std::endl;
		}
	}
	JS::Core::shutdownJs();
	PhysFS::System::deinit();
	std::cout << "Goodbye!" << std::endl;
	return 0;
}
