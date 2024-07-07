#include "TestSystem.hpp"
#include <MhLib/Media/Image/MhPNG.hpp>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <jsapi.h>
#include <MhLib/Media/Image/MhStandardImage2D.hpp>
#include <GUI/MhTexturedButton.hpp>

struct PrimitiveColoredGayTriangle {
	glm::vec3 POS;
	glm::vec4 CLR;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};
struct WidgetVertex {
	glm::vec2 aPos;
	glm::vec2 aTexCoords;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};

const MH33::GFX::AttributeDescriptor PrimitiveColoredGayTriangle::attributes[] = {
	{ .SemanticName = "POSITION", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x3, .offset = offsetof(PrimitiveColoredGayTriangle,POS) },
	{ .SemanticName = "CLR", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x4, .offset = offsetof(PrimitiveColoredGayTriangle,CLR) }
};
const MH33::GFX::AttributeDescriptor WidgetVertex::attributes[] = {
	{ .SemanticName = "POSITION", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(WidgetVertex,aPos) },
	{ .SemanticName = "TEXCOORD", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(WidgetVertex,aTexCoords) }
};

const MH33::GFX::VertexDescriptor PrimitiveColoredGayTriangle::vertexDescriptor = {
	.stride = sizeof(PrimitiveColoredGayTriangle),
	.descriptors = attributes
};

const MH33::GFX::VertexDescriptor WidgetVertex::vertexDescriptor = {
	.stride = sizeof(WidgetVertex),
	.descriptors = attributes
};

static const PrimitiveColoredGayTriangle MyTriangleUwu[] = {
	{ .POS = { -0.75f, -0.75f, 0.0f }, .CLR = { 1.0f, 0.0f, 0.0f, 1.0f } },
	{ .POS = { 0.0f, 0.75f, 0.0f }, .CLR = { 0.0f, 1.0f, 0.0f, 1.0f } },
	{ .POS = { 0.75f, -0.75f, 0.0f }, .CLR = { 0.0f, 0.0f, 1.0f, 0.1f } }
};

static const WidgetVertex ScreenQuad[] = {
	{ .aPos = { -1.0f, 1.0f }, .aTexCoords = { 0.0f, 0.0f } },
	{ .aPos = { -1.0f, -1.00f }, .aTexCoords = { 0.0f, 1.0f } },
	{ .aPos = { 1.0f, 1.0f }, .aTexCoords = { 1.0f, 0.0f } },
	{ .aPos = { 1.0f, -1.0f }, .aTexCoords = { 1.0f, 1.0f } }
};

static const uint32_t ScreenQuadIndices[] = {
	0, 1, 2, 1, 2, 3
};

struct STD140 UniformForCompute {
	int32_t width, height;
	int32_t padding[2];
};
/*struct STD140 UniformForCompute {
	int32_t width, pad0, pad2, pad3, height, pad4, pad5, pad6, pad7;
};*/

static const std::string SHADER_PATH_PREFIX = "/shaders/";

