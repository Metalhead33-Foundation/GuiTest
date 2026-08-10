#ifndef KALDIVULKANDEVICE_HPP
#define KALDIVULKANDEVICE_HPP
/**
 * @file KaldiVulkanDevice.hpp
 * @brief Declares the KaldiVulkanDevice API in the KaldiVK module.
 *
 * This header is part of the public declaration surface for KaldiVK.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace Vk {
/** @brief Vulkan implementation of the Kaldi rendering device contract. */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates an empty Vulkan device wrapper. */
	Device();
};
}
#endif // KALDIVULKANDEVICE_HPP
