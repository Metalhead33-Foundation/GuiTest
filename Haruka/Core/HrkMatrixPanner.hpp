#ifndef HRKMATRIXPANNER_HPP
#define HRKMATRIXPANNER_HPP
/**
 * @file HrkMatrixPanner.hpp
 * @brief Declares the HrkMatrixPanner API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/Core/HrkProxyPlayable.hpp>
#include <vector>
#include <memory_resource>
#include <span>
namespace Hrk {

/** @brief Applies a caller-editable channel matrix while proxying another playable. */
class MH_HARUKA_API MatrixPanner : public ProxyPlayable
{
private:
	ChannelCount inChannels;
	ChannelCount outChannels;
	InterleavingType interleavingType;
	FrameCount frameCount;
	std::pmr::vector<float> buffer; // Size: (inChannels*outChannels)+(inChannels*frameCount)? Why? Because start contains the matrix. Starting offset of the scratch memory is at (inChannels*outChannels).
public:
	/** @brief Pulls from the proxied playable, applies the matrix, and writes output. */
	FrameCount outputTo(Output& output) override;
	/** @brief Creates a matrix panner with a matrix plus scratch buffer. */
	MatrixPanner(ChannelCount inChannels, ChannelCount outChannels, InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Reinitializes the panner dimensions and scratch storage. */
	void reset(ChannelCount inChannels, ChannelCount outChannels, InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Returns the number of input channels. */
	ChannelCount getInChannels() const;
	/** @brief Returns the number of output channels. */
	ChannelCount getOutChannels() const;
	/** @brief Returns the configured frame block size. */
	FrameCount getFrameCount() const;
	/** @brief Returns the panner interleaving mode. */
	InterleavingType getInterleavingType() const;
	/** @brief Sets the panner interleaving mode. */
	void setInterleavingType(InterleavingType newInterleavingType);
	/** @brief Returns mutable matrix coefficients. */
	std::span<float> getMatrix();
	/** @brief Returns immutable matrix coefficients. */
	std::span<const float> getMatrix() const;
	/** @brief Returns the scratch-buffer offset used for input samples. */
	SampleCount getBufferOffsetForInput() const;
	/** @brief Returns the matrix-buffer offset for input channel x and output channel y. */
	SampleCount getBufferOffsetFor(ChannelCount x, ChannelCount y) const;
	/** @brief Replaces the matrix with an identity mapping where dimensions overlap. */
	void setToIdentity();
};

}
#endif // HRKMATRIXPANNER_HPP
