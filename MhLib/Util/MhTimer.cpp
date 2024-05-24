#include "MhTimer.hpp"
#include <SDL2/SDL_timer.h>

namespace MH33 {
namespace Util {
uint64_t Time::getMilliseconds() const
{
	return milliseconds;
}

void Time::setMilliseconds(uint64_t newMilliseconds)
{
	milliseconds = newMilliseconds;
}

std::future<void> Time::timeAciton(std::function<void ()> callback) const
{
	return TimerAsync(asChronoTimeMS(), callback);
}

Time::Time(uint64_t milliseconds)
	: milliseconds(milliseconds)
{

}

double Time::asSeconds() const
{
	return static_cast<double>(milliseconds) * 0.001;
}

std::chrono::duration<uint64_t, std::milli> Time::asChronoTimeMS() const
{
	return std::chrono::duration<uint64_t, std::milli>(milliseconds);
}

Time Time::fromSeconds(double seconds)
{
	return Time(static_cast<uint64_t>(seconds * 1000.0));
}

std::chrono::duration<double, std::ratio<1>> Time::asChronoTimeS() const
{
	return std::chrono::duration<double, std::ratio<1>>( static_cast<uint64_t>(static_cast<double>(milliseconds) * 0.001));
}

Time::Time(const Time& cpy)
	: milliseconds(cpy.milliseconds)
{

}
Time& Time::operator=(const Time& cpy) {
	this->milliseconds = cpy.milliseconds;
	return *this;
}
Time& Time::operator=(uint64_t ms) {
	this->milliseconds = ms;
	return *this;
}
Time& Time::operator=(double s) {
	this->milliseconds = static_cast<uint64_t>(s * 1000.0);
	return *this;
}
Time& Time::operator+=(const Time& b) {
	this->milliseconds += b.milliseconds;
	return *this;
}
Time& Time::operator+=(uint64_t b) {
	this->milliseconds += b;
	return *this;
}
Time& Time::operator+=(double b) {
	this->milliseconds += static_cast<uint64_t>(b * 1000.0);
	return *this;
}
Time& Time::operator-=(const Time& b){
	this->milliseconds -= b.milliseconds;
	return *this;
}
Time& Time::operator-=(uint64_t b) {
	this->milliseconds -= b;
	return *this;
}
Time& Time::operator-=(double b){
	this->milliseconds -= static_cast<uint64_t>(b * 1000.0);
	return *this;
}
Time& Time::operator*=(uint64_t b) {
	this->milliseconds *= b;
	return *this;
}
Time& Time::operator*=(double b) {
	this->milliseconds = static_cast<uint64_t>( static_cast<double>(this->milliseconds) * b );
	return *this;
}
Time& Time::operator/=(uint64_t b) {
	this->milliseconds /= b;
	return *this;
}
Time& Time::operator/=(double b) {
	this->milliseconds = static_cast<uint64_t>( static_cast<double>(this->milliseconds) / b );
	return *this;
}
Time& Time::operator%=(const Time& b) {
	this->milliseconds %= b.milliseconds;
	return *this;
}
Time& Time::operator%=(uint64_t b) {
	this->milliseconds %= b;
	return *this;
}
Time& Time::operator%=(double b) {
	this->milliseconds %= static_cast<uint64_t>(b * 1000.0);
	return *this;
}

Time Time::operator+(const Time& b) const {
	Time tmp(*this);
	tmp += b;
	return tmp;
}
Time Time::operator+(uint64_t b) const{
	Time tmp(*this);
	tmp += b;
	return tmp;
}
Time Time::operator+(double b) const{
	Time tmp(*this);
	tmp += b;
	return tmp;
}
Time Time::operator-(const Time& b) const {
	Time tmp(*this);
	tmp -= b;
	return tmp;
}
Time Time::operator-(uint64_t b) const {
	Time tmp(*this);
	tmp -= b;
	return tmp;
}
Time Time::operator-(double b) const {
	Time tmp(*this);
	tmp -= b;
	return tmp;
}
Time Time::operator*(uint64_t b) const {
	Time tmp(*this);
	tmp *= b;
	return tmp;
}
Time Time::operator*(double b) const {
	Time tmp(*this);
	tmp *= b;
	return tmp;
}
Time Time::operator/(uint64_t b) const {
	Time tmp(*this);
	tmp /= b;
	return tmp;
}
Time Time::operator/(double b) const {
	Time tmp(*this);
	tmp /= b;
	return tmp;
}
Time Time::operator%(const Time& b) const {
	Time tmp(*this);
	tmp %= b;
	return tmp;
}
Time Time::operator%(uint64_t b) const {
	Time tmp(*this);
	tmp %= b;
	return tmp;
}
Time Time::operator%(double b) const {
	Time tmp(*this);
	tmp %= b;
	return tmp;
}

Clock::Clock()
	: timestamp(SDL_GetTicks64())
{

}

Time Clock::getElapsed() const
{
	uint64_t curTs = SDL_GetTicks64();
	return Time(curTs - timestamp);
}

Time Clock::restart()
{
	uint64_t curTs = SDL_GetTicks64();
	uint64_t toReturn = curTs - timestamp;
	timestamp = curTs;
	return Time(toReturn);
}

}
}
