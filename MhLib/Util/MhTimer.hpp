#ifndef TIMER_HPP
#define TIMER_HPP
#include <cstdint>
#include <chrono>
#include <functional>
/*#ifndef _POSIX_SEM_VALUE_MAX
#define _POSIX_SEM_VALUE_MAX 32767
#endif*/
#include <thread>
#include <future>

namespace MH33 {
namespace Util {
template <class _Rep, class _Period> std::future<void> TimerAsync(std::chrono::duration<_Rep, _Period> duration, std::function<void()> callback) {
	return std::async(std::launch::async, [duration,callback]() {
		std::this_thread::sleep_for(duration);
		callback();
	} );
}

class Time
{
private:
	uint64_t milliseconds;
public:
	Time(uint64_t milliseconds = 0);
	double asSeconds() const;
	std::chrono::duration<uint64_t, std::milli> asChronoTimeMS() const;
	template <class _Rep, class _Period> std::chrono::duration<_Rep, _Period> chrono_cast_fromMs() const {
		return std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(asChronoTimeMS());
	}
	static Time fromSeconds(double seconds);
	std::chrono::duration<double, std::ratio<1>> asChronoTimeS() const;
	template <class _Rep, class _Period> std::chrono::duration<_Rep, _Period> chrono_cast_fromS() const {
		return std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(asChronoTimeS());
	}
	template <class _Rep, class _Period> static Time fromChronoTime(const std::chrono::duration<_Rep, _Period>& time) {
		return Time( std::chrono::duration_cast<std::chrono::duration<uint64_t, std::milli>>(time).count() );
	}
	Time(const Time& cpy);
	// Assignment operators
	Time& operator=(const Time& cpy);
	Time& operator=(uint64_t ms);
	Time& operator=(double s);
	Time& operator+=(const Time& b);
	Time& operator+=(uint64_t b);
	Time& operator+=(double b);
	Time& operator-=(const Time& b);
	Time& operator-=(uint64_t b);
	Time& operator-=(double b);
	Time& operator*=(uint64_t b);
	Time& operator*=(double b);
	Time& operator/=(uint64_t b);
	Time& operator/=(double b);
	Time& operator%=(const Time& b);
	Time& operator%=(uint64_t b);
	Time& operator%=(double b);
	// Non-assignment operators
	Time operator+(const Time& b) const;
	Time operator+(uint64_t b) const;
	Time operator+(double b) const;
	Time operator-(const Time& b) const;
	Time operator-(uint64_t b) const;
	Time operator-(double b) const;
	Time operator*(uint64_t b) const;
	Time operator*(double b) const;
	Time operator/(uint64_t b) const;
	Time operator/(double b) const;
	Time operator%(const Time& b) const;
	Time operator%(uint64_t b) const;
	Time operator%(double b) const;
	uint64_t getMilliseconds() const;
	void setMilliseconds(uint64_t newMilliseconds);
	std::future<void> timeAciton(std::function<void()> callback) const;
};

class Clock {
private:
	uint64_t timestamp;
public:
	Clock();
	Time getElapsed() const;
	Time restart();
};
}
}

#endif // TIMER_HPP
