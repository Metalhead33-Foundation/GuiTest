/**
 * @file LtePipeline.cpp
 * @brief Implements shader compilation and pipeline ownership declared in LtePipeline.hpp.
 *
 * The glslang workflow is deliberately one-pass: the includer is handed straight
 * to `TShader::parse()`, so the source is tokenised exactly once. The legacy
 * implementation preprocessed the source into a string first, parsed that string
 * again, and leaked the include cache; all of that is gone.
 */
#include "LtePipeline.hpp"
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace Lotte {
namespace Gfx {

ShaderIncludeResolver::~ShaderIncludeResolver() = default;

IoShaderIncludeResolver::IoShaderIncludeResolver(Elv::Io::System& iosys, std::string_view directory)
	: iosys_(&iosys)
	, directory_(directory)
{
}

bool IoShaderIncludeResolver::resolve(std::string_view name, std::string_view, std::string& outSource)
{
	std::string path(directory_);
	path.append(name);
	if (!iosys_->exists(path.c_str())) {
		return false;
	}
	// The I/O device owns the bytes; copy them into the caller's string so the
	// resolver stays independent of device lifetime.
	auto device = iosys_->openUnique(path.c_str(), Elv::Io::Mode::READ);
	if (!device) {
		return false;
	}
	const std::vector<std::byte> bytes = device->readAllAsVector<std::byte>();
	outSource.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	return true;
}

ShaderBinary::ShaderBinary()
	: stage_(Kld::ShaderStage::Vertex)
{
}

ShaderBinary::ShaderBinary(Kld::ShaderStage stage, std::vector<uint32_t> words)
	: stage_(stage)
	, words_(std::move(words))
{
}

Kld::ShaderBinaryDescriptor ShaderBinary::descriptor() const noexcept
{
	Kld::ShaderBinaryDescriptor descriptor{};
	descriptor.stage = stage_;
	descriptor.spirv = words_.data();
	descriptor.wordCount = words_.size();
	return descriptor;
}

Pipeline::Pipeline() = default;

Pipeline::Pipeline(std::vector<ShaderBinary> shaderBinaries,
				   std::vector<Kld::AttributeDescriptor> attributes,
				   std::size_t stride)
	: shaderBinaries_(std::move(shaderBinaries))
	, attributes_(std::move(attributes))
	, stride_(stride)
{
}

Kld::VertexDescriptor Pipeline::vertexInput() const noexcept
{
	Kld::VertexDescriptor descriptor{};
	descriptor.stride = stride_;
	descriptor.descriptors = std::span<const Kld::AttributeDescriptor>(attributes_.data(), attributes_.size());
	return descriptor;
}

void Pipeline::setVertexAttributes(std::vector<Kld::AttributeDescriptor> attributes, std::size_t stride)
{
	attributes_ = std::move(attributes);
	stride_ = stride;
}

std::vector<Kld::ShaderBinaryDescriptor> Pipeline::shaderDescriptors() const
{
	std::vector<Kld::ShaderBinaryDescriptor> descriptors;
	descriptors.reserve(shaderBinaries_.size());
	for (const ShaderBinary& binary : shaderBinaries_) {
		descriptors.push_back(binary.descriptor());
	}
	return descriptors;
}

const ShaderBinary* Pipeline::findStage(Kld::ShaderStage stage) const noexcept
{
	for (const ShaderBinary& binary : shaderBinaries_) {
		if (binary.stage() == stage) {
			return &binary;
		}
	}
	return nullptr;
}

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// glslang plumbing
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Initialises glslang exactly once for the lifetime of the process. */
class GlslangRuntime {
public:
	GlslangRuntime() { glslang::InitializeProcess(); }
	~GlslangRuntime() { glslang::FinalizeProcess(); }
	GlslangRuntime(const GlslangRuntime&) = delete;
	GlslangRuntime& operator=(const GlslangRuntime&) = delete;
};

/** @brief Ensures glslang's global pool exists before the first parse. */
void ensureGlslangInitialised()
{
	static GlslangRuntime runtime;
	(void)runtime;
}

/** @brief Maps a Kaldi pipeline stage onto glslang's stage enum. */
EShLanguage toGlslangStage(Kld::ShaderStage stage)
{
	switch (stage) {
		case Kld::ShaderStage::Vertex: return EShLangVertex;
		case Kld::ShaderStage::Fragment: return EShLangFragment;
		case Kld::ShaderStage::Geometry: return EShLangGeometry;
		case Kld::ShaderStage::TessControl: return EShLangTessControl;
		case Kld::ShaderStage::TessEvaluation: return EShLangTessEvaluation;
		case Kld::ShaderStage::Compute: return EShLangCompute;
	}
	return EShLangVertex;
}

/** @brief Human-readable stage name for diagnostics. */
const char* stageName(Kld::ShaderStage stage)
{
	switch (stage) {
		case Kld::ShaderStage::Vertex: return "vertex";
		case Kld::ShaderStage::Fragment: return "fragment";
		case Kld::ShaderStage::Geometry: return "geometry";
		case Kld::ShaderStage::TessControl: return "tessellation control";
		case Kld::ShaderStage::TessEvaluation: return "tessellation evaluation";
		case Kld::ShaderStage::Compute: return "compute";
	}
	return "unknown";
}

/**
 * @brief `glslang::TShader::Includer` over a `ShaderIncludeResolver`.
 *
 * Every resolved include is owned by this object until glslang calls
 * `releaseInclude()`, which is exactly the lifetime contract glslang documents.
 */
class GlslangIncluder final : public glslang::TShader::Includer {
public:
	explicit GlslangIncluder(ShaderIncludeResolver* resolver)
		: resolver_(resolver)
	{
	}

	IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t /*inclusionDepth*/) override
	{
		return include(headerName, includerName);
	}

	IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t /*inclusionDepth*/) override
	{
		return include(headerName, includerName);
	}

	void releaseInclude(IncludeResult* result) override
	{
		if (result == nullptr) {
			return;
		}
		for (auto it = entries_.begin(); it != entries_.end(); ++it) {
			if (it->get() == result->userData) {
				entries_.erase(it);
				return;
			}
		}
	}

private:
	/** @brief One resolved include; the strings must outlive the `IncludeResult`. */
	struct Entry {
		Entry(std::string requestedName, std::string contents)
			: name(std::move(requestedName))
			, text(std::move(contents))
			, result(name, text.data(), text.size(), this)
		{
		}
		std::string name;
		std::string text;
		IncludeResult result;
	};

	IncludeResult* include(const char* headerName, const char* includerName)
	{
		if (resolver_ == nullptr || headerName == nullptr) {
			return nullptr;
		}
		std::string contents;
		if (!resolver_->resolve(headerName, includerName != nullptr ? includerName : "", contents)) {
			return nullptr;
		}
		auto entry = std::make_unique<Entry>(headerName, std::move(contents));
		entries_.push_back(std::move(entry));
		return &entries_.back()->result;
	}

	ShaderIncludeResolver* resolver_;
	std::vector<std::unique_ptr<Entry>> entries_;
};

