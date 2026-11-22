#include "KaldiContext.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <array>
#include <stdexcept>

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

KaldiContext::KaldiContext(const SDL_SysWMinfo& window, int width, int height,
			   const Settings& settings)
{
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
	// `MhRenderingDevice_t*` wraps the backend's `Kld::Device*`; see the header.
	device_ = reinterpret_cast<Kld::Device*>(handle_);
	capabilities_ = device_->capabilities();
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

Kld::HalStatus KaldiContext::clear(const float (&color)[4])
{
	if (device_ == nullptr) {
		return Kld::HalStatus { Kld::HalStatusCode::InitializationFailed,
					"KaldiContext: the device was not created" };
	}
	const Kld::OpClearColorAttachment clearCommand {
		Kld::DefaultFramebuffer, 0, { color[0], color[1], color[2], color[3] }
	};
	// The default frame path is the whole point of this context: begin, clear, end,
	// present - four commands, no resources, nothing to retire.
	const std::array<Kld::GfxOp, 4> ops = { {
		{ Kld::Opcode::BeginFrame, { .opBeginFrame = { Kld::DefaultFramebuffer } } },
		{ Kld::Opcode::ClearColorAttachment, { .opClearColorAttachment = clearCommand } },
		{ Kld::Opcode::EndFrame, { .opEndFrame = { Kld::DefaultFramebuffer } } },
		{ Kld::Opcode::Present, { .opPresent = { Kld::DefaultFramebuffer } } }
	} };
	return device_->processCommandQueue(ops);
}
