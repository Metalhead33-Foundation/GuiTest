#ifndef MHAUDIOERROR_HPP
#define MHAUDIOERROR_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <exception>
#include <string>
#include <MhLib/Media/AdvancedAudio/MhAudioPacket.hpp>

namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API ChannelCountMismatchError : public std::exception {
private:
	const ChannelCount excepted,received;
	std::string str;
public:
	ChannelCountMismatchError(ChannelCount excepted, ChannelCount received);
	const char* what() const noexcept;
	ChannelCount getExcepted() const;
	ChannelCount getReceived() const;
};
class MH_ADV_AUDIO_API FrameratetMismatchError : public std::exception {
private:
	const FrameRate excepted,received;
	std::string str;
public:
	FrameratetMismatchError(FrameRate excepted, FrameRate received);
	const char* what() const noexcept;
	FrameRate getExcepted() const;
	FrameRate getReceived() const;
};
class MH_ADV_AUDIO_API InterleavingMismatchError : public std::exception {
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
}

#endif // MHAUDIOERROR_HPP