static MH33::GFX::uPipeline createPipelineFromFiles(MH33::GFX::ResourceFactory& gfx, MH33::Io::System& iosys, const std::string& shaderName, const MH33::GFX::VertexDescriptor* vertexDescriptor) {
	MH33::GFX::ShaderModuleCreateInfo moduleCreateInfos[2];
	moduleCreateInfos[0].shaderType = MH33::GFX::ShaderModuleType::VERTEX_SHADER;
	moduleCreateInfos[0].isBinary = false;
	moduleCreateInfos[1].shaderType = MH33::GFX::ShaderModuleType::PIXEL_SHADER;
	moduleCreateInfos[1].isBinary = false;
	std::string shaderSourceCodes[2];
	MH33::Io::uDevice f1(iosys.open(SHADER_PATH_PREFIX + shaderName + ".vert.hlsl",MH33::Io::Mode::READ));
	MH33::Io::uDevice f2(iosys.open(SHADER_PATH_PREFIX + shaderName + ".frag.hlsl",MH33::Io::Mode::READ));
	shaderSourceCodes[0] = f1->readAllAsString();
	shaderSourceCodes[1] = f2->readAllAsString();
	gfx.prepareShaderModuleFor(iosys,moduleCreateInfos,shaderSourceCodes);
	return MH33::GFX::uPipeline(gfx.createPipeline(moduleCreateInfos,vertexDescriptor));
}
static MH33::GFX::sPipeline createPipelineFromFilesS(MH33::GFX::ResourceFactory& gfx, MH33::Io::System& iosys, const std::string& shaderName, const MH33::GFX::VertexDescriptor* vertexDescriptor) {
	MH33::GFX::ShaderModuleCreateInfo moduleCreateInfos[2];
	moduleCreateInfos[0].shaderType = MH33::GFX::ShaderModuleType::VERTEX_SHADER;
	moduleCreateInfos[0].isBinary = false;
	moduleCreateInfos[1].shaderType = MH33::GFX::ShaderModuleType::PIXEL_SHADER;
	moduleCreateInfos[1].isBinary = false;
	std::string shaderSourceCodes[2];
	MH33::Io::uDevice f1(iosys.open(SHADER_PATH_PREFIX + shaderName + ".vert.hlsl",MH33::Io::Mode::READ));
	MH33::Io::uDevice f2(iosys.open(SHADER_PATH_PREFIX + shaderName + ".frag.hlsl",MH33::Io::Mode::READ));
	shaderSourceCodes[0] = f1->readAllAsString();
	shaderSourceCodes[1] = f2->readAllAsString();
	gfx.prepareShaderModuleFor(iosys,moduleCreateInfos,shaderSourceCodes);
	return MH33::GFX::uPipeline(gfx.createPipeline(moduleCreateInfos,vertexDescriptor));
}
static void createDuealPipeline(MH33::GFX::ResourceFactory& gfx, MH33::Io::System& iosys,
								const std::string& shaderNameA,
								const std::string& shaderNameB,
								const std::function<void(const MH33::GFX::ConstModuleCreateInfoList&, const MH33::GFX::ConstModuleCreateInfoList&)>& fun) {
	MH33::GFX::ShaderModuleCreateInfo moduleCreateInfosA[2];
	moduleCreateInfosA[0].shaderType = MH33::GFX::ShaderModuleType::VERTEX_SHADER;
	moduleCreateInfosA[0].isBinary = false;
	moduleCreateInfosA[1].shaderType = MH33::GFX::ShaderModuleType::PIXEL_SHADER;
	moduleCreateInfosA[1].isBinary = false;
	MH33::GFX::ShaderModuleCreateInfo moduleCreateInfosB[2];
	moduleCreateInfosB[0].shaderType = MH33::GFX::ShaderModuleType::VERTEX_SHADER;
	moduleCreateInfosB[0].isBinary = false;
	moduleCreateInfosB[1].shaderType = MH33::GFX::ShaderModuleType::PIXEL_SHADER;
	moduleCreateInfosB[1].isBinary = false;
	std::string sourceCodesA[2];
	std::string sourceCodesB[2];
	MH33::Io::uDevice f1(iosys.open(SHADER_PATH_PREFIX + shaderNameA + ".vert.hlsl",MH33::Io::Mode::READ));
	MH33::Io::uDevice f2(iosys.open(SHADER_PATH_PREFIX + shaderNameA + ".frag.hlsl",MH33::Io::Mode::READ));
	MH33::Io::uDevice f3(iosys.open(SHADER_PATH_PREFIX + shaderNameB + ".vert.hlsl",MH33::Io::Mode::READ));
	MH33::Io::uDevice f4(iosys.open(SHADER_PATH_PREFIX + shaderNameB + ".frag.hlsl",MH33::Io::Mode::READ));
	sourceCodesA[0] = f1->readAllAsString();
	sourceCodesA[1] = f2->readAllAsString();
	sourceCodesB[0] = f3->readAllAsString();
	sourceCodesB[1] = f4->readAllAsString();
	gfx.prepareShaderModuleFor(iosys,moduleCreateInfosA,sourceCodesA);
	gfx.prepareShaderModuleFor(iosys,moduleCreateInfosB,sourceCodesB);
	fun(moduleCreateInfosA, moduleCreateInfosB);
}
#define INSERT_HUNGARIAN
#define INSERT_RUSSIAN
#define INSERT_JAPANESE