/** @brief Builds the exception text for a failed parse. */
std::string parseErrorMessage(const ShaderStageSource& stage, glslang::TShader& shader)
{
	std::string message = "Lotte::Gfx: failed to compile the ";
	message.append(stageName(stage.stage));
	message.append(" stage");
	if (!stage.debugName.empty()) {
		message.append(" (");
		message.append(stage.debugName);
		message.append(")");
	}
	message.append(":\n");
	message.append(shader.getInfoLog());
	const char* debugLog = shader.getInfoDebugLog();
	if (debugLog != nullptr && *debugLog != '\0') {
		message.append("\n");
		message.append(debugLog);
	}
	return message;
}

/** @brief Builds the exception text for a failed program link. */
std::string linkErrorMessage(glslang::TProgram& program)
{
	std::string message = "Lotte::Gfx: failed to link the shader program:\n";
	message.append(program.getInfoLog());
	const char* debugLog = program.getInfoDebugLog();
	if (debugLog != nullptr && *debugLog != '\0') {
		message.append("\n");
		message.append(debugLog);
	}
	return message;
}

} // namespace

std::vector<ShaderBinary> compileShaderProgram(ShaderIncludeResolver* includes,
											   std::span<const ShaderStageSource> stages,
											   const ShaderCompileOptions& options)
{
	if (stages.empty()) {
		throw std::invalid_argument("Lotte::Gfx::compileShaderProgram: no shader stages were supplied");
	}
	ensureGlslangInitialised();

	EShMessages messages = EShMsgSpvRules;
	glslang::EShClient client = glslang::EShClientOpenGL;
	glslang::EShTargetClientVersion clientVersion = glslang::EShTargetOpenGL_450;
	switch (options.target) {
		case SpirvTarget::Vulkan:
			client = glslang::EShClientVulkan;
			clientVersion = glslang::EShTargetVulkan_1_0;
			messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
			break;
		case SpirvTarget::OpenGL:
		case SpirvTarget::OpenGLES:
			// GLSL ES is host-side identical to OpenGL from glslang's point of view;
			// the ES specific output is produced later by the SPIR-V -> GLSL step.
			client = glslang::EShClientOpenGL;
			clientVersion = glslang::EShTargetOpenGL_450;
			messages = EShMsgSpvRules;
			break;
	}
	const glslang::EShSource source = (options.language == ShaderSourceLanguage::Hlsl) ? glslang::EShSourceHlsl : glslang::EShSourceGlsl;
	// TShader keeps the pointer it is given, so the string has to outlive the parse.
	const std::string entryPoint(options.entryPoint);

	// glslang 16 exports the default limits in the global namespace; older
	// releases declared the same function inside `namespace glslang`.
	const TBuiltInResource* resources = ::GetDefaultResources();
	if (resources == nullptr) {
		throw std::runtime_error("Lotte::Gfx::compileShaderProgram: glslang has no default resource limits");
	}

	glslang::TProgram program;
	GlslangIncluder includer(includes);
	// TProgram only stores raw pointers, so the shaders must stay alive here.
	std::vector<std::unique_ptr<glslang::TShader>> shaders;
	shaders.reserve(stages.size());
	std::vector<Kld::ShaderStage> seenStages;
	seenStages.reserve(stages.size());

	for (const ShaderStageSource& stage : stages) {
		if (stage.source.empty()) {
			throw std::invalid_argument(std::string("Lotte::Gfx::compileShaderProgram: empty ") +
										stageName(stage.stage) + " stage source");
		}
		for (Kld::ShaderStage previous : seenStages) {
			if (previous == stage.stage) {
				throw std::invalid_argument(std::string("Lotte::Gfx::compileShaderProgram: duplicate ") +
											stageName(stage.stage) + " stage");
			}
		}
		seenStages.push_back(stage.stage);

		const EShLanguage language = toGlslangStage(stage.stage);
		auto shader = std::make_unique<glslang::TShader>(language);

		// `setStringsWithLengths` accepts non-terminated views, which is exactly
		// what std::string_view gives us.
		const char* sourceText = stage.source.data();
		const int sourceLength = static_cast<int>(stage.source.size());
		shader->setStringsWithLengths(&sourceText, &sourceLength, 1);
		shader->setEnvInput(source, language, client, options.defaultVersion);
		shader->setEnvClient(client, clientVersion);
		shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
		shader->setEntryPoint(entryPoint.c_str());
		shader->setSourceEntryPoint(entryPoint.c_str());
		shader->setAutoMapBindings(options.autoMapBindings);
		shader->setAutoMapLocations(options.autoMapBindings);

		if (!shader->parse(resources, options.defaultVersion, false, messages, includer)) {
			throw std::runtime_error(parseErrorMessage(stage, *shader));
		}
		program.addShader(shader.get());
		shaders.push_back(std::move(shader));
	}

	if (!program.link(messages)) {
		throw std::runtime_error(linkErrorMessage(program));
	}

	glslang::SpvOptions spvOptions;
	spvOptions.disableOptimizer = false;
	spvOptions.generateDebugInfo = options.generateDebugInfo;
	spvOptions.optimizeSize = options.optimizeSize;
	spvOptions.validate = options.validate;
	spvOptions.compileOnly = false;
	spvOptions.disassemble = false;

	std::vector<ShaderBinary> binaries;
	binaries.reserve(stages.size());
	for (const ShaderStageSource& stage : stages) {
		glslang::TIntermediate* intermediate = program.getIntermediate(toGlslangStage(stage.stage));
		if (intermediate == nullptr) {
			throw std::runtime_error(std::string("Lotte::Gfx::compileShaderProgram: no linked program for the ") +
									 stageName(stage.stage) + " stage");
		}
		std::vector<uint32_t> words;
		glslang::GlslangToSpv(*intermediate, words, &spvOptions);
		if (words.empty()) {
			throw std::runtime_error(std::string("Lotte::Gfx::compileShaderProgram: glslang produced no SPIR-V for the ") +
									 stageName(stage.stage) + " stage");
		}
		binaries.emplace_back(stage.stage, std::move(words));
	}
	return binaries;
}

}
}
