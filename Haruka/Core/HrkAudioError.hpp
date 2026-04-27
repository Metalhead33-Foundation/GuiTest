#ifndef HRKAUDIOERROR_HPP
#define HRKAUDIOERROR_HPP
/**
 * @file HrkAudioError.hpp
 * @brief Declares the HrkAudioError API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <exception>
#include <string>
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
namespace Hrk {

/** @brief Exception raised when an operation receives an unexpected channel count. */
class MH_HARUKA_API ChannelCountMismatchError : public std::exception {
private:
	const ChannelCount excepted,received;
	std::string str;
public:
	/** @brief Builds an error from the expected and received channel counts. */
	ChannelCountMismatchError(ChannelCount excepted, ChannelCount received);
	/** @brief Returns the formatted mismatch message. */
	const char* what() const noexcept;
	/** @brief Returns the expected channel count. */
	ChannelCount getExcepted() const;
	/** @brief Returns the received channel count. */
	ChannelCount getReceived() const;
};
/** @brief Exception raised when an operation receives an unexpected sample rate. */
class MH_HARUKA_API SamplerateMismatchError : public std::exception {
private:
	const SampleRate excepted,received;
	std::string str;
public:
	/** @brief Builds an error from the expected and received sample rates. */
	SamplerateMismatchError(SampleRate excepted, SampleRate received);
	/** @brief Returns the formatted mismatch message. */
	const char* what() const noexcept;
	/** @brief Returns the expected sample rate. */
	SampleRate getExcepted() const;
	/** @brief Returns the received sample rate. */
	SampleRate getReceived() const;
};
/** @brief Exception raised when interleaved/separate-channel layout does not match. */
class MH_HARUKA_API InterleavingMismatchError : public std::exception {
private:
	const InterleavingType excepted,received;
	std::string str;
public:
	/** @brief Builds an error from the expected and received interleaving modes. */
	InterleavingMismatchError(InterleavingType excepted, InterleavingType received);
	/** @brief Returns the formatted mismatch message. */
	const char* what() const noexcept;
	/** @brief Returns the expected interleaving mode. */
	InterleavingType getExcepted() const;
	/** @brief Returns the received interleaving mode. */
	InterleavingType getReceived() const;
};


}
#endif // HRKAUDIOERROR_HPP
