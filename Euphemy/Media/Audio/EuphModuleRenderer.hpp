#ifndef EUPHMODULERENDERER_HPP
#define EUPHMODULERENDERER_HPP
/**
 * @file EuphModuleRenderer.hpp
 * @brief Declares the EuphModuleRenderer API in the Euphemy/Media/Audio module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Audio.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <libopenmpt/libopenmpt.h>
namespace Euph {
namespace Media {
namespace Audio {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ModuleRenderer)
/**
 * @brief A class for rendering module music files using the OpenMPT library.
 *
 * This class provides functionality to load and render module files (e.g., MOD, S3M, XM, IT)
 * with various audio configurations and playback controls.
 */
class MH_EUPH_API ModuleRenderer
{
public:
	/// Unique pointer type for managing OpenMPT module instances with custom deleter
	typedef std::unique_ptr<openmpt_module, decltype(&openmpt_module_destroy)> OpenmptModulePtr;

private:
	Elv::Io::uDevice fileDevice;  ///< Device handle for the module file
	OpenmptModulePtr module;      ///< OpenMPT module instance

	// No copy construction or assignment
	ModuleRenderer(const ModuleRenderer& cpy) = delete;
	ModuleRenderer& operator=(const ModuleRenderer& cpy) = delete;

public:
	/**
	 * @brief Move constructor
	 * @param mov The ModuleRenderer to move from
	 */
	ModuleRenderer(ModuleRenderer&& mov);

	/**
	 * @brief Move assignment operator
	 * @param mov The ModuleRenderer to move from
	 * @return Reference to this object
	 */
	ModuleRenderer& operator=(ModuleRenderer&& mov);

	/**
	 * @brief Destructor
	 */
	~ModuleRenderer();

	/**
	 * @brief Constructor that takes a file device
	 * @param fileDev The file device containing the module to load
	 */
	ModuleRenderer(Elv::Io::uDevice&& fileDev);

	// Playback control

	/**
	 * @brief Set the current subsong
	 * @param subsong The subsong index to select
	 */
	void setSubsong(int32_t subsong);

	/**
	 * @brief Get the current subsong
	 * @return The current subsong index
	 */
	int32_t getSubsong() const;

	/**
	 * @brief Set whether playback should loop
	 * @param value True to enable looping, false to disable
	 */
	void setRepeating(bool value);

	/**
	 * @brief Check if playback is set to loop
	 * @return True if looping is enabled, false otherwise
	 */
	bool isRepeating() const;

	/**
	 * @brief Get the duration of the current subsong
	 * @return Duration in seconds
	 */
	double getDuration() const;

	/**
	 * @brief Set the playback position
	 * @param seconds Position in seconds to seek to
	 * @return The actual position set (may differ due to pattern boundaries)
	 */
	double setPosition(double seconds);

	/**
	 * @brief Get the current playback position
	 * @return Current position in seconds
	 */
	double getPosition() const;

	// Audio rendering parameters

	/**
	 * @brief Set the master gain
	 * @param value Gain in millibels
	 */
	void setMasterGain(int32_t value);

	/**
	 * @brief Get the master gain
	 * @return Current gain in millibels
	 */
	int32_t getMasterGain() const;

	/**
	 * @brief Set the stereo separation
	 * @param value Separation percentage (0-100)
	 */
	void setStereoSeparation(int32_t value);

	/**
	 * @brief Get the stereo separation
	 * @return Current separation percentage
	 */
	int32_t getStereoSeparation() const;

	/**
	 * @brief Set the interpolation filter length
	 * @param value Filter length
	 */
	void setInterpolationFilter(int32_t value);

	/**
	 * @brief Get the interpolation filter length
	 * @return Current filter length
	 */
	int32_t getInterpolationFilter() const;

	/**
	 * @brief Set the volume ramping strength
	 * @param value Ramping strength
	 */
	void setVolumeRamping(int32_t value);

	/**
	 * @brief Get the volume ramping strength
	 * @return Current ramping strength
	 */
	int32_t getVolumeRamping() const;

	// Metadata accessors

	/**
	 * @brief Get the module type short name
	 * @return Module type (e.g., "mod", "s3m")
	 */
	const char *getType() const;

	/**
	 * @brief Get the module type long name
	 * @return Full module type name
	 */
	const char *getTypeLong() const;

	/**
	 * @brief Get the original module type short name
	 * @return Original type if the module was converted
	 */
	const char *getOriginalType() const;

