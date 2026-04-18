#ifndef HRKAUDIOERROR_HPP
#define HRKAUDIOERROR_HPP
#include <exception>
#include <string>
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
namespace Hrk {

class MH_HARUKA_API ChannelCountMismatchError : public std::exception {
private:
	const ChannelCount excepted,received;
	std::string str;
public:
	ChannelCountMismatchError(ChannelCount excepted, ChannelCount received);
	const char* what() const noexcept;
	ChannelCount getExcepted() const;
	ChannelCount getReceived() const;
};
class MH_HARUKA_API SamplerateMismatchError : public std::exception {
private:
	const SampleRate excepted,received;
	std::string str;
public:
	SamplerateMismatchError(SampleRate excepted, SampleRate received);
	const char* what() const noexcept;
	SampleRate getExcepted() const;
	SampleRate getReceived() const;
};
class MH_HARUKA_API InterleavingMismatchError : public std::exception {
private:
	const InterleavingType excepted,received;
	std::string str;
public:
	InterleavingMismatchError(InterleavingType excepted, InterleavingType received);
	const char* what() const noexcept;
	InterleavingType getExcepted() const;
	InterleavingType getReceived() const;
};


}
#endif // HRKAUDIOERROR_HPP
