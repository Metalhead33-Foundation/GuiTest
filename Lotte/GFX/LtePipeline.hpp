#ifndef LTEPIPELINE_HPP
#define LTEPIPELINE_HPP
/**
 * @file LtePipeline.hpp
 * @brief Shader compilation and graphics-pipeline ownership for Lotte.
 *
 * @section ltepipeline_design Design notes
 *
 * The legacy version of this file carried two throw-away structs
 * (`ShaderModuleCreateInfo` / `ShaderModuleCreateInfoRef`) so that code could be
 * written "quickly". They are gone; the wire format between Lotte and Kaldi is
 * `Kld::ShaderBinaryDescriptor`, and `ShaderBinary` below is its owning
 * counterpart (Kaldi's descriptor is a non-owning view by contract).
 *
 * **Does shader compilation belong in Lotte?** Yes, but only as a library
 * function, never as a hidden side effect:
 *
 *  - the same `compileShaderProgram()` can be driven by a build tool for offline
 *    pre-compilation *or* at runtime for hot-reload/generated shaders, because it
 *    has no global state beyond glslang's one-time initialisation;
 *  - Lotte stops at SPIR-V. Cross-compiling SPIR-V to GLSL/HLSL is a backend
 *    concern (`KaldiGLCommon/GlSpirvShader.hpp`, `KaldiD3DCommon/D3DSPirvShader.hpp`),
 *    exactly like the Vulkan backend consuming SPIR-V directly.
 *
 * **Which compiler?** Today it is the HLSL front-end of glslang. That front-end is
 * deprecated upstream (as of April 2026, removed at the next major version; see
 * KhronosGroup/glslang issue #4210) and the intended replacement is DXC, whose
 * only notable flaw for this engine is that it targets Shader Model 6.x while
 * shaders here are authored at Shader Model 4/5 level. The migration is deferred
 * indefinitely; `docs/graphics/SHADERS.md` records the rationale, the reference `dxc`
 * invocation and the acceptance gate. Nothing in this header changes either way:
 * the contract is SPIR-V 1.0 words plus a stage, not a compiler.
 *
 * **Is "Pipeline" too low level for Lotte?** A `Pipeline` here is a thin owning
 * container of compiled stages plus the vertex input Kaldi needs. Materials,
 * effect graphs and node-based shader authoring are a higher layer that does not
 * exist yet; when it does, it will *produce* a `Pipeline`, not replace it. That
 * decision is recorded here instead of being left as a TODO.
 *
 * @section ltepipeline_usage Typical use
 *
 * @code
 * Lotte::Gfx::IoShaderIncludeResolver includes(iosys);
 * std::vector<Lotte::Gfx::ShaderStageSource> stages = {
 *     { Kld::ShaderStage::Vertex,   vertexSource,   "ui.vs.hlsl" },
 *     { Kld::ShaderStage::Fragment, fragmentSource, "ui.ps.hlsl" },
 * };
 * Lotte::Gfx::Pipeline pipeline(Lotte::Gfx::compileShaderProgram(&includes, stages), attributes, stride);
 * auto binaries = pipeline.shaderDescriptors();  // hand these to Kld::oop_detail<>::createPipeline
 * @endcode
 */
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <Lotte/LotteLib.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Gfx {

/** @brief Shading language the stage sources are authored in. */
enum class ShaderSourceLanguage : uint8_t {
	/** @brief High-level shading language (the engine's authoring language). */
	Hlsl,
	/** @brief OpenGL shading language. */
	Glsl
};

/**
 * @brief Resolution hook for `#include` directives.
 *
 * Lotte deliberately does not depend on a specific file system here, so the
 * engine can feed includes from a PhysFS archive, a memory blob or the real
 * filesystem. Implementations must return the *complete* text of the include.
 */
class MH_LOTTE_API ShaderIncludeResolver {
public:
	/** @brief Virtual destructor for implementations. */
	virtual ~ShaderIncludeResolver();

	/**
	 * @brief Resolves one include request.
	 * @param name The name written inside the `#include` directive.
	 * @param requestingFile The file that contains the directive (may be empty).
	 * @param outSource Receives the include text on success.
	 * @return @c true when the include was resolved.
	 */
	virtual bool resolve(std::string_view name, std::string_view requestingFile, std::string& outSource) = 0;
};

/**
 * @brief `ShaderIncludeResolver` backed by an Elvavena I/O system.
 *
 * The provided directory (default `/shaders/`) is prepended to every include name.
 */
class MH_LOTTE_API IoShaderIncludeResolver final : public ShaderIncludeResolver {
public:
	/**
	 * @brief Creates a resolver over @p iosys.
	 * @param iosys The I/O system include files are read from.
	 * @param directory Prefix prepended to every include name.
	 */
	explicit IoShaderIncludeResolver(Elv::Io::System& iosys, std::string_view directory = "/shaders/");

	/** @copydoc ShaderIncludeResolver::resolve */
	bool resolve(std::string_view name, std::string_view requestingFile, std::string& outSource) override;

private:
	Elv::Io::System* iosys_;
	std::string directory_;
};

/** @brief One stage of a shader program, as authored by the engine. */
struct ShaderStageSource {
	/** @brief Pipeline stage this source implements. */
	Kld::ShaderStage stage = Kld::ShaderStage::Vertex;
	/** @brief Complete source text; must stay alive for the duration of the call. */
	std::string_view source;
	/** @brief Optional file/debug name used in diagnostics and include resolution. */
	std::string_view debugName;
};

/**
 * @brief Options for compileShaderProgram().
 *
 * @note There is deliberately no "target environment" option. Lotte always emits
 * **Vulkan** SPIR-V, because that single blob has to serve every backend:
 *
 *  - the Vulkan backend consumes it as-is, so it must be Vulkan-legal
 *    (a `-G`/OpenGL module carries `OriginLowerLeft` and extra built-in inputs that
 *    Vulkan rejects);
 *  - the GL/GLES and D3D11/D3D12 backends cross-compile it with SPIRV-Cross, whose
 *    tested and documented input is Vulkan SPIR-V;
 *  - glslang itself refuses HLSL input under OpenGL semantics
 *    ("Using HLSL input under OpenGL semantics is not currently supported"), so an
 *    OpenGL target could never compile HLSL at all.
 *
 * Consequently the SPIR-V -> GLSL/GLSL ES/HLSL translation belongs to the backends
 * (`KaldiGLCommon/GlSpirvShader`, `KaldiD3DCommon/D3DSpirvShader`), not to Lotte.
 */
struct ShaderCompileOptions {
	/** @brief Language of the supplied sources. */
	ShaderSourceLanguage language = ShaderSourceLanguage::Hlsl;
	/** @brief Entry point name inside every stage. */
	std::string_view entryPoint = "main";
	/** @brief Automatically assign binding/location decorations when the target needs it. */
	bool autoMapBindings = true;
	/** @brief Emit SPIR-V debug info (`OpLine`, names). */
	bool generateDebugInfo = false;
	/** @brief Optimise the SPIR-V for size instead of speed. */
	bool optimizeSize = false;
	/** @brief Run the SPIR-V validator after translation. */
	bool validate = true;
	/**
	 * @brief GLSL version glslang assumes when a source has no `#version` directive.
	 *
	 * Ignored for HLSL input. Vulkan SPIR-V requires a modern GLSL version, so the
	 * default is 450 rather than glslang's own ancient default.
	 */
	int defaultVersion = 450;
};

/**
 * @brief Owning SPIR-V blob tagged with its pipeline stage.
 *
 * Kaldi's `Kld::ShaderBinaryDescriptor` is a non-owning `(stage, words, count)`
 * view; `ShaderBinary` is the owning counterpart so shader binaries can be
 * stored, moved and released safely. `descriptor()` produces a view that stays
 * valid as long as this object is alive and not modified.
 */
class MH_LOTTE_API ShaderBinary {
public:
	/** @brief Creates an empty binary; `stage()` is `Vertex` and `empty()` is true. */
	ShaderBinary();

	/**
	 * @brief Creates a binary from already compiled SPIR-V words.
	 * @param stage Pipeline stage the words implement.
	 * @param words SPIR-V words (magic number included).
	 */
	ShaderBinary(Kld::ShaderStage stage, std::vector<uint32_t> words);

	/** @brief Pipeline stage this binary implements. */
	Kld::ShaderStage stage() const noexcept { return stage_; }
	/** @brief Read-only access to the SPIR-V words. */
	const std::vector<uint32_t>& words() const noexcept { return words_; }
	/** @brief Mutable access to the SPIR-V words (invalidates outstanding descriptors). */
	std::vector<uint32_t>& words() noexcept { return words_; }
	/** @brief True when no SPIR-V words are stored. */
	bool empty() const noexcept { return words_.empty(); }
	/** @brief Size of the blob in bytes. */
	std::size_t byteSize() const noexcept { return words_.size() * sizeof(uint32_t); }

	/** @brief Non-owning Kaldi view over the stored words. */
	Kld::ShaderBinaryDescriptor descriptor() const noexcept;

private:
	Kld::ShaderStage stage_;
	std::vector<uint32_t> words_;
};

/**
 * @brief Owns the shader binaries and vertex input of one graphics pipeline.
 *
 * The class deliberately does not own Kaldi handles: creating the backend
 * pipeline happens through `Kld::oop_detail<>::createPipeline`, so a Lotte
 * pipeline can be reused for several backends or recorded into several command
 * buffers.
 */
class MH_LOTTE_API Pipeline {
public:
	/** @brief Creates an empty pipeline. */
	Pipeline();

	/**
	 * @brief Creates a pipeline from compiled stages and a vertex layout.
	 * @param shaderBinaries Compiled stages; at least one should be present.
	 * @param attributes Attribute descriptors describing the vertex stream.
	 * @param stride Byte size of one vertex.
	 */
	Pipeline(std::vector<ShaderBinary> shaderBinaries,
			 std::vector<Kld::AttributeDescriptor> attributes,
			 std::size_t stride);

	/** @brief The compiled stages. */
	const std::vector<ShaderBinary>& shaderBinaries() const noexcept { return shaderBinaries_; }
	/** @brief Mutable access to the compiled stages (invalidates outstanding descriptors). */
	std::vector<ShaderBinary>& shaderBinaries() noexcept { return shaderBinaries_; }

	/**
	 * @brief Kaldi vertex input view.
	 *
	 * The returned span points into this object and stays valid until the
	 * attribute list is replaced or the object is moved.
	 */
	Kld::VertexDescriptor vertexInput() const noexcept;

	/** @brief Replaces the vertex attribute list used by vertexInput(). */
	void setVertexAttributes(std::vector<Kld::AttributeDescriptor> attributes, std::size_t stride);

	/**
	 * @brief Builds the descriptor array Kaldi's create-pipeline command expects.
	 *
	 * The descriptors point into the stored binaries; keep this object alive and
	 * unmodified until the backend has processed the command queue.
	 */
	std::vector<Kld::ShaderBinaryDescriptor> shaderDescriptors() const;

	/** @brief Finds a stage by name; returns @c nullptr when absent. */
	const ShaderBinary* findStage(Kld::ShaderStage stage) const noexcept;

private:
	std::vector<ShaderBinary> shaderBinaries_;
	std::vector<Kld::AttributeDescriptor> attributes_;
	std::size_t stride_ = 0;
};

/**
 * @brief Compiles a set of stages as one glslang program and emits one SPIR-V blob per stage.
 *
 * All stages are parsed and linked together, so interface mismatches between
 * (for example) a vertex and a fragment stage are reported by glslang instead of
 * by the driver. The ordering of the result matches the ordering of @p stages.
 *
 * @param includes Optional include resolver; pass @c nullptr to reject `#include`.
 * @param stages Source of every stage of the program.
 * @param options Language, target environment and SPIR-V tuning knobs.
 * @return One SPIR-V binary per entry of @p stages.
 *
 * @throws std::invalid_argument When the stage list is empty, a stage is duplicated
 *         or a source is empty.
 * @throws std::runtime_error When glslang fails to parse or link the program; the
 *         message contains glslang's info log.
 *
 * @note glslang keeps a small amount of process-global state; the first call
 * initialises it. Do not call this from several threads at the same time unless
 * your glslang build is thread-safe.
 */
std::vector<ShaderBinary> compileShaderProgram(ShaderIncludeResolver* includes,
											   std::span<const ShaderStageSource> stages,
											   const ShaderCompileOptions& options = {});

}
}

#endif // LTEPIPELINE_HPP