	/**
	 * @brief Get the original module type long name
	 * @return Full original type name if the module was converted
	 */
	const char *getOriginalTypeLong() const;

	/**
	 * @brief Get the container short name
	 * @return Container type (e.g., "umx")
	 */
	const char *getContainer() const;

	/**
	 * @brief Get the container long name
	 * @return Full container type name
	 */
	const char *getContainerLong() const;

	/**
	 * @brief Get the tracker name
	 * @return Name of the tracker used to create the module
	 */
	const char *getTracker() const;

	/**
	 * @brief Get the artist name
	 * @return Module artist
	 */
	const char *getArtist() const;

	/**
	 * @brief Get the title
	 * @return Module title
	 */
	const char *getTitle() const;

	/**
	 * @brief Get the creation date
	 * @return Date string
	 */
	const char *getDate() const;

	// Audio rendering methods

	/**
	 * @brief Render mono audio to 16-bit integer buffer
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param mono Destination buffer for mono audio
	 * @return Number of frames actually rendered
	 */
	FrameCount readMono(SampleRate framerate, FrameCount frames, int16_t *mono);

	/**
	 * @brief Render mono audio to 32-bit float buffer
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param mono Destination buffer for mono audio
	 * @return Number of frames actually rendered
	 */
	FrameCount readMono(SampleRate framerate, FrameCount frames, float *mono);

	/**
	 * @brief Render stereo audio to 16-bit integer buffers
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param left Destination buffer for left channel
	 * @param right Destination buffer for right channel
	 * @return Number of frames actually rendered
	 */
	FrameCount readStereo(SampleRate framerate, FrameCount frames, int16_t *left, int16_t *right);

	/**
	 * @brief Render stereo audio to 32-bit float buffers
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param left Destination buffer for left channel
	 * @param right Destination buffer for right channel
	 * @return Number of frames actually rendered
	 */
	FrameCount readStereo(SampleRate framerate, FrameCount frames, float *left, float *right);

	/**
	 * @brief Render quad audio to 16-bit integer buffers
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param left Destination buffer for front left channel
	 * @param right Destination buffer for front right channel
	 * @param rear_left Destination buffer for rear left channel
	 * @param rear_right Destination buffer for rear right channel
	 * @return Number of frames actually rendered
	 */
	FrameCount readQuad(SampleRate framerate, FrameCount frames, int16_t *left,
						int16_t *right, int16_t *rear_left, int16_t *rear_right);

	/**
	 * @brief Render quad audio to 32-bit float buffers
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param left Destination buffer for front left channel
	 * @param right Destination buffer for front right channel
	 * @param rear_left Destination buffer for rear left channel
	 * @param rear_right Destination buffer for rear right channel
	 * @return Number of frames actually rendered
	 */
	FrameCount readQuad(SampleRate framerate, FrameCount frames, float *left,
						float *right, float *rear_left, float *rear_right);

	/**
	 * @brief Render interleaved stereo audio to 16-bit integer buffer
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param interleaved_stereo Destination buffer (LRLRLR...)
	 * @return Number of frames actually rendered
	 */
	FrameCount readInterleavedStereo(SampleRate framerate, FrameCount frames, int16_t *interleaved_stereo);

	/**
	 * @brief Render interleaved stereo audio to 32-bit float buffer
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param interleaved_stereo Destination buffer (LRLRLR...)
	 * @return Number of frames actually rendered
	 */
	FrameCount readInterleavedStereo(SampleRate framerate, FrameCount frames, float *interleaved_stereo);

	/**
	 * @brief Render interleaved quad audio to 16-bit integer buffer
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param interleaved_quad Destination buffer (LRLRLR... for front, then rear channels)
	 * @return Number of frames actually rendered
	 */
	FrameCount readInterleavedQuad(SampleRate framerate, FrameCount frames, int16_t *interleaved_quad);

	/**
	 * @brief Render interleaved quad audio to 32-bit float buffer
	 * @param framerate Sample rate in Hz
	 * @param frames Number of frames to render
	 * @param interleaved_quad Destination buffer (LRLRLR... for front, then rear channels)
	 * @return Number of frames actually rendered
	 */
	FrameCount readInterleavedQuad(SampleRate framerate, FrameCount frames, float *interleaved_quad);
};

}
}
}
#endif // EUPHMODULERENDERER_HPP
