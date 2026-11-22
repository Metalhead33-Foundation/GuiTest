/**
 * @file VkExecutorUnsupported.cpp
 * @brief Named `UnsupportedBaseline` handlers for the not-yet-ported Vulkan commands.
 *
 * The Vulkan backend is being brought up in stages. Buffers, the default
 * swapchain/frame lifecycle, render-state tracking, clears, render passes and
 * barriers are implemented; the resource/pipeline/transfer halves are still
 * pending. Every missing handler below returns `HalStatusCode::UnsupportedBaseline`
 * with its opcode name, so a portable stream fails with a precise message instead
 * of a silent no-op.
 *
 * When a handler moves into `VkExecutorResources.cpp`, `VkExecutorPipelines.cpp`
 * or `VkExecutorTransfer.cpp`, delete its stub here - the translation unit will
 * otherwise report a duplicate definition.
 */
#include "VkCommandExecutor.hpp"
#include <string>

namespace Vk {

namespace {

/** @brief Standard "not ported yet" status for @p opcode. */
Kld::HalStatus notPorted(const char* opcode)
{
	static thread_local std::string message;
	message = std::string("the Vulkan backend does not implement ") + opcode + " yet";
	return Kld::HalStatus { Kld::HalStatusCode::UnsupportedBaseline, message.c_str() };
}

} // namespace

#define KALDI_VK_UNSUPPORTED(Opcode, Method) \
	Kld::HalStatus CommandExecutor::Method(const Kld::Op##Opcode&) { return notPorted(#Opcode); }

KALDI_VK_UNSUPPORTED(CreateTexture1D, createTexture1D)
KALDI_VK_UNSUPPORTED(CreateTexture2D, createTexture2D)
KALDI_VK_UNSUPPORTED(CreateCompressedTexture2D, createCompressedTexture2D)
KALDI_VK_UNSUPPORTED(CreateTexture2DFromDecodeTarget, createTextureFromDecodeTarget)
KALDI_VK_UNSUPPORTED(CreateTexture3D, createTexture3D)
KALDI_VK_UNSUPPORTED(CreateTextureCube, createTextureCube)
KALDI_VK_UNSUPPORTED(CreateTexture2DArray, createTexture2DArray)
KALDI_VK_UNSUPPORTED(ResizeTexture2D, resizeTexture2D)
KALDI_VK_UNSUPPORTED(UpdateTexture2D, updateTexture2D)
KALDI_VK_UNSUPPORTED(UpdateCompressedTexture2D, updateCompressedTexture2D)
KALDI_VK_UNSUPPORTED(UpdateTextureCubeFace, updateTextureCubeFace)
KALDI_VK_UNSUPPORTED(UpdateTexture2DArray, updateTexture2DArray)
KALDI_VK_UNSUPPORTED(BlitTexture2D, blitTexture2D)
KALDI_VK_UNSUPPORTED(GenerateTextureMipmaps, generateTextureMipmaps)
KALDI_VK_UNSUPPORTED(CreateSampler, createSampler)
KALDI_VK_UNSUPPORTED(CreateImage2D, createImage2D)
KALDI_VK_UNSUPPORTED(ResizeImage2D, resizeImage2D)
KALDI_VK_UNSUPPORTED(CreateMultisampledImage2D, createMultisampledImage2D)
KALDI_VK_UNSUPPORTED(UpdateImage2D, updateImage2D)
KALDI_VK_UNSUPPORTED(BlitImage2D, blitImage2D)
KALDI_VK_UNSUPPORTED(ResolveImage2D, resolveImage2D)
KALDI_VK_UNSUPPORTED(CreateFramebuffer, createFramebuffer)
KALDI_VK_UNSUPPORTED(CreatePipeline, createPipeline)
KALDI_VK_UNSUPPORTED(CreateComputePipeline, createComputePipeline)
KALDI_VK_UNSUPPORTED(BindPipeline, bindPipeline)
KALDI_VK_UNSUPPORTED(BindComputePipeline, bindComputePipeline)
KALDI_VK_UNSUPPORTED(BindTexture, bindTexture)
KALDI_VK_UNSUPPORTED(BindSampler, bindSampler)
KALDI_VK_UNSUPPORTED(BindUniformBuffer, bindUniformBuffer)
KALDI_VK_UNSUPPORTED(BindStorageBuffer, bindStorageBuffer)
KALDI_VK_UNSUPPORTED(BindStorageImage, bindStorageImage)
KALDI_VK_UNSUPPORTED(BindVertexBuffer, bindVertexBuffer)
KALDI_VK_UNSUPPORTED(BindIndexBuffer, bindIndexBuffer)
KALDI_VK_UNSUPPORTED(DrawTriangles, drawTriangles)
KALDI_VK_UNSUPPORTED(DrawIndexedTriangles, drawIndexedTriangles)
KALDI_VK_UNSUPPORTED(DrawLines, drawLines)
KALDI_VK_UNSUPPORTED(DrawIndexedLines, drawIndexedLines)
KALDI_VK_UNSUPPORTED(DrawTrianglesInstanced, drawTrianglesInstanced)
KALDI_VK_UNSUPPORTED(DrawIndexedTrianglesInstanced, drawIndexedTrianglesInstanced)
KALDI_VK_UNSUPPORTED(DrawLinesInstanced, drawLinesInstanced)
KALDI_VK_UNSUPPORTED(DrawIndexedLinesInstanced, drawIndexedLinesInstanced)
KALDI_VK_UNSUPPORTED(DrawIndirect, drawIndirect)
KALDI_VK_UNSUPPORTED(DrawIndexedIndirect, drawIndexedIndirect)
KALDI_VK_UNSUPPORTED(DispatchCompute, dispatchCompute)
KALDI_VK_UNSUPPORTED(DispatchComputeIndirect, dispatchComputeIndirect)
KALDI_VK_UNSUPPORTED(CreateQueryPool, createQueryPool)
KALDI_VK_UNSUPPORTED(ResetQueryPool, resetQueryPool)
KALDI_VK_UNSUPPORTED(BeginQuery, beginQuery)
KALDI_VK_UNSUPPORTED(EndQuery, endQuery)
KALDI_VK_UNSUPPORTED(WriteTimestamp, writeTimestamp)
KALDI_VK_UNSUPPORTED(ReadQueryResults, readQueryResults)
KALDI_VK_UNSUPPORTED(CopyBuffer, copyBuffer)
KALDI_VK_UNSUPPORTED(CopyBufferToImage2D, copyBufferToImage2D)
KALDI_VK_UNSUPPORTED(CopyImage2DToBuffer, copyImage2DToBuffer)
KALDI_VK_UNSUPPORTED(CopyImage2D, copyImage2D)
KALDI_VK_UNSUPPORTED(ReadBuffer, readBuffer)
KALDI_VK_UNSUPPORTED(ReadImage2D, readImage2D)

#undef KALDI_VK_UNSUPPORTED

Kld::HalStatus CommandExecutor::destroyTexture(const Kld::OpDestroy& op)
{
	return failUnsupported("DestroyTexture for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

Kld::HalStatus CommandExecutor::destroySampler(const Kld::OpDestroy& op)
{
	return failUnsupported("DestroySampler for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

Kld::HalStatus CommandExecutor::destroyImage(const Kld::OpDestroyImage& op)
{
	return failUnsupported("DestroyImage for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

Kld::HalStatus CommandExecutor::destroyFramebuffer(const Kld::OpDestroyFramebuffer& op)
{
	return failUnsupported("DestroyFramebuffer for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

Kld::HalStatus CommandExecutor::destroyPipeline(const Kld::OpDestroyPipeline& op)
{
	return failUnsupported("DestroyPipeline for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

Kld::HalStatus CommandExecutor::destroyComputePipeline(const Kld::OpDestroyComputePipeline& op)
{
	return failUnsupported("DestroyComputePipeline for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

Kld::HalStatus CommandExecutor::destroyQueryPool(const Kld::OpDestroyQueryPool& op)
{
	return failUnsupported("DestroyQueryPool for handle " + std::to_string(op.id)
			       + " is not implemented yet");
}

// Helpers declared in the header but only used by the pending handlers; the
// definitions keep the class complete for a later port.
Kld::HalStatus CommandExecutor::uploadImage(VkTextureObject&, const void*, std::uint32_t,
					    std::uint32_t, std::uint32_t, std::uint32_t)
{
	return failUnsupported("texture uploads are not implemented yet");
}

Kld::HalStatus CommandExecutor::generateMips(VkTextureObject&)
{
	return failUnsupported("mip generation is not implemented yet");
}

Kld::HalStatus CommandExecutor::blitImages(const VkTextureObject&, std::uint16_t, std::uint16_t,
					   const VkTextureObject&, std::uint16_t, std::uint16_t,
					   std::uint16_t, std::uint16_t)
{
	return failUnsupported("image blits are not implemented yet");
}

Kld::HalStatus CommandExecutor::createPipelineLayout(VkPipelineObject&,
						     const Kld::PipelineBindingLayout*)
{
	return failUnsupported("pipeline layouts are not implemented yet");
}

Kld::HalStatus CommandExecutor::createGraphicsVariant(VkPipelineObject&, VkRenderPass,
						      const VkPipelineState&, VkPipeline&)
{
	return failUnsupported("graphics pipelines are not implemented yet");
}

Kld::HalStatus CommandExecutor::createComputeObject(VkPipelineObject&)
{
	return failUnsupported("compute pipelines are not implemented yet");
}

Kld::HalStatus CommandExecutor::updateDescriptors(VkPipelineObject&)
{
	return failUnsupported("descriptor sets are not implemented yet");
}

Kld::HalStatus CommandExecutor::prepareDraw(VkPipelineObject*&, bool)
{
	return failUnsupported("draws are not implemented yet");
}

}
