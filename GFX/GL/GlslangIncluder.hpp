#ifndef GLSLANGINCLUDER_HPP
#define GLSLANGINCLUDER_HPP
#include <glslang/Public/ShaderLang.h>
#include <MhLib/IoSys/MhIoSystem.hpp>
#include <map>

class GlslangIncluder : public glslang::TShader::Includer
{
private:
	MH33::Io::pSystem iosys;
	std::map<std::string, std::unique_ptr<IncludeResult>> mIncludes;
	std::map<std::string, std::vector<std::byte>> mSources;
public:
	GlslangIncluder(MH33::Io::pSystem iosys);

	// Includer interface
public:
	/*
		// For the "system" or <>-style includes; search the "system" paths.
		virtual IncludeResult* includeSystem(const char* headerName,
											 const char* includerName,
											 size_t inclusionDepth) { return nullptr; }

		// For the "local"-only aspect of a "" include. Should not search in the
		// "system" paths, because on returning a failure, the parser will
		// call includeSystem() to look in the "system" locations.
		virtual IncludeResult* includeLocal(const char* headerName,
											const char* includerName,
											size_t inclusionDepth) { return nullptr; }
*/

	IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override;
	IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override;
	void releaseInclude(IncludeResult* res) override;
};

#endif // GLSLANGINCLUDER_HPP
