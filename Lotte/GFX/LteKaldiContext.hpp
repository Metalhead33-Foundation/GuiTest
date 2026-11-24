#ifndef LTEKALDICONTEXT_HPP
#define LTEKALDICONTEXT_HPP
/**
 * @file LteKaldiContext.hpp
 * @brief Lotte's rendering context: a Kaldi backend loaded at run time, its device, and the frame loop.
 *
 * This is the promoted form of `Sandbox/KaldiContext.{hpp,cpp}`. The reasoning did
 * not change - a host should *pick* a backend, not link one, because the backends
 * are separate shared libraries that export one C ABI and cannot be linked side by
 * side - but the ownership now belongs to Lotte so every consumer of the graphics
 * stack gets the same loader, resource factory and frame recording.
 *
 * @section lte_kaldi_context_frame Frame protocol
 *
 * A frame is a command batch, not an API call sequence:
 *
 * @code
 * context.beginFrame();        // BeginFrame + ClearColorAttachment
 * renderer.render(...);        // draws record into the same dispatcher
 * context.endFrame();          // EndFrame + Present, then swap and submit
 * @endcode
 *
 * `beginFrame()`/`endFrame()` are the only places that submit, so resources created
 * between them travel in the same command span as the frame that first uses them.
 *
 * @section lte_kaldi_context_lifetime Lifetime
 *
 * The backend library is declared before the device and destroyed last: the device
 * object and its virtual table live inside it, so `kld_destroy_rendering_device()`
 * runs first, `dlclose` second.
 *
 * Every `Lotte::Texture`/`Buffer`/`Pipeline` proxy created through `resources()`
 * records its destroy command into this context's dispatcher when the proxy dies,
 * so proxies must be destroyed **before** the context. Declare the context first
 * among the graphics members of a host class and the member order guarantees it.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Kaldi/KaldiLib.h>
#include <Kaldi/KldDevice.hpp>
#include <Kaldi/KldHal.hpp>
#include <Elvavena/Util/ElvDynamicLib.hpp>
#include <SDL2/SDL_syswm.h>
#include <memory>
#include <string>
#include <string_view>

namespace Lotte {
namespace Gfx {

/** @brief Everything a context needs beyond the window and its size. */
struct KaldiContextSettings {
	/**
	 * @brief Backend library to load.
	 *
	 * Either an absolute path or a plain soname the loader resolves through
	 * `LD_LIBRARY_PATH` (`libKaldiVK.so`, `libKaldiGL.so`, `libKaldiGLES.so`).
	 */
	std::string backendLibrary = "libKaldiVK.so";
	/** @brief Linear RGBA the window is cleared to every frame. */
	float clearColor[4] = { 0.05f, 0.06f, 0.09f, 1.0f };
};

/** @brief Run-time-loaded Kaldi backend, its device, and the recording dispatcher. */
class MH_LOTTE_API KaldiContext {
public:
	/**
	 * @brief Loads @p settings.backendLibrary and creates its device for @p window.
	 * @param window SDL window-manager info of the window to render into.
	 * @param width Initial surface width in device pixels.
	 * @param height Initial surface height in device pixels.
	 * @param settings Backend selection and clear colour.
	 * @throws std::runtime_error When the library cannot be loaded, does not export
	 *         the backend entry points, or refuses to create a device. The message
	 *         carries the backend's own diagnostics when it has any.
	 */
	KaldiContext(const SDL_SysWMinfo& window, std::uint16_t width, std::uint16_t height,
		     const KaldiContextSettings& settings = {});

	/** @brief Destroys the device, then unloads the backend library. */
	~KaldiContext();

	KaldiContext(const KaldiContext&) = delete;
	KaldiContext& operator=(const KaldiContext&) = delete;
	KaldiContext(KaldiContext&&) = delete;
	KaldiContext& operator=(KaldiContext&&) = delete;

	/** @brief The loaded device, or @c nullptr when the backend declined to create one. */
	Kld::Device* device() const noexcept { return device_; }
	/** @brief True when a device exists and can be submitted to. */
	bool isValid() const noexcept { return device_ != nullptr; }

	/** @brief The portable capability snapshot of the device. */
	const Kld::HalCapabilities& capabilities() const noexcept { return capabilities_; }

	/** @brief The backend's own name, for logs (`kld_get_renderer_name`). */
	std::string_view rendererName() const noexcept { return rendererName_; }
	/** @brief The backend library version string (`kld_get_version`). */
	std::string_view backendVersion() const noexcept { return backendVersion_; }
	/** @brief The HAL contract version the device reports. */
	std::string halVersionName() const;

	/** @brief The dispatcher every resource and frame command is recorded into. */
	CommandBuffer& commands() noexcept { return commands_; }
	/** @brief The resource factory bound to @ref commands() and @ref device(). */
	ResourceFactory& resources() noexcept { return resources_; }

	/** @brief Replaces the clear colour used by @ref beginFrame(). */
	void setClearColor(const float (&color)[4]) noexcept;
	/** @brief The clear colour used by @ref beginFrame(). */
	const float (&clearColor() const noexcept)[4] { return clearColor_; }

	/**
	 * @brief Records `BeginFrame` and clears the default framebuffer.
	 * @return `Succeeded`, or a status describing why the frame could not start.
	 */
	Kld::HalStatus beginFrame();
	/**
	 * @brief Records `EndFrame` and `Present`, then swaps and submits the batch.
	 * @return The device's status for the whole frame.
	 */
	Kld::HalStatus endFrame();
	/** @brief Swaps and submits whatever has been recorded so far. */
	Kld::HalStatus submit();

private:
	/**
	 * @brief The loaded backend library.
	 *
	 * Declared first so it is destroyed *last*: the device pointer and every entry
	 * point below live inside it.
	 */
	std::unique_ptr<Elv::Util::DynamicLibrary> library_;
	MhDestroyRenderingDevice destroyDevice_ = nullptr;
	MhGetLastError getLastError_ = nullptr;
	/** @brief Raw C handle; released with @ref destroyDevice_. */
	MhRenderingDeviceHandle handle_ = nullptr;
	/** @brief The same object as @ref handle_, viewed as its C++ interface. */
	Kld::Device* device_ = nullptr;
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
	std::string rendererName_;
	std::string backendVersion_;
	float clearColor_[4] = { 0.05f, 0.06f, 0.09f, 1.0f };
	/** @brief Recording dispatcher; destroyed before the device. */
	CommandBuffer commands_;
	/** @brief Factory over @ref commands_ and @ref device_. */
	ResourceFactory resources_;
};

} // namespace Gfx
} // namespace Lotte

#endif // LTEKALDICONTEXT_HPP
