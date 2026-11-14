#include "HrkAudioError.hpp"
#include <sstream>
namespace Hrk {

static const char* interleaving2str(InterleavingType inttype) {
	switch (inttype) {
		case InterleavingType::DONT_CARE: return "DONT'T CARE";
		case InterleavingType::INTERLEAVED: return "INTERLEAVED";
		case InterleavingType::SEPARATE_CHANNELS: return "SEPARATE CHANNELS";
		default: return "";
	}
}

ChannelCountMismatchError::ChannelCountMismatchError(ChannelCount excepted, ChannelCount received)
	: excepted(excepted), received(received)
{
	std::stringstream str;
	str << "Channel count mismatch!\nExcepted channel number: " << excepted.var << "\nReceived channel count: " << received.var;
	this->str = str.str();
}

const char* ChannelCountMismatchError::what() const noexcept
{
	return str.c_str();
}

ChannelCount ChannelCountMismatchError::getExcepted() const
{
	return excepted;
}

ChannelCount ChannelCountMismatchError::getReceived() const
{
	return received;
}

SamplerateMismatchError::SamplerateMismatchError(SampleRate excepted, SampleRate received)
	: excepted(excepted), received(received)
{
	std::stringstream str;
	str << "Sameplrate mismatch!\nExcepted samplerate: " << excepted.var << "\nReceived samplerate: " << received.var;
	this->str = str.str();
}

const char* SamplerateMismatchError::what() const noexcept
{
	return str.c_str();
}

SampleRate SamplerateMismatchError::getExcepted() const
{
	return excepted;
}

SampleRate SamplerateMismatchError::getReceived() const
{
	return received;
}

InterleavingMismatchError::InterleavingMismatchError(InterleavingType excepted, InterleavingType received)
	: excepted(excepted), received(received)
{
	std::stringstream str;
	str << "Interleaving type mismatch!\nExcepted interleaving type: " << interleaving2str(excepted) << "\nReceived interleaving type: " << interleaving2str(received);
	this->str = str.str();
}

const char* InterleavingMismatchError::what() const noexcept
{
	return str.c_str();
}

InterleavingType InterleavingMismatchError::getExcepted() const
{
	return excepted;
}

InterleavingType InterleavingMismatchError::getReceived() const
{
	return received;
}

}
