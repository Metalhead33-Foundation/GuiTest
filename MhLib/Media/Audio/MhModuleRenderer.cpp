#include <MhLib/Media/Audio/MhModuleRenderer.hpp>
#include <libopenmpt/libopenmpt.h>
#include <cassert>

size_t vfio_openmpt_read( void *stream, void *dst, size_t bytes );
int vfio_openmpt_seek( void *stream, int64_t offset, int whence );
int64_t vfio_openmpt_tell( void *stream );

static openmpt_stream_callbacks OpenmptIO = {
	vfio_openmpt_read, vfio_openmpt_seek, vfio_openmpt_tell };
namespace MH33 {
namespace Audio {
#define MODULE_HDNL reinterpret_cast<openmpt_module*>(handle)

ModuleRenderer::ModuleRenderer(ModuleRenderer &&mov): handle(mov.handle), iodev(std::move(mov.iodev))
{

}

openmpt_module_initial_ctl cts[] = {
	{ nullptr, nullptr },
};

ModuleRenderer &ModuleRenderer::operator=(ModuleRenderer &&mov)
{
	if(handle) openmpt_module_destroy(MODULE_HDNL);
	this->handle = mov.handle;
	mov.handle = nullptr;
	this->iodev = std::move(mov.iodev);
	return *this;
}

ModuleRenderer::~ModuleRenderer()
{
	if(handle) openmpt_module_destroy(MODULE_HDNL);
}

ModuleRenderer::ModuleRenderer(const Io::sDevice &iodev) : handle(nullptr), iodev(iodev)
{
	assert(this->iodev);
	if(this->iodev) {
		handle = openmpt_module_create2(OpenmptIO,this->iodev.get(),nullptr,
										nullptr, nullptr, nullptr, nullptr,
										nullptr, cts);
	}
}

ModuleRenderer::ModuleRenderer(Io::sDevice &&iodev) : handle(nullptr), iodev(std::move(iodev))
{
	assert(this->iodev);
	if(this->iodev) {
		handle = openmpt_module_create2(OpenmptIO,this->iodev.get(),nullptr,
										nullptr, nullptr, nullptr, nullptr,
										nullptr, cts);
	}
}

ModuleRenderer::ModuleRenderer(Io::DeviceCreator iodev_creator, Io::Mode mode) : handle(nullptr), iodev(iodev_creator(mode))
{
	assert(this->iodev);
	if(this->iodev) {
		handle = openmpt_module_create2(OpenmptIO,this->iodev.get(),nullptr,
										nullptr, nullptr, nullptr, nullptr,
										nullptr, nullptr);
	}
}

ModuleRenderer::ModuleRenderer(Io::System &iosys, const char *path, Io::Mode mode)
	: handle(nullptr), iodev(iosys.open(path,mode))
{
	assert(this->iodev);
	if(this->iodev) {
		handle = openmpt_module_create2(OpenmptIO,this->iodev.get(),nullptr,
										nullptr, nullptr, nullptr, nullptr,
										nullptr, nullptr);
	}
}

ModuleRenderer::ModuleRenderer(Io::System &iosys, const std::string &path, Io::Mode mode)
	: handle(nullptr), iodev(iosys.open(path,mode))
{
	assert(this->iodev);
	if(this->iodev) {
		handle = openmpt_module_create2(OpenmptIO,this->iodev.get(),nullptr,
										nullptr, nullptr, nullptr, nullptr,
										nullptr, nullptr);
	}
}
// Getters and setters
void ModuleRenderer::setSubsong( int32_t subsong ) {
	assert(handle);
	if(handle) openmpt_module_select_subsong(MODULE_HDNL,subsong);
}
int32_t ModuleRenderer::getSubsong( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_selected_subsong(MODULE_HDNL);
	else return 0;
}
void ModuleRenderer::setRepeating( bool value ) {
	assert(handle);
	if(handle) openmpt_module_set_repeat_count(MODULE_HDNL, value ? -1 : 0);
}
bool ModuleRenderer::isRepeating( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_repeat_count(MODULE_HDNL) != 0;
	else return false;
}
double ModuleRenderer::getDuration( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_duration_seconds(MODULE_HDNL);
	else return 0.0;
}
double ModuleRenderer::setPosition( double seconds ) {
	assert(handle);
	if(handle) return openmpt_module_set_position_seconds(MODULE_HDNL,seconds);
	else return 0.0;
}
double ModuleRenderer::getPosition( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_position_seconds(MODULE_HDNL);
	else return 0.0;
}
void ModuleRenderer::setMasterGain(int32_t	value ) {
	assert(handle);
	if(handle) openmpt_module_set_render_param( MODULE_HDNL, OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL, value);
} // set_render_param(OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL)
int32_t ModuleRenderer::getMasterGain( ) const {
	int32_t val = 0;
	if(handle) openmpt_module_get_render_param(MODULE_HDNL,OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL,&val);
	return val;
} // get_render_param(OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL)
void ModuleRenderer::setStereoSeparation(int32_t value ) {
	assert(handle);
	if(handle) openmpt_module_set_render_param( MODULE_HDNL, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, value);
} // set_render_param(OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT)
int32_t ModuleRenderer::getStereoSeparation( ) const {
	assert(handle);
	int32_t val = 0;
	if(handle) openmpt_module_get_render_param(MODULE_HDNL,OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT,&val);
	return val;
} // get_render_param(OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT)
void ModuleRenderer::setInterpolationFilter(int32_t	value ) {
	assert(handle);
	if(handle) openmpt_module_set_render_param( MODULE_HDNL, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, value);
} // set_render_param(OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH)
int32_t ModuleRenderer::getInterpolationFilter( ) const {
	assert(handle);
	int32_t val = 0;
	if(handle) openmpt_module_get_render_param(MODULE_HDNL,OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH,&val);
	return val;
} // get_render_param(OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH)
void ModuleRenderer::setVolumeRamping(int32_t value ) {
	assert(handle);
	if(handle) openmpt_module_set_render_param( MODULE_HDNL, OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH, value);
} // set_render_param(OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH)
int32_t ModuleRenderer::getVolumeRamping( )	const {
	assert(handle);
	int32_t val = 0;
	if(handle) openmpt_module_get_render_param(MODULE_HDNL,OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH,&val);
	return val;
} // get_render_param(OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH)
// Metadata
const char* ModuleRenderer::getType( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"type");
	else return nullptr;
}         // get_metadata('type')
const char* ModuleRenderer::getTypeLong( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"type_long");
	else return nullptr;
}     // get_metadata('type_long')
const char* ModuleRenderer::getOriginalType( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"originaltype");
	else return nullptr;
} // get_metadata('originaltype')
const char* ModuleRenderer::getOriginalTypeLong( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"originaltype_long");
	else return nullptr;
}      // get_metadata('originaltype_long')
const char* ModuleRenderer::getContainer( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"container");
	else return nullptr;
} // get_metadata('container')
const char* ModuleRenderer::getContainerLong( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"container_long");
	else return nullptr;
} // get_metadata('container_long')
const char* ModuleRenderer::getTracker( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"tracker");
	else return nullptr;
}       // get_metadata('tracker')
const char* ModuleRenderer::getArtist( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"artist");
	else return nullptr;
}        // get_metadata('artist')
const char* ModuleRenderer::getTitle( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"title");
	else return nullptr;
}         // get_metadata('title')
const char* ModuleRenderer::getDate( ) const {
	assert(handle);
	if(handle) return openmpt_module_get_metadata(MODULE_HDNL,"date");
	else return nullptr;
}          // get_metadata('date')
// Rendering
FrameCount ModuleRenderer::readMono(FrameRate framerate, FrameCount frames, int16_t *mono ) {
	assert(handle);
	assert(mono);
	if(handle) return FrameCount(openmpt_module_read_mono(MODULE_HDNL,framerate.var,frames.var,mono));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readMono( FrameRate framerate, FrameCount frames, float *mono ) {
	assert(handle);
	assert(mono);
	if(handle) return FrameCount(openmpt_module_read_float_mono(MODULE_HDNL,framerate.var,frames.var,mono));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readStereo(FrameRate framerate, FrameCount frames, int16_t *left, int16_t *right ) {
	assert(handle);
	assert(left);
	assert(right);
	if(handle) return FrameCount(openmpt_module_read_stereo(MODULE_HDNL,framerate.var,frames.var,left,right));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readStereo( FrameRate framerate, FrameCount frames, float *left, float *right ) {
	assert(handle);
	assert(left);
	assert(right);
	if(handle) return FrameCount(openmpt_module_read_float_stereo(MODULE_HDNL,framerate.var,frames.var,left,right));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readQuad( FrameRate framerate, FrameCount frames, int16_t *left,
								int16_t *right, int16_t *rear_left,	int16_t *rear_right ) {
	assert(handle);
	assert(left);
	assert(right);
	assert(rear_left);
	assert(rear_right);
	if(handle) return FrameCount(openmpt_module_read_quad(MODULE_HDNL,framerate.var,frames.var,left,right,rear_left,rear_right));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readQuad( FrameRate framerate, FrameCount frames, float *left,
								float *right, float *rear_left, float *rear_right ) {
	assert(handle);
	assert(left);
	assert(right);
	assert(rear_left);
	assert(rear_right);
	if(handle) return FrameCount(openmpt_module_read_float_quad(MODULE_HDNL,framerate.var,frames.var,left,right,rear_left,rear_right));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readInterleavedStereo( FrameRate framerate, FrameCount frames, int16_t *interleaved_stereo ) {
	assert(handle);
	assert(interleaved_stereo);
	if(handle) return FrameCount(openmpt_module_read_interleaved_stereo(MODULE_HDNL,framerate.var,frames.var,interleaved_stereo));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readInterleavedStereo( FrameRate framerate, FrameCount frames, float *interleaved_stereo ) {
	assert(handle);
	assert(interleaved_stereo);
	if(handle) return FrameCount(openmpt_module_read_interleaved_float_stereo(MODULE_HDNL,framerate.var,frames.var,interleaved_stereo));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readInterleavedQuad( FrameRate framerate, FrameCount frames, int16_t *interleaved_quad ) {
	assert(handle);
	assert(interleaved_quad);
	if(handle) return FrameCount(openmpt_module_read_interleaved_quad(MODULE_HDNL,framerate.var,frames.var,interleaved_quad));
	else return FrameCount(0);
}
FrameCount ModuleRenderer::readInterleavedQuad( FrameRate framerate, FrameCount frames, float *interleaved_quad ) {
	assert(handle);
	assert(interleaved_quad);
	if(handle) return FrameCount(openmpt_module_read_interleaved_float_quad(MODULE_HDNL,framerate.var,frames.var,interleaved_quad));
	else return FrameCount(0);
}


}
}

size_t vfio_openmpt_read( void *stream, void *dst, size_t bytes ) {
	assert(stream);
	assert(dst);
	return size_t( reinterpret_cast< MH33::Io::Device* >( stream )->read(
		dst, int64_t( bytes ) ) );
}
int vfio_openmpt_seek( void *stream, int64_t offset, int whence ) {
	assert(stream);
	MH33::Io::Device* chandle = reinterpret_cast< MH33::Io::Device* >( stream );
	switch ( whence ) {
	case OPENMPT_STREAM_SEEK_SET:
		chandle->seek( MH33::Io::SeekOrigin::SET, offset ); return chandle->tell() ? 0 : -1;
	case OPENMPT_STREAM_SEEK_CUR:
		chandle->seek( MH33::Io::SeekOrigin::CUR, offset ); return chandle->tell() ? 0 : -1;
	case OPENMPT_STREAM_SEEK_END:
		chandle->seek( MH33::Io::SeekOrigin::END, offset ); return chandle->tell() ? 0 : -1;
	default:
		return -1;
	}
}
int64_t vfio_openmpt_tell( void *stream ) {
	assert(stream);
	return int64_t( reinterpret_cast< MH33::Io::Device* >( stream )->tell( ) );
}
