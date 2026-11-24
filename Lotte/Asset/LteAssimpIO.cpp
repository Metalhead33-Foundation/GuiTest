#include "LteAssimpIO.hpp"

namespace Lotte {
namespace Asset {
AssimpIOStream::AssimpIOStream(Elv::Io::uDevice device) noexcept
	: m_device(std::move(device)) {}

AssimpIOStream::~AssimpIOStream() = default;

size_t AssimpIOStream::Read(void* pvBuffer, size_t pSize, size_t pCount) {
	if (!m_device || !m_device->isValid()) {
		return 0;
	}
	return m_device->read(pvBuffer, pSize, pCount);
}

size_t AssimpIOStream::Write(const void* pvBuffer, size_t pSize, size_t pCount) {
	if (!m_device || !m_device->isValid()) {
		return 0;
	}
	return m_device->write(pvBuffer, pSize, pCount);
}

aiReturn AssimpIOStream::Seek(size_t pOffset, aiOrigin pOrigin) {
	if (!m_device || !m_device->isValid()) {
		return aiReturn_FAILURE;
	}

	Elv::Io::SeekOrigin origin;
	switch (pOrigin) {
		case aiOrigin_SET: origin = Elv::Io::SeekOrigin::SET; break;
		case aiOrigin_CUR: origin = Elv::Io::SeekOrigin::CUR; break;
		case aiOrigin_END: origin = Elv::Io::SeekOrigin::END; break;
		default:           return aiReturn_FAILURE;
	}

	// Assimp passes size_t for offset, whereas Elv::Io::Device expects long
	return (m_device->seek(static_cast<long>(pOffset), origin) == 0)
			   ? aiReturn_SUCCESS
			   : aiReturn_FAILURE;
}

size_t AssimpIOStream::Tell() const {
	if (!m_device || !m_device->isValid()) {
		return 0;
	}
	const long pos = m_device->tell();
	return pos >= 0 ? static_cast<size_t>(pos) : 0;
}

size_t AssimpIOStream::FileSize() const {
	if (!m_device || !m_device->isValid()) {
		return 0;
	}
	return m_device->size();
}

void AssimpIOStream::Flush() {
	if (m_device && m_device->isValid()) {
		m_device->flush();
	}
}

AssimpIOSystem::AssimpIOSystem(Elv::Io::System& system, std::pmr::memory_resource* memRes) noexcept
	: m_system(system), m_memRes(memRes) {}

AssimpIOSystem::~AssimpIOSystem() = default;

bool AssimpIOSystem::Exists(const char* pFile) const {
	return m_system.exists(pFile);
}

char AssimpIOSystem::getOsSeparator() const {
	return m_system.separator();
}

Assimp::IOStream* AssimpIOSystem::Open(const char* pFile, const char* pMode) {
	const Elv::Io::Mode mode = parseMode(pMode);
	Elv::Io::uDevice device = m_system.openUnique(pFile, mode, m_memRes);

	if (!device || !device->isValid()) {
		return nullptr;
	}

	// Assimp takes ownership of the returned IOStream* and frees it via Close()
	return new AssimpIOStream(std::move(device));
}

void AssimpIOSystem::Close(Assimp::IOStream* pFile) {
	delete pFile;
}

bool AssimpIOSystem::ComparePaths(const char* one, const char* second) const {
	// Fallback to Assimp's default path comparison, or customize if your
	// Elv::Io::System backend has specific case-sensitivity rules.
	return Assimp::IOSystem::ComparePaths(one, second);
}

bool AssimpIOSystem::CreateDirectory(const std::string& path) {
	return m_system.mkdir(path);
}

bool AssimpIOSystem::ChangeDirectory(const std::string& path) {
	// Elv::Io::System does not expose a stateful working directory
	return false;
}

bool AssimpIOSystem::DeleteFile(const std::string& file) {
	return m_system.remove(file);
}

Elv::Io::Mode AssimpIOSystem::parseMode(std::string_view modeStr) noexcept {
	bool hasRead   = modeStr.find('r') != std::string_view::npos;
	bool hasWrite  = modeStr.find('w') != std::string_view::npos;
	bool hasAppend = modeStr.find('a') != std::string_view::npos;
	bool hasPlus   = modeStr.find('+') != std::string_view::npos;

	if (hasPlus) {
		if (hasAppend) {
			return Elv::Io::Mode::READ_APPEND;
		}
		return Elv::Io::Mode::READ_WRITE;
	}

	if (hasAppend) return Elv::Io::Mode::APPEND;
	if (hasWrite)  return Elv::Io::Mode::WRITE;

	// Default to READ (covers "r", "rb", "rt", or empty/unrecognized)
	return Elv::Io::Mode::READ;
}

}
}
