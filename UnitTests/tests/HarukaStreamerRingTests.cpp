/**
 * @file HarukaStreamerRingTests.cpp
 * @brief Exercises the extracted contiguous ring buffer through Haruka's streamer.
 *
 * A deterministic `RingBufferedStreamer` subclass decodes a known sample
 * sequence. A four-frame ring forces repeated wrap-around while the test checks
 * both interleaved and separate-channel output.
 */
#include <catch2/catch_test_macros.hpp>
#include <Haruka/IO/HrkRingBufferedStreamer.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using Hrk::ChannelCount;
using Hrk::FrameCount;
using Hrk::FrameIndex;
using Hrk::InterleavingType;
using Hrk::Output;
using Hrk::PlayStatus;
using Hrk::SampleRate;

/** @brief Streamer that emits frame-major, channel-minor counting samples. */
class CountingStreamer final : public Hrk::RingBufferedStreamer {
public:
	explicit CountingStreamer(FrameCount capacity)
		: Hrk::RingBufferedStreamer(capacity) {
		getState().setPlayStatus(PlayStatus::PLAYING);
	}

protected:
	void resolveOutputFormat(Output& output, ChannelCount& channels,
		SampleRate& sampleRate) override {
		channels = ChannelCount(2);
		sampleRate = SampleRate(48000);
		output.samplerate = sampleRate;
	}

	FrameCount decodeFrames(float* destination, FrameCount frames, ChannelCount channels,
		SampleRate) override {
		for (std::uintptr_t frame = 0; frame < frames.var; ++frame) {
			for (std::uintptr_t channel = 0; channel < channels.var; ++channel) {
				destination[frame * channels.var + channel] =
					static_cast<float>((nextFrame + frame) * 10 + channel);
			}
		}
		nextFrame += frames.var;
		return frames;
	}

	bool seekDecoder(FrameIndex cursor, ChannelCount, SampleRate) override {
		nextFrame = cursor.var;
		return true;
	}

	bool rewindDecoder(ChannelCount, SampleRate) override {
		nextFrame = 0;
		return true;
	}

private:
	std::uintptr_t nextFrame = 0;
};

} // namespace

TEST_CASE("Haruka streamer wraps through the extracted ring buffer", "[haruka][ringbuffer]")
{
	CountingStreamer streamer(FrameCount(4));
	std::vector<float> destination(20, -1.0f);
	Output output;
	output.dst = destination.data();
	output.frameCount = FrameCount(10);
	output.channels = ChannelCount(2);
	output.interleaving = InterleavingType::INTERLEAVED;
	output.samplerate = SampleRate(0);

	CHECK(streamer.outputTo(output) == FrameCount(10));
	for (std::uintptr_t frame = 0; frame < 10; ++frame) {
		CHECK(destination[frame * 2] == static_cast<float>(frame * 10));
		CHECK(destination[frame * 2 + 1] == static_cast<float>(frame * 10 + 1));
	}
}

TEST_CASE("Haruka streamer writes separate channel planes", "[haruka][ringbuffer]")
{
	CountingStreamer streamer(FrameCount(3));
	std::vector<float> destination(12, -1.0f);
	Output output;
	output.dst = destination.data();
	output.frameCount = FrameCount(6);
	output.channels = ChannelCount(2);
	output.interleaving = InterleavingType::SEPARATE_CHANNELS;
	output.samplerate = SampleRate(0);

	CHECK(streamer.outputTo(output) == FrameCount(6));
	for (std::uintptr_t frame = 0; frame < 6; ++frame) {
		CHECK(destination[frame] == static_cast<float>(frame * 10));
		CHECK(destination[6 + frame] == static_cast<float>(frame * 10 + 1));
	}
}
