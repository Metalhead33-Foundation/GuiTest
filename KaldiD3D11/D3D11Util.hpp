#ifndef KALDID3D11UTIL_HPP
#define KALDID3D11UTIL_HPP
/**
 * @file D3D11Util.hpp
 * @brief Direct3D 11 specific glue: input layouts, map modes and texture uploads.
 *
 * The DXGI *format* tables live in `KaldiD3DCommon/D3DUtil.hpp` because D3D11 and
 * D3D12 share them; this header contains only the parts that depend on the D3D11
 * API surface itself (`D3D11_INPUT_ELEMENT_DESC`, `D3D11_MAP`,
 * `D3D11_SUBRESOURCE_DATA`).
 *
 * @note Windows-only. The consuming `.pro` compiles this translation unit inside
 * a `win32` scope, so Linux/macOS builds of the repository never see it.
 */
#if __has_include(<d3d11.h>)
#include <d3d11.h>
#else
#error "KaldiD3D11 needs <d3d11.h> from the Windows SDK (or the DXSDK fallback include path)."
#endif
#include "D3DUtil.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace D3D11 {

/**
 * @brief Owns the semantic-name storage that backs a `D3D11_INPUT_ELEMENT_DESC` array.
 *
 * `D3D11_INPUT_ELEMENT_DESC::SemanticName` is a `const char*`, while
 * `Kld::AttributeDescriptor::SemanticName` is a `std::string_view` that is not
 * guaranteed to be null-terminated. The layout therefore keeps owning
 * `std::string` copies and hands out views into them.
 *
 * @warning Copying is disabled on purpose: a copy would leave the copied
 * elements pointing at the source layout's strings. Moving is safe because
 * both vectors are moved without reallocating their elements.
 */
class InputLayout {
public:
	/** @brief Creates an empty layout. */
	InputLayout() = default;
	InputLayout(const InputLayout&) = delete;
	InputLayout& operator=(const InputLayout&) = delete;
	InputLayout(InputLayout&&) noexcept = default;
	InputLayout& operator=(InputLayout&&) noexcept = default;

	/** @brief The input elements, ready for `ID3D11Device::CreateInputLayout`. */
	std::span<const D3D11_INPUT_ELEMENT_DESC> elements() const noexcept { return elements_; }

private:
	friend InputLayout makeInputLayout(const Kld::VertexDescriptor&, const char*);
	std::vector<std::string> semantics_;
	std::vector<D3D11_INPUT_ELEMENT_DESC> elements_;
};

/**
 * @brief Builds a D3D11 input layout from a Kaldi vertex descriptor.
 *
 * Attributes are placed in input slot 0 with `D3D11_INPUT_PER_VERTEX_DATA`.
 * The semantic name comes from `Kld::AttributeDescriptor::SemanticName`; when it
 * is empty (a GL/Vulkan-style location-only descriptor) @p fallbackSemantic is
 * used with the attribute index as semantic index.
 *
 * Every attribute whose `Kld::PrimitiveType` has no DXGI equivalent (three
 * component 8/16-bit formats, 64-bit components, ...) yields
 * `DXGI_FORMAT_UNKNOWN`, which D3D11 rejects at pipeline creation time - check
 * the formats with `D3D::translateToDxgiFormat()` first if you need a nicer error.
 */
InputLayout makeInputLayout(const Kld::VertexDescriptor& descriptor, const char* fallbackSemantic = "TEXCOORD");

/** @brief Translates a Kaldi buffer mapping mode into a `D3D11_MAP`. */
D3D11_MAP toD3D11Map(Kld::BufferMapAccess access) noexcept;

/** @brief Geometry bookkeeping produced while building subresource data. */
struct TextureUploadInfo {
	/** @brief Width of mip level 0. */
	unsigned width = 0;
	/** @brief Height of mip level 0 (1 for 1D textures). */
	unsigned height = 0;
	/** @brief Depth, array layer or face count. */
	unsigned depth = 0;
	/** @brief Number of `D3D11_SUBRESOURCE_DATA` entries produced. */
	uint8_t levels = 0;
};

/**
 * @brief Builds `D3D11_SUBRESOURCE_DATA` entries from the frames of a decode target.
 *
 * The returned entries point directly into the frames of @p source, so they stay
 * valid only as long as @p source does. `SysMemPitch` is taken from each frame's
 * stride, `SysMemSlicePitch` from the frame's total size.
 */
std::vector<D3D11_SUBRESOURCE_DATA> makeSubresources(const Euph::Media::Image::DecodeTarget& source, TextureUploadInfo& info);

}

#endif // KALDID3D11UTIL_HPP
