#include <span>
#include <iostream>
#include "GFX/Abstract/VertexBufferAdapter.hpp"
#include <vector>
#include <cstring>
#include "System/IniConfiguration.hpp"
#include <fstream>
#include "System/TestSystem.hpp"
#include <GFX/GL/GlxContext.hpp>
#include <GFX/GL/EglContext.hpp>

/*class BullshitVertexBuffer : public MH33::GFX::UnindexedVertexBuffer {
private:
	const MH33::GFX::VertexDescriptor* vertexDescriptor;
	std::vector<std::byte> buff;
public:
	~BullshitVertexBuffer() {
		std::cout << "Destructed." << std::endl;
	}
	BullshitVertexBuffer(const MH33::GFX::VertexDescriptor* vertexDescriptor) : vertexDescriptor(vertexDescriptor) {
		std::cout << "Constructed without arguments." << std::endl;
	}
	BullshitVertexBuffer(const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t size) : vertexDescriptor(vertexDescriptor), buff(size) {
		std::cout << "Constructed with a size of " << size << std::endl;
	}
	const MH33::GFX::VertexDescriptor* getVertexDescriptor() const override {
		return vertexDescriptor;
	}
	void bind() const override
	{
	}
	void unbind() const override
	{
	}
	void initializeData(const std::span<const std::byte> &data) override
	{
		buff.resize(data.size());
		std::memcpy(buff.data(), data.data(), data.size() );
	}
	void getData(void *data) const override
	{
		std::memcpy(data,buff.data(),buff.size() );
	}
	size_t getDataSize() const override
	{
		return buff.size();
	}
	void ensureDataSize(size_t size) override
	{
		if(buff.size() > size) buff.resize(size);
	}
	void setData(const AccessorFunc &fun, bool needsToRead) override
	{
		(void)needsToRead;
		fun(buff);
	}
	void setData(const std::span<const std::byte> &data, size_t offset) override
	{
		std::memcpy(&buff[offset],data.data(),data.size());
	}
	void getData(const ConstAccessorFunc &fun) const override
	{
		fun(buff);
	}

	MH33::GFX::Handle getNativeHandle() override
	{
		return { .ptr = buff.data() } ;
	}
	MH33::GFX::ConstHandle getNativeHandle() const override
	{
		return { .ptr = buff.data() } ;
	}
};*/
typedef MH33::GFX::TypedVertexBuffer<int> IntVertBuff;

int main() {
	/*auto buffCreator = [](const MH33::GFX::VertexDescriptor* descriptor, size_t size) { return size ? new BullshitVertexBuffer(descriptor,size) : new BullshitVertexBuffer(descriptor); };
	auto buffAccessor = [](const std::span<const int>& data) {
		for(const auto& it : data) {
			std::cout << it << ' ';
		}
		std::cout << std::endl;
	};
	int ints[] = {0, 2, 4, 6, 8, 1, 0, 4, 7, 3, 4, 8, 9, 0};
	IntVertBuff intbuff(buffCreator,nullptr, 0);
	intbuff.initializeData( ints );
	intbuff.getData(buffAccessor);*/
	std::ifstream ifs;
	ifs.open("hello.ini");
	IniConfiguration ini(ifs);
	ifs.close();
	ini["Video"].getValueOrDefault("sRenderer","OpenGL");
	for(auto i = std::begin(ini); i != std::end(ini) ; ++i ) {
		std::cout << i->first << std::endl;
		for(auto j = std::begin(i->second) ; j != std::end( i->second ) ; ++j ) {
			std::cout << std::boolalpha << j->first << '-' << j->second << std::endl;
		}
	}
#ifdef USE_GLX
	auto initializer = [](const SDL_SysWMinfo& syswmi) {
		return new GLX::RenderingContext(syswmi);
	};
#else
	auto initializer = [](const SDL_SysWMinfo& syswmi) {
		return new EGL::RenderingContext(syswmi);
	};
#endif
	TestSystem testSys(initializer, ini);
	testSys.run();
	return 0;
}
