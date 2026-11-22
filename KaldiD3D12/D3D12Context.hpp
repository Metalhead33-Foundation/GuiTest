#ifndef KALDID3D12CONTEXT_HPP
#define KALDID3D12CONTEXT_HPP
/**
 * @file D3D12Context.hpp
 * @brief Direct3D 12 equivalent of the GL context layer.
 *
 * D3D12 needs more scaffolding than D3D11 before a frame can be presented:
 * a factory and an adapter to create the device, a direct command queue to own
 * the swap chain, a descriptor heap holding the back-buffer RTVs, and a fence so
 * a resize (or destruction) can wait until the GPU is done with the old buffers.
 * `D3D12::Context` owns exactly those objects, which is the D3D12 counterpart of
 * "display + drawable + context".
 *
 * Command allocators, command lists and per-frame synchronisation belong to the
 * command-queue implementation and are deliberately not part of the context.
 *
 * @note Windows-only and **unverifiable in this repository** (`win32` scope of
 * `KaldiD3D12.pro`). Unlike the DXGI *format* tables, real D3D12 object creation
 * needs the Windows SDK (`d3d12.h`, `dxgi1_4.h`), not just `microsoft/DirectX-Headers`.
 */
#if __has_include(<directx/d3d12.h>)
#include <directx/d3d12.h>
#elif __has_include(<d3d12.h>)
#include <d3d12.h>
#else
#error "KaldiD3D12 needs <directx/d3d12.h> from microsoft/DirectX-Headers (or <d3d12.h> from the Windows SDK)."
#endif
#include <dxgi1_4.h>
#include <cstdint>
#include <vector>

namespace D3D12 {

/** @brief Everything the caller may tune when the device and swap chain are created. */
struct ContextConfig {
	/** @brief Enable the D3D12 debug layer (requires the Graphics Tools feature). */
	bool debugLayer = false;
	/** @brief Allow the WARP adapter as a fallback when no hardware adapter works. */
	bool allowWarp = false;
	/** @brief Present with vsync (sync interval 1). */
	bool vsync = true;
	/** @brief Number of back buffers; the flip model requires at least 2. */
	uint32_t bufferCount = 2;
	/** @brief Back-buffer format. */
	DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	/** @brief Feature level the device must reach. */
	D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;
};

/**
 * @brief Owns the factory, device, command queue, swap chain, RTV heap and fence.
 *
 * @note Not copyable and not movable: teardown has to wait for the GPU before the
 * swap chain and its resources are released.
 */
class Context final {
public:
	/**
	 * @brief Creates the device chain and the swap chain for @p window.
	 * @param window Window to present to.
	 * @param config Debug/vsync/format/feature-level tuning.
	 * @throws std::invalid_argument When @p window is null or has no client area.
	 * @throws std::runtime_error When any COM object cannot be created; the
	 *         message carries the failing call and `HRESULT`.
	 */
	Context(HWND window, const ContextConfig& config = ContextConfig());

	/** @brief Waits for the GPU, then releases everything in reverse order. */
	~Context();

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;
	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;

	/** @brief The device. */
	ID3D12Device* device() const noexcept { return device_; }
	/** @brief The direct command queue that owns the swap chain. */
	ID3D12CommandQueue* commandQueue() const noexcept { return commandQueue_; }
	/** @brief The swap chain (flip model). */
	IDXGISwapChain3* swapChain() const noexcept { return swapChain_; }
	/** @brief The RTV descriptor heap holding one view per back buffer. */
	ID3D12DescriptorHeap* renderTargetHeap() const noexcept { return renderTargetHeap_; }
	/** @brief The back buffer for @p index (0..bufferCount()-1). */
	ID3D12Resource* backBuffer(uint32_t index) const;
	/** @brief The CPU descriptor handle of the RTV for @p index. */
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView(uint32_t index) const;
	/** @brief Index of the current back buffer. */
	uint32_t currentBackBufferIndex() const;
	/** @brief Number of back buffers. */
	uint32_t bufferCount() const noexcept { return static_cast<uint32_t>(backBuffers_.size()); }
	/** @brief The configuration the context was created with. */
	const ContextConfig& config() const noexcept { return config_; }
	/** @brief Width of the swap chain, in pixels. */
	unsigned width() const noexcept { return width_; }
	/** @brief Height of the swap chain, in pixels. */
	unsigned height() const noexcept { return height_; }
	/** @brief True when device, queue and swap chain exist. */
	bool isValid() const noexcept
	{
		return device_ != nullptr && commandQueue_ != nullptr && swapChain_ != nullptr;
	}

	/** @brief Blocks until every command submitted so far has finished. */
	void waitForGpu();

	/** @brief Presents the swap chain; returns false when the frame was not presented. */
	bool present();

	/**
	 * @brief Resizes the swap chain and recreates the back-buffer views.
	 * @return @c true on success.
	 */
	bool resize(unsigned width, unsigned height);

private:
	/** @brief Creates the per-back-buffer resources and views. */
	bool createBackBuffers();
	/** @brief Releases the back buffers and their views. */
	void releaseBackBuffers() noexcept;

	ContextConfig config_;
	HWND window_ = nullptr;
	IDXGIFactory4* factory_ = nullptr;
	ID3D12Device* device_ = nullptr;
	ID3D12CommandQueue* commandQueue_ = nullptr;
	IDXGISwapChain3* swapChain_ = nullptr;
	ID3D12DescriptorHeap* renderTargetHeap_ = nullptr;
	UINT renderTargetDescriptorSize_ = 0;
	std::vector<ID3D12Resource*> backBuffers_;
	ID3D12Fence* fence_ = nullptr;
	HANDLE fenceEvent_ = nullptr;
	uint64_t fenceValue_ = 0;
	unsigned width_ = 0;
	unsigned height_ = 0;
};

}

#endif // KALDID3D12CONTEXT_HPP