#define GET_AUDIO_BUFFER_SIZE (conf["Audio"].getValueOrDefault("iBufferSize","2024")->second.value.i_int)
#define GET_AUDIO_SAMPLE_RATE (conf["Audio"].getValueOrDefault("iSamplerate","48000")->second.value.i_int)

const std::vector<MH33::GUI::uWidget>& TestSystem::getWidgets() const
{
	return widgets;
}

TestSystem::TestSystem(const MH33::Io::sSystem& iosys, const ResourceFactoryCreator& gfxCreator, IniConfiguration &conf)
	: AppSystem(
		conf["Main"].getValueOrDefault("sTitle","Default Window")->second.toString(),
		0, 0,
		conf["Video"].getValueOrDefault("iWidth","640")->second.value.i_int,
		conf["Video"].getValueOrDefault("iHeight","640")->second.value.i_int,
		(conf["Video"].getValueOrDefault("bFullscreen","0")->second.value.i_bool) ? SDL_WINDOW_FULLSCREEN : 0
	), audioDriver(GET_AUDIO_SAMPLE_RATE,2,GET_AUDIO_BUFFER_SIZE),
		mixer(new MH33::Audio::Mixer(MH33::Audio::FrameCount(GET_AUDIO_BUFFER_SIZE),MH33::Audio::FrameCount(GET_AUDIO_SAMPLE_RATE),MH33::Audio::ChannelCount(2))),
		iosys(iosys), triangleVbo(nullptr), trianglePipeline(nullptr), gfx(gfxCreator(syswmi)),
		jscore(iosys),
		hoveredOverWidget(nullptr)
{
	textPipeline = createPipelineFromFiles(*gfx, *iosys, "sdftext", &WidgetVertex::vertexDescriptor);
	createDuealPipeline(*gfx, *iosys,"sdftext", "textline",[this](const MH33::GFX::ConstModuleCreateInfoList& a, const MH33::GFX::ConstModuleCreateInfoList& b){
		this->fontRepo = std::make_shared<MH33::GFX::FontRepository>(this->iosys, this->gfx.get(), a, b);
	});
	fontRepo->initializeFont("noto", "/fonts/NotoTraditionalNushu-Regular.ttf");
	guiRenderer = std::make_unique<MH33::GFX::GuiRenderer>( *gfx,
														  createPipelineFromFilesS(*gfx,*iosys,"GuiColouredVertex",&MH33::GFX::ColouredGuiVertex::vertexDescriptor),
														  createPipelineFromFilesS(*gfx,*iosys,"GuiTexturedVertex",&MH33::GFX::TexturedGuiVertex::vertexDescriptor),
														  createPipelineFromFilesS(*gfx,*iosys,"GuiColouredTexturedVertex",&MH33::GFX::ColouredTexturedGuiVertex::vertexDescriptor)
														  );
	/*
	MH33::TXT::uRichTextProcessor rtp;
	MH33::TXT::uMmlParser mml;
*/
	loadLocalizations();
	// Add localizations to JS

	jscore.insertModule("CONFIG", [&conf](JSContext& ctx,JS::RootedObject& obj) {
		for( auto it = std::begin(conf) ; it != std::end(conf) ; ++it ) {
			JS::RootedObject section(&ctx, JS_NewPlainObject(&ctx));
			for( auto zt = std::begin(it->second) ; zt != std::end(it->second) ; ++zt ) {
				switch (zt->second.type) {
					case IniType::INI_BOOLEAN: {
						JS::RootedValue rootedBool(&ctx, zt->second.value.i_bool ? JS::TrueValue() : JS::FalseValue() );
						JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedBool, JSPROP_READONLY | JSPROP_ENUMERATE);
						break; }
					case IniType::INI_FLOAT: {
						JS::RootedValue rootedFloat(&ctx, JS_NumberValue(zt->second.value.i_float) );
						JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedFloat, JSPROP_READONLY | JSPROP_ENUMERATE );
						break;  }
					case IniType::INI_INTEGER: {
						JS::RootedValue rootedInt(&ctx, JS_NumberValue(zt->second.value.i_int) );
						JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedInt, JSPROP_READONLY | JSPROP_ENUMERATE );
						break; }
					case IniType::INI_UINTEGER: {
						JS::RootedValue rootedUint(&ctx, JS_NumberValue(zt->second.value.i_uint) );
						JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedUint, JSPROP_READONLY  | JSPROP_ENUMERATE);
						break; }
					case IniType::INI_STRING: {
						auto strview = zt->second.asStringView();
						JS::UTF8Chars uChars( strview.data(), strview.size() );
						JS::RootedString rootedString(&ctx, JS_NewStringCopyUTF8N(&ctx, uChars) );
						JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedString, JSPROP_READONLY | JSPROP_ENUMERATE );
						break; }
					default: break;
				}
			}
			JS_DefineProperty(&ctx,obj, it->first.c_str(), section, JSPROP_READONLY | JSPROP_ENUMERATE);
		}
	});
	jscore.insertModule("localization", [this](JSContext& ctx,JS::RootedObject& obj) {
		for(auto it = std::begin(this->localizations); it != std::end(this->localizations); ++it) {
			JS::UTF8Chars uChars(it->second.c_str(),it->second.size());
			JS::RootedString rstr(&ctx, JS_NewStringCopyUTF8N(&ctx, uChars));
			JS_DefineProperty(&ctx, obj, it->first.c_str(), rstr, JSPROP_READONLY | JSPROP_ENUMERATE);
		}
	});

	rtp = std::make_unique<MH33::TXT::RichTextProcessor>(fontRepo);
	rtp->setDefaultFontName("noto");
	rtp->setCurrentFontName("noto");
	mml = std::make_unique<MH33::TXT::MmlParser>(rtp.get());
	{
	std::stringstream strm;
	//strm << "<colour=#FF0000><b>Hello </b></colour><colour=#FFFFFF><u>World! &amp; </u></colour><colour=#00FF00><i>Hello World!</i></colour><br>" << std::endl;
	//strm << "<b><s><colour=#FFFFFF><b>Hello </b></colour><colour=#0000AA><u>World! </u></colour><colour=#AA0000><i>World!.</i></colour><br></s></b>" << std::endl;
#ifdef INSERT_HUNGARIAN
		strm << localizations["INSERT_HUNGARIAN"];
#endif
#ifdef INSERT_RUSSIAN
		strm << localizations["INSERT_RUSSIAN"];
#endif
#ifdef INSERT_JAPANESE
		strm << localizations["INSERT_JAPANESE"];
#endif
		mml->parse(strm.str());
	}
	rtp->flush();
