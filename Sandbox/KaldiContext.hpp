#ifndef KALDICONTEXT_HPP
#define KALDICONTEXT_HPP
/**
 * @file KaldiContext.hpp
 * @brief Loads a Kaldi backend at run time and owns the rendering device for a window.
 *
 * This is the same idea as `Haruka_SDL_Driver` one layer up: the Sandbox should not
 * link a graphics backend, it should *pick* one. The Kaldi backends are separate
 * shared libraries that export the small C ABI in `Kaldi/KaldiLib.h`, and they are
 * supposed to be loaded, not linked - which is also what lets one binary offer
 * Vulkan, OpenGL and OpenGL ES side by side (they all export the same symbols, so
 * linking more than one is impossible).
 *
 * @section sandbox_kaldi_lifetime Lifetime
 *
 * The library stays loaded until after the device is destroyed, because the device
 * object and its virtual table live inside it: `kld_destroy_rendering_device()` runs
 * first, `dlclose` second. `KaldiContext` declares the library before the device
 * pointer for exactly that reason.
 *
 * @section sandbox_kaldi_submit Submitting commands
 *
 * The C ABI has no `submit` export, so the handle is reinterpreted as the
 * `Kld::Device` it names (`MhRenderingDevice_t*` wraps `Kld::Device*`, and every
 * backend's device derives from it single-inherited, making the cast the identity).
 * The Sandbox is built from the same headers with the same toolchain as the
 * backends, so this is the intended path here; a host that wants to stay purely
 * ABI-bound would add a `kld_process_command_queue` export instead.
 */
#include <Kaldi/KaldiLib.h>
#include <Kaldi/KldDevice.hpp>
#include <Elvavena/Util/ElvDynamicLib.hpp>
#include <SDL2/SDL_syswm.h>
#include <memory>
#include <string>
#include <string_view>

class KaldiContext
{
public:
	/** @brief Everything the context needs beyond the window and its size. */
	struct Settings {
		/**
		 * @brief Backend library to load.
		 *
		 * Either an absolute path or a plain soname that the loader resolves
		 * through `LD_LIBRARY_PATH` (`libKaldiVK.so`, `libKaldiGL.so`,
		 * `libKaldiGLES.so`).
		 */
		std::string backendLibrary = "libKaldiVK.so";
		/** @brief Linear RGBA the window is cleared to every frame. */
		float clearColor[4] = { 0.05f, 0.06f, 0.09f, 1.0f };
	};

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
	KaldiContext(const SDL_SysWMinfo& window, int width, int height, const Settings& settings);

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

	/**
	 * @brief Clears the window to @p color and presents it.
	 * @return The device's status; on failure the message is valid until the next call.
	 */
	Kld::HalStatus clear(const float (&color)[4]);

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
};

#endif // KALDICONTEXT_HPP
