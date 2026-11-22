#include "LteKaldiContext.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <stdexcept>
#include <string>

namespace {

/**
 * @brief Links @p name in @p library, or throws with the library that lacked it.
 *
 * `DynamicLibrary::link()` returns null for every symbol when the library itself
 * failed to load, so the same check covers "wrong path" and "wrong backend".
 */
template <typename T>
T requireSymbol(const Elv::Util::DynamicLibrary& library, const char* name,
		const std::string& path)
{
	// `sym` is non-const because dlsym/GetProcAddress hand out mutable addresses.
	auto* mutableLibrary = const_cast<Elv::Util::DynamicLibrary*>(&library);
	const T symbol = mutableLibrary->sym<T>(name);
	if (symbol == nullptr) {
		throw std::runtime_error("KaldiContext: '" + path + "' does not export " + name);
	}
	return symbol;
}

} // namespace

namespace Lotte {
namespace Gfx {

KaldiContext::KaldiContext(const SDL_SysWMinfo& window, std::uint16_t width,
			   std::uint16_t height, const KaldiContextSettings& settings)
	: commands_(), resources_(commands_)
{
	clearColor_[0] = settings.clearColor[0];
	clearColor_[1] = settings.clearColor[1];
	clearColor_[2] = settings.clearColor[2];
	clearColor_[3] = settings.clearColor[3];

	library_ = std::make_unique<Elv::Util::DynamicLibrary>(settings.backendLibrary);
	const MhCreateRenderingDevice createDevice =
		library_->sym<MhCreateRenderingDevice>("kld_create_rendering_device");
	if (createDevice == nullptr) {
		throw std::runtime_error("KaldiContext: could not load the backend library '" +
					 settings.backendLibrary + "'");
	}
	destroyDevice_ = requireSymbol<MhDestroyRenderingDevice>(*library_,
						"kld_destroy_rendering_device",
						settings.backendLibrary);
	getLastError_ = requireSymbol<MhGetLastError>(*library_, "kld_get_last_error_message",
						      settings.backendLibrary);
	if (const MhGetRendererName getName = library_->sym<MhGetRendererName>("kld_get_renderer_name")) {
		if (const char* name = getName()) {
			rendererName_ = name;
		}
	}
	if (const MhGetVersion getVersion = library_->sym<MhGetVersion>("kld_get_version")) {
		if (const char* version = getVersion()) {
			backendVersion_ = version;
		}
	}
	const MhRenderingDeviceCreationInfo creation { width, height };
	handle_ = createDevice(&window, &creation);
	if (handle_ == nullptr) {
		std::string message = "KaldiContext: '" + settings.backendLibrary +
			"' could not create a rendering device";
		const char* backendMessage = getLastError_ != nullptr ? getLastError_() : nullptr;
		if (backendMessage != nullptr) {
			message += ": ";
			message += backendMessage;
		}
		throw std::runtime_error(message);
	}
	// `MhRenderingDevice_t*` wraps the backend's `Kld::Device*`; every backend's
	// device derives from it single-inherited, so the cast is the identity.
	device_ = reinterpret_cast<Kld::Device*>(handle_);
	capabilities_ = device_->capabilities();
	// Creation commands recorded before the first frame still have to reach the
	// backend, so the factory needs the device immediately.
	resources_.setDevice(device_);
}

KaldiContext::~KaldiContext()
{
	// The device (and its virtual table) lives inside the library, so it has to go
	// first; `library_` unloads afterwards, when the member is destroyed.
	if (destroyDevice_ != nullptr && handle_ != nullptr) {
		destroyDevice_(handle_);
	}
	handle_ = nullptr;
	device_ = nullptr;
}

std::string KaldiContext::halVersionName() const
{
	const Kld::HalVersion version = capabilities_.version;
	return std::to_string(version.major) + "." + std::to_string(version.minor) + "." +
		std::to_string(version.patch);
}

void KaldiContext::setClearColor(const float (&color)[4]) noexcept
{
	clearColor_[0] = color[0];
	clearColor_[1] = color[1];
	clearColor_[2] = color[2];
	clearColor_[3] = color[3];
}

Kld::HalStatus KaldiContext::beginFrame()
{
	if (device_ == nullptr) {
		return { Kld::HalStatusCode::InitializationFailed,
			 "KaldiContext: the device was not created" };
	}
	KldOopDetail::beginFrame(commands_);
	const Kld::OpClearColorAttachment clearCommand {
		Kld::DefaultFramebuffer, 0,
		{ clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3] }
	};
	commands_.push({ Kld::Opcode::ClearColorAttachment,
			 { .opClearColorAttachment = clearCommand } });
	return Kld::HalOk;
}

Kld::HalStatus KaldiContext::endFrame()
{
	if (device_ == nullptr) {
		return { Kld::HalStatusCode::InitializationFailed,
			 "KaldiContext: the device was not created" };
	}
	KldOopDetail::endFrame(commands_);
	KldOopDetail::present(commands_);
	return resources_.submit();
}

Kld::HalStatus KaldiContext::submit()
{
	if (device_ == nullptr) {
		return { Kld::HalStatusCode::InitializationFailed,
			 "KaldiContext: the device was not created" };
	}
	return resources_.submit();
}

} // namespace Gfx
} // namespace Lotte
