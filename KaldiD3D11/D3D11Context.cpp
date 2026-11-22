/**
 * @file D3D11Context.cpp
 * @brief Implements D3D11Context.hpp.
 *
 * @warning Windows-only, not built by the Linux CI host.
 */
#include "D3D11Context.hpp"
#include <stdexcept>
#include <string>

namespace D3D11 {

namespace {

/** @brief Builds the exception message for a failed call. */
[[noreturn]] void throwHResult(const char* step, HRESULT result)
{
	throw std::runtime_error(std::string("D3D11Context: ") + step + " failed (HRESULT 0x" +
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
		throw std::invalid_argument("D3D11Context: no Win32 window was supplied");
	}

	RECT clientRect{};
	GetClientRect(window_, &clientRect);
	width_ = static_cast<unsigned>(clientRect.right - clientRect.left);
	height_ = static_cast<unsigned>(clientRect.bottom - clientRect.top);
	if (width_ == 0 || height_ == 0) {
		throw std::invalid_argument("D3D11Context: the window has a zero-sized client area");
	}

	UINT flags = 0;
	if (config_.debugLayer) {
		flags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = width_;
	swapChainDesc.BufferDesc.Height = height_;
	swapChainDesc.BufferDesc.Format = config_.colorFormat;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = window_;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	const D3D_FEATURE_LEVEL featureLevels[] = { config_.minimumFeatureLevel };
	D3D_FEATURE_LEVEL achievedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	IDXGISwapChain* swapChain = nullptr;
	HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr, config_.allowWarp ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE,
		nullptr, flags, featureLevels, 1, D3D11_SDK_VERSION,
		&swapChainDesc, &swapChain, &device_, &achievedFeatureLevel, &deviceContext_);
	if (FAILED(result)) {
		throwHResult("D3D11CreateDeviceAndSwapChain", result);
	}

	// The device always hands out a DXGI 1.x swap chain for the descriptor above.
	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&swapChain_));
	swapChain->Release();
	if (FAILED(result)) {
		release(deviceContext_);
		release(device_);
		throwHResult("IDXGISwapChain::QueryInterface(IDXGISwapChain1)", result);
	}

	if (!createTargets()) {
		release(swapChain_);
		release(deviceContext_);
		release(device_);
		throw std::runtime_error("D3D11Context: the default render targets could not be created");
	}
}

Context::~Context()
{
	releaseTargets();
	release(swapChain_);
	release(deviceContext_);
	release(device_);
}

void Context::releaseTargets() noexcept
{
	release(depthStencilView_);
	release(renderTargetView_);
}

bool Context::createTargets()
{
	ID3D11Texture2D* backBuffer = nullptr;
	HRESULT result = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	if (FAILED(result) || backBuffer == nullptr) {
		return false;
	}
	result = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
	backBuffer->Release();
	if (FAILED(result)) {
		release(renderTargetView_);
		return false;
	}

	if (config_.depthFormat == DXGI_FORMAT_UNKNOWN) {
		return true;
	}

	D3D11_TEXTURE2D_DESC depthDesc{};
	depthDesc.Width = width_;
	depthDesc.Height = height_;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = config_.depthFormat;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* depthTexture = nullptr;
	result = device_->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
	if (FAILED(result) || depthTexture == nullptr) {
		release(renderTargetView_);
		return false;
	}
	result = device_->CreateDepthStencilView(depthTexture, nullptr, &depthStencilView_);
	depthTexture->Release();
	if (FAILED(result)) {
		releaseTargets();
		return false;
	}
	return true;
}

bool Context::present()
{
	const HRESULT result = swapChain_->Present(config_.vsync ? 1 : 0, 0);
	return SUCCEEDED(result);
}

bool Context::resize(unsigned width, unsigned height)
{
	if (swapChain_ == nullptr || width == 0 || height == 0) {
		return false;
	}
	// A swap chain cannot be resized while views reference its buffers.
	deviceContext_->OMSetRenderTargets(0, nullptr, nullptr);
	releaseTargets();

	width_ = width;
	height_ = height;
	const HRESULT result = swapChain_->ResizeBuffers(0, width_, height_, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result)) {
		return false;
	}
	return createTargets();
}

}
