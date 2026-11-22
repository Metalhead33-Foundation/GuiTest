/**
 * @file D3D11Util.cpp
 * @brief Implements the Direct3D 11 helpers declared in D3D11Util.hpp.
 */
#include "D3D11Util.hpp"

namespace D3D11 {

InputLayout makeInputLayout(const Kld::VertexDescriptor& descriptor, const char* fallbackSemantic)
{
	InputLayout layout;
	layout.elements_.reserve(descriptor.descriptors.size());
	for (std::size_t index = 0; index < descriptor.descriptors.size(); ++index) {
		const Kld::AttributeDescriptor& attribute = descriptor.descriptors[index];
		if (attribute.SemanticName.empty()) {
			layout.semantics_.emplace_back(fallbackSemantic);
		} else {
			layout.semantics_.emplace_back(attribute.SemanticName);
		}

		D3D11_INPUT_ELEMENT_DESC element{};
		element.SemanticName = layout.semantics_.back().c_str();
		element.SemanticIndex = attribute.SemanticIndex;
		element.Format = D3D::translateToDxgiFormat(attribute.type);
		element.InputSlot = 0;
		element.AlignedByteOffset = static_cast<UINT>(attribute.offset);
		element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		element.InstanceDataStepRate = 0;
		layout.elements_.push_back(element);
	}
	return layout;
}

D3D11_MAP toD3D11Map(Kld::BufferMapAccess access) noexcept
{
	switch (access) {
		case Kld::BufferMapAccess::ReadOnly: return D3D11_MAP_READ;
		case Kld::BufferMapAccess::WriteOnly: return D3D11_MAP_WRITE_DISCARD;
		case Kld::BufferMapAccess::ReadWrite: return D3D11_MAP_READ_WRITE;
	}
	return D3D11_MAP_READ;
}

std::vector<D3D11_SUBRESOURCE_DATA> makeSubresources(const Euph::Media::Image::DecodeTarget& source, TextureUploadInfo& info)
{
	std::vector<D3D11_SUBRESOURCE_DATA> subresources;
	const std::pmr::vector<Euph::Media::Image::Frame>& frames = source.getFrames();
	subresources.reserve(frames.size());

	for (const Euph::Media::Image::Frame& frame : frames) {
		D3D11_SUBRESOURCE_DATA subresource{};
		subresource.pSysMem = frame.data.data();
		subresource.SysMemPitch = frame.stride;
		subresource.SysMemSlicePitch = static_cast<UINT>(frame.data.size());
		subresources.push_back(subresource);
	}

	info.width = frames.empty() ? 0u : frames.front().width;
	info.height = frames.empty() ? 0u : frames.front().height;
	info.depth = static_cast<unsigned>(frames.size());
	info.levels = static_cast<uint8_t>(frames.size() > 255 ? 255 : frames.size());
	return subresources;
}

} // namespace D3D11