/*	{
		MH33::GFX::ShaderModuleCreateInfo moduleCreateInfos[1];
		moduleCreateInfos[0].shaderType = MH33::GFX::ShaderModuleType::COMPUTE_SHADER;
		MH33::Image::DecodeTarget decodeTarget;
		MH33::Io::uDevice f1(iosys->open("letterA.png",MH33::Io::Mode::READ));
		MH33::Io::uDevice f2(iosys->open("sdf1.glsl",MH33::Io::Mode::READ));
		moduleCreateInfos[0].source = f2->readAll();
		moduleCreateInfos[0].isBinary = false;
		MH33::Image::PNG::decode(*f1,decodeTarget);
		tex1 = MH33::GFX::uTexture2D(gfx->createTexture2D(decodeTarget));
		tex2 = MH33::GFX::uTexture2D(gfx->createTexture2D(decodeTarget.format, tex1->getWidth(), tex1->getHeight()));
		MH33::GFX::uComputeShader compshader(gfx->createComputeShader(moduleCreateInfos));
		MH33::GFX::uStorageBuffer uniform(gfx->createStorageBuffer(MH33::GFX::StorageBufferType::UNIFORM_BUFFER,sizeof(UniformForCompute)));
		UniformForCompute uniform1 = { .width = static_cast<int32_t>(decodeTarget.frames[0].width), .height = static_cast<int32_t>(decodeTarget.frames[0].height) };
		uniform->bind();
		uniform->initializeData(std::span<std::byte>(reinterpret_cast<std::byte*>(&uniform1), sizeof(UniformForCompute)));
		compshader->bind();
		compshader->setUniform(compshader->getBindingPoint("fontTexture"), *tex1, 0, MH33::GFX::ImageBindingType::READ_ONLY);
		compshader->setUniform(compshader->getBindingPoint("sdfTexture"), *tex2, 1, MH33::GFX::ImageBindingType::WRITE_ONLY);
		compshader->setUniform(compshader->getBindingPoint("dimensions"), *uniform, 2);
		compshader->dispatchCompute(decodeTarget.frames[0].width, decodeTarget.frames[0].height, 1);
		compshader->waitForFinish();
	}
	{
		MH33::GFX::ShaderModuleCreateInfo moduleCreateInfos[1];
		moduleCreateInfos[0].shaderType = MH33::GFX::ShaderModuleType::COMPUTE_SHADER;
		moduleCreateInfos[0].isBinary = false;
		MH33::Io::uDevice f1(iosys->open("gaussian.glsl",MH33::Io::Mode::READ));
		moduleCreateInfos[0].source = f1->readAll();
		MH33::GFX::uComputeShader compshader(gfx->createComputeShader(moduleCreateInfos));
		MH33::GFX::uStorageBuffer uniform(gfx->createStorageBuffer(MH33::GFX::StorageBufferType::UNIFORM_BUFFER,sizeof(UniformForCompute)));
		UniformForCompute uniform1 = { .width = static_cast<int32_t>(tex1->getWidth()), .height = static_cast<int32_t>(tex1->getHeight()) };
		uniform->bind();
		uniform->initializeData(std::span<std::byte>(reinterpret_cast<std::byte*>(&uniform1), sizeof(UniformForCompute)));
		compshader->bind();
		compshader->setUniform(compshader->getBindingPoint("inputTexture"), *tex2, 0, MH33::GFX::ImageBindingType::READ_ONLY);
		compshader->setUniform(compshader->getBindingPoint("outputTexture"), *tex1, 1, MH33::GFX::ImageBindingType::WRITE_ONLY);
		compshader->setUniform(compshader->getBindingPoint("dimensions"), *uniform, 2);
		compshader->dispatchCompute(uniform1.width, uniform1.height, 1);
		compshader->waitForFinish();
	}*/
	{
		MH33::Image::DecodeTarget decodeTarget;
		MH33::Io::uDevice f1(iosys->open("/cursors/wodmouse.png",MH33::Io::Mode::READ));
		MH33::Image::PNG::decode(*f1,decodeTarget);
		std::unique_ptr<MH33::Image::Image2D> tmpImg(MH33::Image::createImage2D(decodeTarget.frames[0],decodeTarget.format));
		cursorTex = std::unique_ptr<MH33::GFX::Texture2D>(gfx->createTexture2D(*tmpImg));
		cursor = std::make_unique<MH33::GUI::Cursor>(*tmpImg,reinterpret_cast<uintptr_t>(cursorTex.get()), glm::fvec2(widthR,heightR));
	}
	{
		//matrixPanner = std::make_shared<MH33::Audio::MatrixPanner>(MH33::Audio::FrameCount(1024),MH33::Audio::ChannelCount(1),MH33::Audio::ChannelCount(2));
		MH33::Io::sDevice f1(iosys->open("/sounds/menu_click.ogg",MH33::Io::Mode::READ));
		audioBuffer = std::make_shared<MH33::Audio::Buffer>(f1);
		audioBuffer->resample(mixer->getFrameRate());
		soundSource = std::make_shared<MH33::Audio::SoundSource>();
		soundSource->setBuffer(audioBuffer);
		mixer->set(soundSource,1.0f);
		//mixer->set(matrixPanner,1.0f);
	}
	{
		MH33::Image::DecodeTarget decodeTarget;
		MH33::Io::uDevice f1(iosys->open("/textures/menuButton.png",MH33::Io::Mode::READ));
		MH33::Image::PNG::decode(*f1,decodeTarget);
		buttonTex = std::unique_ptr<MH33::GFX::Texture2D>(gfx->createTexture2D(decodeTarget));
		MH33::GUI::TexturedWidget::TextureCoordDuo textCoords[5];
		float texRecipH =  1.0f / static_cast<float>(buttonTex->getHeight());
		unsigned individualHeight = buttonTex->getHeight() / 5;
		for(int i = 0; i < 5; ++i) {
			textCoords[i].first = { 0.0f, texRecipH * static_cast<float>(i * individualHeight)};
			textCoords[i].second = { 1.0f, texRecipH * static_cast<float>( (i+1) * individualHeight ) };
		}
		MH33::GUI::TexturedWidget::TextureCoordDuo textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::FULL_STATE)];
		textCoords2[0] = textCoords[0];
		textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::CLICKED)] = textCoords[0];
		textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS)] = textCoords[0];
		textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::ENABLED)] = textCoords[1];
		textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::ENABLED) |
				static_cast<int>(MH33::GUI::WidgetStateFlags::CLICKED)] = textCoords[2];
		textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::ENABLED) |
				static_cast<int>(MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS)] = textCoords[3];
		textCoords2[static_cast<int>(MH33::GUI::WidgetStateFlags::ENABLED) |
				static_cast<int>(MH33::GUI::WidgetStateFlags::CLICKED) |
				static_cast<int>(MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS)] = textCoords[4];
		auto btn = std::make_unique<MH33::GUI::TexturedButton>(buttonTex.get(),textCoords2);
		btn->setTopLeft( glm::fvec2(0.0, 0.0f ));
		btn->setBottomRight( glm::fvec2(0.4f, -0.33333333333333f) );
		btn->setHidden(false);
		btn->signal_onStateChanged.connect( [this](MH33::GUI::pWidget widg, uint32_t oldState, uint32_t newState) {
			bool oldClickedState = bool(oldState & static_cast<uint32_t>(MH33::GUI::WidgetStateFlags::CLICKED));
			bool newClickedState = bool(newState & static_cast<uint32_t>(MH33::GUI::WidgetStateFlags::CLICKED));
			if(oldClickedState != newClickedState) {
				soundSource->setCursor(MH33::Audio::FrameIndex(0));
				soundSource->setState(MH33::Audio::PlayStatus::PLAYING);
			}
			//std::cout << '[' << static_cast<void*>(widg) << "] changed state. Old state: " << oldState << ". New State: " << newState << '.' << std::endl;
		});
		btn->setFlag(true,MH33::GUI::WidgetStateFlags::ENABLED);
		widgets.push_back(MH33::GUI::uWidget(std::move(btn)));
	}
	audioDriver.setPlayable(mixer);
	audioDriver.unlock();
	audioDriver.pause(false);
