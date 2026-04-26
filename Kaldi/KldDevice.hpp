#ifndef KLDDEVICE_HPP
#define KLDDEVICE_HPP
/**
 * @file KldDevice.hpp
 * @brief Backend submission interface for Kaldi graphics command queues.
 *
 * @ref Device is the server-side boundary of the VM-like API. Client code owns
 * command storage; backend implementations translate submitted command spans to
 * their native graphics API and report portable status values.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldHal.hpp>
#include <span>

namespace Kld {

/**
 * @brief Abstract backend device that consumes externally owned command spans.
 *
 * Implementations execute or enqueue command data but do not own the memory
 * referenced by the span. If a backend defers parsing beyond the
 * @ref Device::processCommandQueue call, it must copy the @ref GfxOp payloads
 * and any pointed-to data it needs, or otherwise ensure the documented
 * caller-owned payload lifetime has not ended.
 *
 * Backend implementers should validate command order, handle kinds, resource
 * states, and capability limits, returning @ref HalStatus instead of throwing
 * or exposing API-specific error codes.
 */
class Device {
public:
	/** @brief Virtual destructor for backend implementations. */
	virtual ~Device() = default;
	/** @brief Returns the backend's probed portable capability contract. */
	virtual const HalCapabilities& capabilities() const = 0;
	/**
	 * @brief Processes a queue of graphics commands.
	 * @param commands Externally owned command span valid for the duration of the call.
	 * @return Submission status reported by the backend.
	 *
	 * The span order is execution order. Destroy commands are logical retirement
	 * requests; physical deletion must wait until previously submitted GPU work
	 * that can reference the resource has completed.
	 */
	virtual HalStatus processCommandQueue(std::span<const GfxOp> commands) = 0;
};

}
#endif // KLDDEVICE_HPP
