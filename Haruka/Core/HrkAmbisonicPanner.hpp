#ifndef HRKAMBISONICPANNER_HPP
#define HRKAMBISONICPANNER_HPP
#include <Haruka/Core/HrkProxyPlayable.hpp>
#include <vector>
#include <memory_resource>
#include <span>
namespace Hrk {

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
	AmbisonicPanner(InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	void resetBuffer(FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	void updatePosition(float theta, float phi, float distInv);
	float getTheta() const;
	float getPhi() const;
	float getLF() const;
	float getLB() const;
	float getRF() const;
	float getRB() const;
	float getDistInv() const;
	FrameCount outputTo(Output& output) override;
	InterleavingType getInterleavingType() const;
	void setInterleavingType(InterleavingType newInterleavingType);
};

}
#endif // HRKAMBISONICPANNER_HPP
