#include "GlslangIncluder.hpp"

GlslangIncluder::GlslangIncluder(MH33::Io::pSystem iosys)
	: iosys(iosys)
{

}

glslang::TShader::Includer::IncludeResult* GlslangIncluder::includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth)
{
	std::string path = "/shaders/" + std::string(headerName);
	{
		auto it = mIncludes.find(path);
		if(it != std::end(mIncludes)) {
			return it->second.get();
		}
	}
	if(!iosys->exists(path)) {
		return nullptr;
	}
	MH33::Io::uDevice iodev(iosys->open(path,MH33::Io::Mode::READ));
	mSources[path] = iodev->readAll();
	auto result = std::make_unique<IncludeResult>(
		   path, reinterpret_cast<const char*>(mSources[path].data()),
		   mSources[path].size(), nullptr);
	auto [it, b] = mIncludes.emplace(std::make_pair(path, std::move(result)));
	return it->second.get();
}

glslang::TShader::Includer::IncludeResult* GlslangIncluder::includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth)
{
	return nullptr;
}

void GlslangIncluder::releaseInclude(IncludeResult*res)
{
		{
			auto it = mSources.find(res->headerName);
			if (it != mSources.end())
			{
				mSources.erase(it);
			}
		}
		{
			auto it = mIncludes.find(res->headerName);
			if (it != mIncludes.end())
			{
				it->second = nullptr;
				mIncludes.erase(it);
			}
		}
}
