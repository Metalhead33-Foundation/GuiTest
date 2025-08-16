#ifndef EUPHMODULERENDERER_HPP
#define EUPHMODULERENDERER_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <libopenmpt/libopenmpt.h>
namespace Euph {
namespace Media {
namespace Audio {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ModuleRenderer)
class MH_EUPH_API ModuleRenderer
{
public:
	typedef std::unique_ptr<openmpt_module, decltype(&openmpt_module_destroy)> OpenmptModulePtr;
private:
	Elv::Io::uDevice fileDevice;
	OpenmptModulePtr module;
	// No copy construction or assignment
	ModuleRenderer(const ModuleRenderer& cpy) = delete;
	ModuleRenderer& operator=(const ModuleRenderer& cpy) = delete;
public:
	// Move construction and assignment
	ModuleRenderer(ModuleRenderer&& mov);
	ModuleRenderer& operator=(ModuleRenderer&& mov);
	// Actual constructor and destructor
	~ModuleRenderer();
	ModuleRenderer(Elv::Io::uDevice&& fileDev);

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
}
#endif // EUPHMODULERENDERER_HPP
