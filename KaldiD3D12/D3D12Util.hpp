#ifndef KALDID3D12UTIL_HPP
#define KALDID3D12UTIL_HPP
/**
 * @file D3D12Util.hpp
 * @brief Direct3D 12 specific glue: input layouts, upload subresources and alignments.
 *
 * The DXGI *format* tables live in `KaldiD3DCommon/D3DUtil.hpp` because D3D11 and
 * D3D12 share them; this header contains only the parts that depend on the D3D12
 * API surface itself (`D3D12_INPUT_ELEMENT_DESC`, `D3D12_SUBRESOURCE_DATA`).
 *
 * @note Windows-only. The consuming `.pro` compiles this translation unit inside
 * a `win32` scope, so Linux/macOS builds of the repository never see it.
 * For WSL usage, `microsoft/DirectX-Headers` expects `<wsl/winadapter.h>` to be
 * included before `<directx/d3d12.h>`; that is a host-tooling concern of whoever
 * builds the backend, not something this header tries to guess.
 */
#if __has_include(<directx/d3d12.h>)
#include <directx/d3d12.h>
#elif __has_include(<d3d12.h>)
#include <d3d12.h>
#else
#error "KaldiD3D12 needs <directx/d3d12.h> from microsoft/DirectX-Headers (or <d3d12.h> from the Windows SDK)."
#endif
#include "D3DUtil.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace D3D12 {

/**
 * @brief Owns the semantic-name storage that backs a `D3D12_INPUT_ELEMENT_DESC` array.
 *
 * See `D3D11::InputLayout` for the ownership rationale; the D3D12 element
 * description needs exactly the same null-terminated semantic names.
 */
class InputLayout {
public:
	/** @brief Creates an empty layout. */
	InputLayout() = default;
	InputLayout(const InputLayout&) = delete;
	InputLayout& operator=(const InputLayout&) = delete;
	InputLayout(InputLayout&&) noexcept = default;
	InputLayout& operator=(InputLayout&&) noexcept = default;

	/** @brief The input elements, ready for a `D3D12_INPUT_LAYOUT_DESC`. */
	std::span<const D3D12_INPUT_ELEMENT_DESC> elements() const noexcept { return elements_; }

private:
	friend InputLayout makeInputLayout(const Kld::VertexDescriptor&, const char*);
	std::vector<std::string> semantics_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements_;
};

/**
 * @brief Builds a D3D12 input layout from a Kaldi vertex descriptor.
 *
 * Attributes are placed in input slot 0 with `D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA`.
 * @see D3D11::makeInputLayout for the semantic-name fallback rules.
 */
InputLayout makeInputLayout(const Kld::VertexDescriptor& descriptor, const char* fallbackSemantic = "TEXCOORD");

/** @brief Geometry bookkeeping produced while building subresource data. */
struct TextureUploadInfo {
	/** @brief Width of mip level 0. */
	unsigned width = 0;
	/** @brief Height of mip level 0. */
	unsigned height = 0;
	/** @brief Depth, array layer or face count. */
	unsigned depth = 0;
	/** @brief Number of `D3D12_SUBRESOURCE_DATA` entries produced. */
	uint8_t levels = 0;
};

/**
 * @brief Builds `D3D12_SUBRESOURCE_DATA` entries from the frames of a decode target.
 *
 * The entries point directly into @p source's frames. Note that D3D12 requires
 * 256-byte aligned row pitches for `UpdateSubresources`, which is the caller's
 * responsibility (`D3D::alignUp(frame.stride, D3D::textureAlignment)`); the
 * helper reports the original stride so the caller can decide how to copy.
 */
std::vector<D3D12_SUBRESOURCE_DATA> makeSubresources(const Euph::Media::Image::DecodeTarget& source, TextureUploadInfo& info);

}

#endif // KALDID3D12UTIL_HPP
