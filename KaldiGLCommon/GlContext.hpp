#ifndef KALDIGLCONTEXT_HPP
#define KALDIGLCONTEXT_HPP
/**
 * @file GlContext.hpp
 * @brief Window-system independent OpenGL / OpenGL ES context contract.
 *
 * A Kaldi GL-family backend needs three things from the platform before it can
 * execute a single command: an initialized display, a context with a drawable,
 * and loaded GL entry points. This header is the portable half of that:
 *
 *  - `Context` - the abstract context a backend owns and makes current;
 *  - `ContextConfig` - what the caller may tune (version, profile, buffer
 *    sizes, MSAA, ...);
 *  - `Kld::NativeWindowHandles` (see `Kaldi/KldNativeWindow.hpp`) - the
 *    backend-neutral window handles this layer consumes;
 *  - createContext() - the factory a backend calls, which picks EGL or GLX for
 *    desktop OpenGL and always uses EGL for OpenGL ES.
 *
 * The concrete implementations live next to it:
 *
 * | Class | Header | Built into |
 * |---|---|---|
 * | `EglContext` | `EglContext.hpp` | `KaldiGL` and `KaldiGLES` |
 * | `GlxContext` | `GlxContext.hpp` (desktop only) | `KaldiGL` |
 *
 * @section kaldigl_context_configuration Configuration
 *
 * `ContextConfig` is deliberately a plain aggregate: it is the future mapping
 * surface for a configuration file. The plan is that a backend receives a
 * callback returning the defaults it wants for a given purpose, and those
 * defaults are persisted through `Euph::Conf::Configuration` (`.ini` sections
 * with typed `key=value` lines). Until that exists, callers fill the struct in
 * directly - see `KaldiGLCommon/README.md` for the proposed callback shape.
 *
 * @section kaldigl_context_lifetime Lifetime and threading
 *
 * EGL/GLX contexts are current *per thread*. makeCurrent()/releaseCurrent() must
 * be called from the thread that issues GL commands, and a `Context` must
 * outlive every object created in it (textures, buffers, pipelines). The GL
 * entry points loaded by loadGlFunctions() are process-wide pointers, so
 * unloadGlFunctions() must only be called after the last context is gone.
 */
#include "GlBackend.hpp"
#include <Kaldi/KldNativeWindow.hpp>
#include <cstdint>
#include <memory>
#include <string_view>

namespace KALDIGL_NS {

// ─────────────────────────────────────────────────────────────────────────────
// API selection
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Window-system binding used to create the GL context. */
enum class ContextApi : uint8_t {
	/** @brief Pick the best binding for the window (see createContext()). */
	Auto,
	/** @brief EGL; the only option for OpenGL ES, and also usable for desktop OpenGL. */
	EGL,
	/** @brief GLX; desktop OpenGL on X11 only. */
	GLX,
	/** @brief WGL; desktop OpenGL on Win32 only. */
	WGL
};

/** @brief Stable, human-readable name of a context API ("auto", "EGL", "GLX"). */
constexpr std::string_view contextApiName(ContextApi api) noexcept
{
	switch (api) {
		case ContextApi::Auto: return "auto";
		case ContextApi::EGL: return "EGL";
		case ContextApi::GLX: return "GLX";
		case ContextApi::WGL: return "WGL";
	}
	return "auto";
}

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Everything the caller may tune when a context is created.
 *
 * The defaults describe desktop OpenGL 4.3 core; defaultContextConfig() overrides
 * them for the OpenGL ES build of the library (3.2, no profile bits). Sizes are in
 * bits, matching both `EGL_*_SIZE` and `GLX_*_SIZE` attribute values.
 */
struct ContextConfig {
	/** @brief Requested window-system binding. */
	ContextApi api = ContextApi::Auto;
	/** @brief Requested major version (4 on desktop, 3 on GLES). */
	int versionMajor = 4;
	/** @brief Requested minor version (3 on desktop, 2 on GLES). */
	int versionMinor = 3;
	/** @brief Request a core profile (desktop only; ignored on GLES). */
	bool coreProfile = true;
	/** @brief Request a forward-compatible context (desktop only). */
	bool forwardCompatible = true;
	/** @brief Request a debug context when the driver offers one. */
	bool debugContext = false;
	/** @brief Request a double-buffered drawable. */
	bool doubleBuffered = true;
	/** @brief Red channel size in bits. */
	int redBits = 8;
	/** @brief Green channel size in bits. */
	int greenBits = 8;
	/** @brief Blue channel size in bits. */
	int blueBits = 8;
	/** @brief Alpha channel size in bits. */
	int alphaBits = 8;
	/** @brief Depth buffer size in bits; @c 0 disables the depth buffer. */
	int depthBits = 24;
	/** @brief Stencil buffer size in bits; @c 0 disables the stencil buffer. */
	int stencilBits = 8;
	/** @brief MSAA sample count; @c 0 (or 1) disables multisampling. */
	int sampleCount = 0;
	/** @brief X11 screen index for GLX; -1 asks for the default screen. */
	int screen = -1;
};

/** @brief True when `ContextConfig::sampleCount` requests multisampling. */
constexpr bool wantsMultisampling(const ContextConfig& config) noexcept
{
	return config.sampleCount > 1;
}

/**
 * @brief Returns the dialect's default context configuration.
 *
 * Desktop OpenGL asks for 4.3 core + forward-compatible; OpenGL ES asks for 3.2
 * (the ES profile has no concept of core/compatibility profiles).
 */
constexpr ContextConfig defaultContextConfig() noexcept
{
	ContextConfig config;
#if KALDIGL_IS_GLES
	config.versionMajor = 3;
	config.versionMinor = 2;
	config.coreProfile = false;
	config.forwardCompatible = false;
#endif
	return config;
}

// ─────────────────────────────────────────────────────────────────────────────
// Abstract context
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A current-able OpenGL / OpenGL ES context with one drawable.
 *
 * Implementations own the native context and surface and free them in their
 * destructor; the display they are built on is reference counted by the
 * implementation, so several contexts may share one display.
 */
class Context {
public:
	/** @brief Virtual destructor; implementations release the native context. */
	virtual ~Context() = default;