#if defined (_DEBUG) || defined (DEBUG) || defined (NDEBUG)
		std::cout << "This is a debug build!" << std::endl;
#else
		SDL_ShowCursor(SDL_DISABLE);
#endif
	jscore.run();
}

TestSystem::~TestSystem()
{
	buttonTex = nullptr;
	cursor = nullptr;
	cursorTex = nullptr;
	fontRepo = nullptr;
	tex1 = nullptr;
	tex2 = nullptr;
	screenQuad = nullptr;
	textPipeline = nullptr;
	triangleVbo = nullptr;
	trianglePipeline = nullptr;
	gfx = nullptr;
}

void TestSystem::loadLocalizations()
{
	iosys->enumerate("/localization/english", (MH33::Io::System::FilesystemCallback)[this](MH33::Io::System* system, const std::string& fullpath, const std::string& fname) {
		MH33::Io::uDevice udev(system->open(fullpath, MH33::Io::Mode::READ));
		std::string str = udev->readAllAsString();
		YAML::Node wholeDocument = YAML::Load(str);
		YAML::Node translationYaml = wholeDocument["localization"];
		for(auto it = std::begin(translationYaml) ; it != std::end(translationYaml) ; ++it ) {
			auto key = it->first.as<std::string>();
			auto value = it->second.as<std::string>();
			std::cout << key << '-' << value << std::endl;
			this->localizations.emplace(std::move(key), std::move(value));
		}
	});
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
/*	textPipeline->bind();
	textPipeline->setUniform(textPipeline->getBindingPoint("texture_diffuse"),*tex1,0);
	textPipeline->draw(*screenQuad, MH33::GFX::RenderType::TRIANGLES);
*/
	glm::vec2 sizeReciprocal(2.0f/static_cast<float>(width),2.0f/static_cast<float>(height));
	MH33::TXT::Font::renderTextBlocks(rtp->getBlocks(),glm::fvec2(-0.9f,-0.7f),sizeReciprocal,1.5f,8);
	//guiRenderer->render( MH33::GUI::ColouredQuad{ .topLeft = { -0.75, -0.75 }, .bottomRight = { 0.75, 0.75 }, .clr = { 0.5, 0.5, 0.5, 0.5 } } );
	for(const auto& it : widgets) {
		it->render(*guiRenderer);
	}
	if(cursor) cursor->render(*guiRenderer);
	guiRenderer->flush();
	//trianglePipeline->bind();
	//trianglePipeline->draw(*triangleVbo, MH33::GFX::RenderType::TRIANGLES, 0, 3);
	gfx->endFrame();
}

