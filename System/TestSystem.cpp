#include "TestSystem.hpp"

struct PrimitiveColoredGayTriangle {
	glm::vec3 POS;
	glm::vec4 CLR;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};

const MH33::GFX::AttributeDescriptor PrimitiveColoredGayTriangle::attributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x3, .offset = offsetof(PrimitiveColoredGayTriangle,POS) },
	{ .SemanticName = "CLR", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x4, .offset = offsetof(PrimitiveColoredGayTriangle,CLR) }
};

const MH33::GFX::VertexDescriptor PrimitiveColoredGayTriangle::vertexDescriptor = {
	.stride = sizeof(PrimitiveColoredGayTriangle),
	.descriptors = attributes
};

static const PrimitiveColoredGayTriangle MyTriangleUwu[] = {
	{ .POS = { -0.75f, -0.75f, 0.0f }, .CLR = { 1.0f, 0.0f, 0.0f, 1.0f } },
	{ .POS = { 0.0f, 0.75f, 0.0f }, .CLR = { 0.0f, 1.0f, 0.0f, 1.0f } },
	{ .POS = { 0.75f, -0.75f, 0.0f }, .CLR = { 0.0f, 0.0f, 1.0f, 0.5f } }
};

TestSystem::TestSystem(const MH33::Io::sSystem& iosys, const ResourceFactoryCreator& gfxCreator, IniConfiguration &conf)
	: AppSystem(
		conf["Main"].getValueOrDefault("sTitle","Default Window")->second.toString(),
		0, 0,
		conf["Video"].getValueOrDefault("iWidth","640")->second.value.i_int,
		conf["Video"].getValueOrDefault("iHeight","640")->second.value.i_int,
		(conf["Video"].getValueOrDefault("bFullscreen","0")->second.value.i_bool) ? SDL_WINDOW_FULLSCREEN : 0
	), iosys(iosys), triangleVbo(nullptr), trianglePipeline(nullptr), gfx(gfxCreator(syswmi))
{
	triangleVbo = MH33::GFX::uUnindexedVertexBuffer(gfx->createUnindexedVertexBuffer(MH33::GFX::VertexBufferUsageClass::Static,&PrimitiveColoredGayTriangle::vertexDescriptor, 0));
	triangleVbo->bind();
	triangleVbo->initializeData(std::span<const std::byte>( reinterpret_cast<const std::byte*>(MyTriangleUwu), sizeof (PrimitiveColoredGayTriangle) * 3 ) );
	MH33::GFX::ShaderModuleCreateInfo moduleCreateInfos[2];
	moduleCreateInfos[0].shaderType = MH33::GFX::ShaderModuleType::VERTEX_SHADER;
	moduleCreateInfos[1].shaderType = MH33::GFX::ShaderModuleType::PIXEL_SHADER;
	MH33::Io::uDevice f1(iosys->open("triang.vert",MH33::Io::Mode::READ));
	MH33::Io::uDevice f2(iosys->open("triang.frag",MH33::Io::Mode::READ));
	moduleCreateInfos[0].source = f1->readAll();
	moduleCreateInfos[1].source = f2->readAll();
	trianglePipeline = MH33::GFX::uPipeline(gfx->createPipeline(moduleCreateInfos,&PrimitiveColoredGayTriangle::vertexDescriptor));
}

void TestSystem::render(float deltaTime)
{
	gfx->beginFrame();
	/*
	//Get window surface
	auto screenSurface = SDL_GetWindowSurface( window.get() );

	//Fill the surface white
	SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) );

	//Update the surface
	SDL_UpdateWindowSurface( window.get() );
	*/
	trianglePipeline->bind();
	trianglePipeline->draw(*triangleVbo, MH33::GFX::RenderType::TRIANGLES, 0, 3);
	gfx->endFrame();
}

void TestSystem::update(float deltaTime)
{
}

void TestSystem::handleDisplayEvent(const SDL_DisplayEvent &ev)
{
}

void TestSystem::handleWindowEvent(const SDL_WindowEvent &ev)
{
	switch (ev.type) {
		case SDL_WINDOWEVENT_CLOSE:
		this->quit = true; break;
	default: break;
	}
}

void TestSystem::handleKeyboardEvent(const SDL_KeyboardEvent &ev)
{
}

void TestSystem::handleTextEditingEvent(const SDL_TextEditingEvent &ev)
{
}

void TestSystem::handleTextEditingExtEvent(const SDL_TextEditingExtEvent &ev)
{
}

void TestSystem::handleTextInputEvent(const SDL_TextInputEvent &ev)
{
}

void TestSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent &ev)
{
}

void TestSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent &ev)
{
}

void TestSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent &ev)
{
}

void TestSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent &ev)
{
}

void TestSystem::handleJoyBallEvent(const SDL_JoyBallEvent &ev)
{
}

void TestSystem::handleJoyHatEvent(const SDL_JoyHatEvent &ev)
{
}

void TestSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent &ev)
{
}

void TestSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent &ev)
{
}

void TestSystem::handleJoyBatteryEvent(const SDL_JoyBatteryEvent &ev)
{
}

void TestSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent &ev)
{
}

void TestSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent &ev)
{
}

void TestSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent &ev)
{
}

void TestSystem::handleControllerTouchpadEvent(const SDL_ControllerTouchpadEvent &ev)
{
}

void TestSystem::handleControllerSensorEvent(const SDL_ControllerSensorEvent &ev)
{
}

void TestSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent &ev)
{
}

void TestSystem::handleSensorEvent(const SDL_SensorEvent &ev)
{
}

void TestSystem::handleQuitEvent(const SDL_QuitEvent &ev)
{
	this->quit = true;
}

void TestSystem::handleUserEvent(const SDL_UserEvent &ev)
{
}

void TestSystem::handleSysWMEvent(const SDL_SysWMEvent &ev)
{
}

void TestSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent &ev)
{
}

void TestSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent &ev)
{
}

void TestSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent &ev)
{
}

void TestSystem::handleDropEvent(const SDL_DropEvent &ev)
{
}

void TestSystem::onLowMemory()
{
	quit = true;
}

void TestSystem::onDidEnterBackground()
{
}

void TestSystem::onDidEnterForeground()
{
}

void TestSystem::onWillEnterBackground()
{
}

void TestSystem::onWillEnterForeground()
{
}

void TestSystem::onTerminate()
{
	quit = true;
}

void TestSystem::onLocaleChanged()
{
}

void TestSystem::onClipboardUpdate()
{
}

void TestSystem::onKeymapChanged()
{
}
