#ifndef KALDIGLESDEVICE_H
#define KALDIGLESDEVICE_H
/**
 * @file KaldiGLESDevice.hpp
 * @brief OpenGL ES implementation of the Kaldi rendering device contract.
 *
 * The device owns the EGL context (see `GlContext.hpp` / `EglContext.hpp`) and
 * therefore the window-system half of the backend. `capabilities()` is probed
 * from that context during construction; `processCommandQueue()` is still
 * unimplemented, so `Device` remains abstract until the command-queue
 * implementation lands.
 */
#include <Kaldi/KldDevice.hpp>
#include "GlContext.hpp"
#include "KldImplLib.h"
#include <memory>

namespace GLES {
/**
 * @brief OpenGL ES implementation of the Kaldi rendering device contract.
 *
 * `capabilities()` is implemented by probing the context created in the
 * constructor; `processCommandQueue()` is still unimplemented, so `Device`
 * remains abstract until the command queue lands.
 */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates a detached device without a context. */
	Device();

	/**
	 * @brief Creates a device and an EGL context for @p window.
	 * @param window SDL window-manager info of the window to render into.
	 * @param config Requested GLES version, buffer sizes and MSAA.
	 * @throws std::invalid_argument When @p window has no usable native drawable.
	 * @throws std::runtime_error When context creation or GL loading fails.
	 */
	explicit Device(const SDL_SysWMinfo& window, const ContextConfig& config = defaultContextConfig());

	/** @brief Releases the context and unloads the GL entry points. */
	~Device() override;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;

	/** @brief The capabilities probed from the owned context, or none when detached. */
	const Kld::HalCapabilities& capabilities() const override { return capabilities_; }

	/** @brief The owned context, or @c nullptr for a detached device. */
	Context* context() noexcept { return context_.get(); }
	/** @brief The owned context, or @c nullptr for a detached device. */
	const Context* context() const noexcept { return context_.get(); }
	/** @brief True when the device owns a context. */
	bool hasContext() const noexcept { return context_ != nullptr; }

	/** @brief "EGL", or "none" when the device is detached. */
	const char* apiName() const noexcept;

	/**
	 * @brief Presents the drawable.
	 * @throws std::logic_error When the device has no context.
	 */
	void swapBuffers();

private:
	std::unique_ptr<Context> context_;
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};
}
#endif // KALDIGLESDEVICE_H