void TestSystem::update(float deltaTime)
{
	if(cursor) cursor->setPosition(mousePos);
	processCommands();
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
	if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<6> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setNumber(ev.state);
			valarr.elements[3].setNumber(ev.repeat);
			valarr.elements[4].setNumber(static_cast<int32_t>(ev.keysym.scancode));
			valarr.elements[5].setNumber(static_cast<int32_t>(ev.keysym.sym));
			JS::RootedValueArray<6> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}
}

void TestSystem::handleTextEditingEvent(const SDL_TextEditingEvent &ev)
{
	if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<5> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setString(JS_NewStringCopyN(&ctx,ev.text,SDL_TEXTEDITINGEVENT_TEXT_SIZE));
			valarr.elements[3].setNumber(ev.start);
			valarr.elements[4].setNumber(ev.length);
			JS::RootedValueArray<5> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}
}

void TestSystem::handleTextEditingExtEvent(const SDL_TextEditingExtEvent &ev)
{
	if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<5> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setString(JS_NewStringCopyZ(&ctx,ev.text));
			valarr.elements[3].setNumber(ev.start);
			valarr.elements[4].setNumber(ev.length);
			JS::RootedValueArray<5> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			SDL_free(ev.text);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}
}

void TestSystem::handleTextInputEvent(const SDL_TextInputEvent &ev)
{
	if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<3> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setString(JS_NewStringCopyN(&ctx,ev.text,SDL_TEXTEDITINGEVENT_TEXT_SIZE));
			JS::RootedValueArray<3> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}
}

void TestSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent &ev)
{
	mousePos = (glm::fvec2( static_cast<float>(ev.x), static_cast<float>(ev.y) ) * (glm::fvec2(widthR,heightR)*2.0f)) - glm::fvec2(1.0f,1.0f);
	mousePos.y *= -1.0f;
	MH33::GUI::pWidget pcwidg = hoveredOverWidget;
	MH33::GUI::pWidget cwidg = nullptr;
	for(const auto& it : widgets) {
		if( !it->getFlag(MH33::GUI::WidgetStateFlags::ENABLED) ) continue;
		const auto& tl = it->getTopLeft();
		const auto& br = it->getBottomRight();
		if( (mousePos.x >= tl.x && mousePos.x <= br.x) && (mousePos.y <= tl.y && mousePos.y >= br.y) ) {
				cwidg = it.get();
				break;
		}
	}
	if(pcwidg != cwidg) {
		if(pcwidg) {
			pcwidg->setFlag(false,MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS);
		}
		if(cwidg) {
			cwidg->setFlag(true,MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS);
		}
	}
	hoveredOverWidget = cwidg;
	/*if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<7> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setNumber(ev.which);
			valarr.elements[3].setNumber(ev.state);
			valarr.elements[4].setNumber(ev.x);
			valarr.elements[5].setNumber(ev.y);
			valarr.elements[6].setNumber(ev.xrel);
			valarr.elements[6].setNumber(ev.yrel);
			JS::RootedValueArray<7> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}*/
}

void TestSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent &ev)
{
	if(hoveredOverWidget) {
		const auto& tl = hoveredOverWidget->getTopLeft();
		hoveredOverWidget->onClick(mousePos - tl,ev.button,ev.state,ev.clicks);
	}
	/*if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<8> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setNumber(ev.which);
			valarr.elements[3].setNumber(ev.state);
			valarr.elements[4].setNumber(ev.button);
			valarr.elements[5].setNumber(ev.state);
			valarr.elements[6].setNumber(ev.button);
			valarr.elements[6].setNumber(ev.x);
			valarr.elements[7].setNumber(ev.y);
			JS::RootedValueArray<8> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}*/
}

void TestSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent &ev)
{
	if(jsSideEventHandlers.contains(ev.type)) {
		jscore.executeWithinContext([this,&ev](JSContext& ctx) {
			auto it = jsSideEventHandlers[ev.type];
			JS::RootedObject rootedF(&ctx, JS_GetFunctionObject(it));
			JS::ValueArray<9> valarr;
			valarr.elements[0].setNumber(ev.timestamp);
			valarr.elements[1].setNumber(ev.windowID);
			valarr.elements[2].setNumber(ev.which);
			valarr.elements[3].setNumber(ev.x);
			valarr.elements[4].setNumber(ev.y);
			valarr.elements[5].setNumber(ev.direction);
			valarr.elements[6].setNumber(ev.preciseX);
			valarr.elements[6].setNumber(ev.preciseY);
			valarr.elements[7].setNumber(ev.mouseX);
			valarr.elements[8].setNumber(ev.mouseY);
			JS::RootedValueArray<9> args(&ctx, valarr);
			JS::RootedValue retval(&ctx);
			JS_CallFunction(&ctx,rootedF,it,args, &retval);
		});
	}
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

MH33::Audio::Mixer& TestSystem::getMixer()
{
	return *mixer;
}

const MH33::Audio::Mixer& TestSystem::getMixer() const
{
	return *mixer;
}

std::vector<MH33::GUI::uWidget>& TestSystem::getWidgets()
{
	return widgets;
}