	/** @brief Makes this context current on the calling thread. */
	virtual void makeCurrent() = 0;

	/**
	 * @brief Unbinds the context from the calling thread.
	 *
	 * Best-effort by contract and therefore `noexcept`: it is called from
	 * destructors, where a failure could only be turned into `std::terminate()`.
	 */
	virtual void releaseCurrent() noexcept = 0;

	/**
	 * @brief Presents the drawable.
	 *
	 * Must be called on a thread where this context is current.
	 */
	virtual void swapBuffers() = 0;

	/** @brief The binding that created this context. */
	virtual ContextApi api() const noexcept = 0;

	/** @brief Human-readable name of api(). */
	virtual std::string_view apiName() const noexcept = 0;

	/** @brief The configuration the context was created with. */
	virtual const ContextConfig& config() const noexcept = 0;

	/** @brief True when the native context and drawable are valid. */
	virtual bool isValid() const noexcept = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// GL entry points
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Loads the GL entry points for the current context through glad.
 * @return glad's version number, or @c 0 when loading failed.
 *
 * Call after makeCurrent(). The pointers are process-wide, so loading is
 * idempotent and only has to be undone once.
 */
int loadGlFunctions() noexcept;

/** @brief Unloads the GL entry points previously loaded by loadGlFunctions(). */
void unloadGlFunctions() noexcept;

/**
 * @brief Applies the GL state Lotte's renderer expects on a fresh context.
 *
 * Currently alpha blending in "src alpha / one minus src alpha" mode. Anything
 * beyond that belongs to the command queue, not to the context.
 */
void applyDefaultState();

// ─────────────────────────────────────────────────────────────────────────────
// Factories
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Creates an EGL context for @p handles.
 * @throws std::invalid_argument When @p handles does not contain a usable drawable.
 * @throws std::runtime_error When EGL fails; the message names the failing step and
 *         the `eglGetError()` code.
 */
std::unique_ptr<Context> createEglContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config);

#if !KALDIGL_IS_GLES
/**
 * @brief Creates a GLX context for @p handles (desktop OpenGL only).
 * @throws std::invalid_argument When @p handles does not contain an X11 display/window.
 * @throws std::runtime_error When GLX fails; the message names the failing step.
 */
std::unique_ptr<Context> createGlxContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config);

#if defined(_WIN32)
/**
 * @brief Creates a WGL context for @p handles (desktop OpenGL on Win32 only).
 * @throws std::invalid_argument When @p handles does not contain an `HWND`.
 * @throws std::runtime_error When WGL fails; the message names the failing step.
 *
 * @note Only compiled on Windows hosts (`win32` scope of `KaldiGL.pro`).
 */
std::unique_ptr<Context> createWglContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config);
#endif
#endif

/**
 * @brief Creates a context for @p handles using the binding selected by @p config.
 *
 * `ContextApi::Auto` resolves from `Kld::NativeWindowHandles::system`: GLX on
 * X11, WGL on Win32, EGL on Wayland/Android - and the OpenGL ES build always ends
 * up on EGL, because it has no GLX/WGL path.
 */
std::unique_ptr<Context> createContext(const Kld::NativeWindowHandles& handles,
									   const ContextConfig& config = defaultContextConfig());

/** @brief Convenience overload extracting the handles from SDL window-manager info. */
std::unique_ptr<Context> createContext(const SDL_SysWMinfo& info,
									   const ContextConfig& config = defaultContextConfig());

}
#endif // KALDIGLCONTEXT_HPP
