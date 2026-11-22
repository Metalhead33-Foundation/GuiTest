#ifndef KALDID3D11CONTEXT_HPP
#define KALDID3D11CONTEXT_HPP
/**
 * @file D3D11Context.hpp
 * @brief Direct3D 11 equivalent of the GL context layer.
 *
 * On D3D11 there is no separate "context" to make current: the device *is* the
 * context. What the GL backends split into display + drawable + context is a
 * single `D3D11CreateDeviceAndSwapChain` call here, so `D3D11::Context` owns the
 * device, its immediate context, the swap chain and the two default render
 * targets - the minimum needed to clear and present a frame.
 *
 * @note Windows-only and **unverifiable in this repository**: `KaldiD3D11.pro`
 * compiles this file inside a `win32` scope, so it never builds on the Linux CI
 * host. It is written against the documented D3D11/DXGI API.
 */
#include "D3DUtil.hpp"
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <cstdint>

namespace D3D11 {

/** @brief Everything the caller may tune when the device and swap chain are created. */
struct ContextConfig {
	/** @brief Enable the D3D11 debug layer (requires the SDK debug layer). */
	bool debugLayer = false;
	/** @brief Allow the WARP software rasterizer as a fallback. */
	bool allowWarp = false;
	/** @brief Present with vsync (DXGI_PRESENT interval 1) instead of tearing. */
	bool vsync = true;
	/** @brief Back-buffer format. */
	DXGI_FORMAT colorFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	/** @brief Depth/stencil format; `DXGI_FORMAT_UNKNOWN` skips the depth target. */
	DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	/** @brief Feature level requested from the device. */
	D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;
};

/**
 * @brief Owns `ID3D11Device`, `ID3D11DeviceContext`, the swap chain and the
 * default render targets of one window.
 *
 * @note Not copyable and not movable: destruction has to release the COM objects
 * in a fixed order after a final `Present`/`Flush`.
 */
class Context final {
public:
	/**
	 * @brief Creates the device and swap chain for @p window.
	 * @param window Window to present to.
	 * @param config Debug/vsync/format/feature-level tuning.
	 * @throws std::invalid_argument When @p window is null.
	 * @throws std::runtime_error When device or swap-chain creation fails; the
	 *         message carries the failing call and `HRESULT` value.
	 */
	Context(HWND window, const ContextConfig& config = ContextConfig());

	/** @brief Releases the render targets, swap chain, context and device. */
	~Context();

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;
	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;

	/** @brief The device. */
	ID3D11Device* device() const noexcept { return device_; }
	/** @brief The immediate device context. */
	ID3D11DeviceContext* deviceContext() const noexcept { return deviceContext_; }
	/** @brief The swap chain created for the window. */
	IDXGISwapChain1* swapChain() const noexcept { return swapChain_; }
	/** @brief Render target view of the current back buffer (may be null after a resize). */
	ID3D11RenderTargetView* renderTargetView() const noexcept { return renderTargetView_; }
	/** @brief Depth/stencil view, or @c nullptr when the depth target is disabled. */
	ID3D11DepthStencilView* depthStencilView() const noexcept { return depthStencilView_; }
	/** @brief The configuration the context was created with. */
	const ContextConfig& config() const noexcept { return config_; }
	/** @brief Width of the swap chain, in pixels. */
	unsigned width() const noexcept { return width_; }
	/** @brief Height of the swap chain, in pixels. */
	unsigned height() const noexcept { return height_; }
	/** @brief True when the device, context and swap chain exist. */
	bool isValid() const noexcept { return device_ != nullptr && deviceContext_ != nullptr && swapChain_ != nullptr; }

	/** @brief Presents the swap chain; returns false when the frame was not presented. */
	bool present();

	/**
	 * @brief Resizes the swap chain and recreates the default render targets.
	 * @return @c true on success; on failure the render target views stay null.
	 */
	bool resize(unsigned width, unsigned height);

private:
	/** @brief Creates the render target and depth/stencil views for the back buffer. */
	bool createTargets();
	/** @brief Releases the views (used before `ResizeBuffers`). */
	void releaseTargets() noexcept;

	ContextConfig config_;
	HWND window_ = nullptr;
	ID3D11Device* device_ = nullptr;
	ID3D11DeviceContext* deviceContext_ = nullptr;
	IDXGISwapChain1* swapChain_ = nullptr;
	ID3D11RenderTargetView* renderTargetView_ = nullptr;
	ID3D11DepthStencilView* depthStencilView_ = nullptr;
	unsigned width_ = 0;
	unsigned height_ = 0;
};

}

#endif // KALDID3D11CONTEXT_HPP
