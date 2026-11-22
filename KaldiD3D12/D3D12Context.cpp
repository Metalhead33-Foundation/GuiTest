/**
 * @file D3D12Context.cpp
 * @brief Implements D3D12Context.hpp.
 *
 * @warning Windows-only, not built by the Linux CI host.
 */
#include "D3D12Context.hpp"
#include <stdexcept>
#include <string>

namespace D3D12 {

namespace {

/** @brief Builds the exception message for a failed call. */
[[noreturn]] void throwHResult(const char* step, HRESULT result)
{
	throw std::runtime_error(std::string("D3D12Context: ") + step + " failed (HRESULT 0x" +
							 std::to_string(static_cast<unsigned long>(result)) + ")");
}

/** @brief Releases a COM pointer and nulls it. */
template <typename T>
void release(T*& object) noexcept
{
	if (object != nullptr) {
		object->Release();
		object = nullptr;
	}
}

} // namespace

Context::Context(HWND window, const ContextConfig& config)
	: config_(config)
	, window_(window)
{
	if (window_ == nullptr) {
		throw std::invalid_argument("D3D12Context: no Win32 window was supplied");
	}

	RECT clientRect{};
	GetClientRect(window_, &clientRect);
	width_ = static_cast<unsigned>(clientRect.right - clientRect.left);
	height_ = static_cast<unsigned>(clientRect.bottom - clientRect.top);
	if (width_ == 0 || height_ == 0) {
		throw std::invalid_argument("D3D12Context: the window has a zero-sized client area");
	}

	if (config_.debugLayer) {
		ID3D12Debug* debugController = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			debugController->Release();
		}
	}

	HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&factory_));
	if (FAILED(result)) {
		throwHResult("CreateDXGIFactory1", result);
	}

	// Pick the first adapter that can create a device of the requested level; a
	// software adapter is only considered when the caller allows WARP.
	for (UINT index = 0; ; ++index) {
		IDXGIAdapter1* adapter = nullptr;
		if (factory_->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) {
			break;
		}
		DXGI_ADAPTER_DESC1 description{};
		adapter->GetDesc1(&description);
		const bool software = (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
		if ((!software || config_.allowWarp) &&
			SUCCEEDED(D3D12CreateDevice(adapter, config_.minimumFeatureLevel, IID_PPV_ARGS(&device_)))) {
			adapter->Release();
			break;
		}
		adapter->Release();
	}
	if (device_ == nullptr) {
		release(factory_);
		throw std::runtime_error("D3D12Context: no adapter could create a D3D12 device");
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	result = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
	if (FAILED(result)) {
		throwHResult("ID3D12Device::CreateCommandQueue", result);
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width_;
	swapChainDesc.Height = height_;
	swapChainDesc.Format = config_.colorFormat;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = config_.bufferCount < 2 ? 2 : config_.bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain1* swapChain1 = nullptr;
	result = factory_->CreateSwapChainForHwnd(commandQueue_, window_, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(result)) {
		throwHResult("IDXGIFactory4::CreateSwapChainForHwnd", result);
	}
	result = swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain_));
	swapChain1->Release();
	if (FAILED(result)) {
		throwHResult("IDXGISwapChain1::QueryInterface(IDXGISwapChain3)", result);
	}
	factory_->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = swapChainDesc.BufferCount;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&renderTargetHeap_));
	if (FAILED(result)) {
		throwHResult("ID3D12Device::CreateDescriptorHeap", result);
	}
	renderTargetDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	result = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (FAILED(result)) {
		throwHResult("ID3D12Device::CreateFence", result);
	}
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent_ == nullptr) {
		throw std::runtime_error("D3D12Context: CreateEvent for the frame fence failed");
	}

	if (!createBackBuffers()) {
		throw std::runtime_error("D3D12Context: the back-buffer render target views could not be created");
	}
}

Context::~Context()
{
	if (device_ != nullptr && commandQueue_ != nullptr && fence_ != nullptr) {
		waitForGpu();
	}
	releaseBackBuffers();
	release(renderTargetHeap_);
	if (fenceEvent_ != nullptr) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
	release(fence_);
	release(swapChain_);
	release(commandQueue_);
	release(device_);
	release(factory_);
}

bool Context::createBackBuffers()
{
	const uint32_t count = config_.bufferCount < 2 ? 2u : config_.bufferCount;
	backBuffers_.assign(count, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = renderTargetHeap_->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t index = 0; index < count; ++index) {
		const HRESULT result = swapChain_->GetBuffer(index, IID_PPV_ARGS(&backBuffers_[index]));
		if (FAILED(result)) {
			releaseBackBuffers();
			return false;
		}
		device_->CreateRenderTargetView(backBuffers_[index], nullptr, handle);
		handle.ptr += renderTargetDescriptorSize_;
	}
	return true;
}

void Context::releaseBackBuffers() noexcept
{
	for (ID3D12Resource*& buffer : backBuffers_) {
		release(buffer);
	}
	backBuffers_.clear();
}

ID3D12Resource* Context::backBuffer(uint32_t index) const
{
	return index < backBuffers_.size() ? backBuffers_[index] : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::renderTargetView(uint32_t index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = renderTargetHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(renderTargetDescriptorSize_) * index;
	return handle;
}

uint32_t Context::currentBackBufferIndex() const
{
	return swapChain_ != nullptr ? swapChain_->GetCurrentBackBufferIndex() : 0;
}

void Context::waitForGpu()
{
	if (commandQueue_ == nullptr || fence_ == nullptr || fenceEvent_ == nullptr) {
		return;
	}
	++fenceValue_;
	if (FAILED(commandQueue_->Signal(fence_, fenceValue_))) {
		return;
	}
	if (fence_->GetCompletedValue() < fenceValue_ &&
		SUCCEEDED(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_))) {
		WaitForSingleObjectEx(fenceEvent_, INFINITE, FALSE);
	}
}

bool Context::present()
{
	if (swapChain_ == nullptr) {
		return false;
	}
	const HRESULT result = swapChain_->Present(config_.vsync ? 1 : 0, 0);
	return SUCCEEDED(result);
}

bool Context::resize(unsigned width, unsigned height)
{
	if (swapChain_ == nullptr || width == 0 || height == 0) {
		return false;
	}
	waitForGpu();
	releaseBackBuffers();
	width_ = width;
	height_ = height;
	const HRESULT result = swapChain_->ResizeBuffers(config_.bufferCount < 2 ? 2 : config_.bufferCount,
													width_, height_, config_.colorFormat,
													DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	if (FAILED(result)) {
		return false;
	}
	return createBackBuffers();
}

}
