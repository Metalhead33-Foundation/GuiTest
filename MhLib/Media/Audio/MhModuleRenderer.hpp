#ifndef MHMODULERENDERER_HPP
#define MHMODULERENDERER_HPP
#include <MhLib/Media/Audio/MhAudio.hpp>
#include <MhLib/Io/MhIoDevice.hpp>
#include <MhLib/IoSys/MhIoSystem.hpp>
#include <MhLib/Media/Audio/MhAudioIterator.hpp>
namespace MH33 {
namespace Audio {

class MH_AUDIO_API ModuleRenderer
{
private:
	void* handle;
	Io::sDevice iodev;
	// No copy construction or assignment
	ModuleRenderer(const ModuleRenderer& cpy) = delete;
	ModuleRenderer& operator=(const ModuleRenderer& cpy) = delete;
public:
	// Move construction and assignment
	ModuleRenderer(ModuleRenderer&& mov);
	ModuleRenderer& operator=(ModuleRenderer&& mov);
	// Actual constructor and destructor
	~ModuleRenderer();
	ModuleRenderer(const Io::sDevice& iodev);
	ModuleRenderer(Io::sDevice&& iodev);
	ModuleRenderer(Io::DeviceCreator iodev_creator, Io::Mode mode);
	ModuleRenderer(Io::System& iosys, const char* path, Io::Mode mode);
	ModuleRenderer(Io::System& iosys, const std::string& path, Io::Mode mode);

	// Getters and setters
	void setSubsong( int32_t subsong );
	int32_t getSubsong( ) const;
	void setRepeating( bool value );
	bool isRepeating( ) const;
	double getDuration( ) const;
	double setPosition( double seconds );
	double getPosition( ) const;
	void setMasterGain(int32_t	value ); // set_render_param(OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL)
	int32_t getMasterGain( ) const; // get_render_param(OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL)
	void setStereoSeparation(int32_t value ); // set_render_param(OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT)
	int32_t getStereoSeparation( ) const; // get_render_param(OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT)
	void setInterpolationFilter(int32_t	value ); // set_render_param(OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH)
	int32_t getInterpolationFilter( ) const; // get_render_param(OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH)
	void setVolumeRamping(int32_t value ); // set_render_param(OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH)
	int32_t getVolumeRamping( )	const; // get_render_param(OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH)
	// Metadata
	const char *getType( ) const;         // get_metadata('type')
	const char *getTypeLong( ) const;     // get_metadata('type_long')
	const char *getOriginalType( ) const; // get_metadata('originaltype')
	const char *getOriginalTypeLong( ) const;      // get_metadata('originaltype_long')
	const char *getContainer( ) const; // get_metadata('container')
	const char *getContainerLong( ) const; // get_metadata('container_long')
	const char *getTracker( ) const;       // get_metadata('tracker')
	const char *getArtist( ) const;        // get_metadata('artist')
	const char *getTitle( ) const;         // get_metadata('title')
	const char *getDate( ) const;          // get_metadata('date')
	// Rendering
	FrameCount readMono( FrameRate framerate, FrameCount frames, int16_t *mono );
	FrameCount readMono(FrameRate framerate, FrameCount frames, float *mono );
	FrameCount readStereo( FrameRate framerate, FrameCount frames, int16_t *left, int16_t *right );
	FrameCount readStereo( FrameRate framerate, FrameCount frames, float *left, float *right );
	FrameCount readQuad( FrameRate framerate, FrameCount frames, int16_t *left,
					int16_t *right, int16_t *rear_left,	int16_t *rear_right );
	FrameCount readQuad( FrameRate framerate, FrameCount frames, float *left,
					float *right, float *rear_left, float *rear_right );
	FrameCount readInterleavedStereo( FrameRate framerate, FrameCount frames, int16_t *interleaved_stereo );
	FrameCount readInterleavedStereo( FrameRate framerate, FrameCount frames, float *interleaved_stereo );
	FrameCount readInterleavedQuad( FrameRate framerate, FrameCount frames, int16_t *interleaved_quad );
	FrameCount readInterleavedQuad( FrameRate framerate, FrameCount frames, float *interleaved_quad );
};

}
}
#endif // MHMODULERENDERER_HPP
