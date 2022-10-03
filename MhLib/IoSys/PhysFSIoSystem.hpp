#ifndef PHYSFSIOSYSTEM_HPP
#define PHYSFSIOSYSTEM_HPP
#include <MhLib/IoSys/MhIoSystem.hpp>
namespace PhysFS {

class MH_IO_API IoSystem : public MH33::Io::System
{
public:
	IoSystem();

	MH33::Io::Device* open(const char* path, MH33::Io::Mode mode);
	bool exists(const char* path);
	char separator() const;
	void enumerate(const char* path, bool withPath, std::vector<std::string>& output);
	void enumerate(const char* path, bool withPath, FilenameCallback functor);
	void enumerate(const char* path, bool withPath, FilenameCallbackPP functor);
	void enumerate(const char* path, FilesystemCallback functor);
	void enumerate(const char* path, FilesystemCallbackPP functor);
	bool isDirectory(const char* path);
	bool isSymlink(const char* path);
	bool isFile(const char* path);
	bool mkdir(const char* dir);
	bool remove(const char* path);

	static void initialize(const char* argv0);
	static void deinitialize();
	// Unique PhysFS functionality
	void mount(const char* path, const char* mountPount, bool append = false);
	inline void mount(const std::string& path, const std::string& mountPount, bool append = false) {
		mount(path.c_str(),mountPount.c_str(),append);
	}
	void enumerateCdDrives(std::vector<std::string>& output);
	void enumerateCdDrives(FilenameCallback functor);
	inline std::vector<std::string> enumerateCdDrives() {
		std::vector<std::string> tmp;
		enumerateCdDrives(tmp);
		return tmp;
	}
	std::string getBaseDir();
	std::string getPrefDir(const char* org, const char* app);
	inline std::string getPrefDir(const std::string& org, const std::string& app) {
		return getPrefDir(org.c_str(),app.c_str());
	}
	std::string getWriteDir();
	void setWriteDir(const char* dir);
	inline void setWriteDir(const std::string& dir) {
		setWriteDir(dir.c_str());
	}
};

}
#endif // PHYSFSIOSYSTEM_HPP
