#ifndef HRKAMBISONICPANNER_HPP
#define HRKAMBISONICPANNER_HPP
/**
 * @file HrkAmbisonicPanner.hpp
 * @brief Declares the HrkAmbisonicPanner API in the Haruka/Core module.
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

/** @brief B-format style panner that derives four speaker gains from polar position. */
class MH_HARUKA_API AmbisonicPanner : public ProxyPlayable
{
private:
	InterleavingType interleavingType;
	std::pmr::vector<float> buffer;
	float theta; // horizontal angle - in radians
	float phi;   // vertical angle - in radians
	float distInv; // Inverse of distance. Implied to be the reciprocal of a number derived from actual distance and an attenuation factor.
	float LF, LB, RF, RB;
public:
	/** @brief Creates a panner with the requested interleaving and scratch frame capacity. */
	AmbisonicPanner(InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Reallocates the internal scratch buffer for a new frame count. */
	void resetBuffer(FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Updates source position and derived speaker gains. */
	void updatePosition(float theta, float phi, float distInv);
	/** @brief Returns the horizontal source angle in radians. */
	float getTheta() const;
	/** @brief Returns the vertical source angle in radians. */
	float getPhi() const;
	/** @brief Returns the left-front gain. */
	float getLF() const;
	/** @brief Returns the left-back gain. */
	float getLB() const;
	/** @brief Returns the right-front gain. */
	float getRF() const;
	/** @brief Returns the right-back gain. */
	float getRB() const;
	/** @brief Returns the inverse distance attenuation factor. */
	float getDistInv() const;
	/** @brief Pulls from the proxied playable, applies panning gains, and writes output. */
	FrameCount outputTo(Output& output) override;
	/** @brief Returns the panner interleaving mode. */
	InterleavingType getInterleavingType() const;
	/** @brief Sets the panner interleaving mode. */
	void setInterleavingType(InterleavingType newInterleavingType);
};

}
#endif // HRKAMBISONICPANNER_HPP
