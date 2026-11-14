#include "EuphModuleRenderer.hpp"

size_t vfio_openmpt_read( void *stream, void *dst, size_t bytes ) {
	return size_t( reinterpret_cast< Elv::Io::Device* >( stream )->read(
		dst, 1, int64_t( bytes ) ) );
}
int vfio_openmpt_seek( void *stream, int64_t offset, int whence ) {
	Elv::Io::Device* chandle = reinterpret_cast< Elv::Io::Device* >( stream );
	switch ( whence ) {
		case OPENMPT_STREAM_SEEK_SET:
			chandle->seek( offset, Elv::Io::SeekOrigin::SET ); return chandle->tell() ? 0 : -1;
		case OPENMPT_STREAM_SEEK_CUR:
			chandle->seek( offset, Elv::Io::SeekOrigin::CUR ); return chandle->tell() ? 0 : -1;
		case OPENMPT_STREAM_SEEK_END:
			chandle->seek( offset, Elv::Io::SeekOrigin::END ); return chandle->tell() ? 0 : -1;
		default:
			return -1;
	}
}
int64_t vfio_openmpt_tell( void *stream ) {
	return int64_t( reinterpret_cast< Elv::Io::Device* >( stream )->tell( ) );
}
openmpt_module_initial_ctl cts[] = {
	{ nullptr, nullptr },
	};

static openmpt_stream_callbacks OpenmptIO = {
	vfio_openmpt_read, vfio_openmpt_seek, vfio_openmpt_tell };

namespace Euph {
namespace Media {
namespace Audio {

ModuleRenderer::ModuleRenderer(ModuleRenderer&& mov)
	: fileDevice(std::move(mov.fileDevice)), module(std::move(mov.module))
{

}

ModuleRenderer& ModuleRenderer::operator=(ModuleRenderer&& mov)
{
	this->module = std::move(mov.module);
	this->fileDevice = std::move(mov.fileDevice);
	return *this;
}

ModuleRenderer::~ModuleRenderer()
{

}

ModuleRenderer::ModuleRenderer(Elv::Io::uDevice&& fileDev)
	: fileDevice(std::move(fileDev)), module(nullptr, openmpt_module_destroy)
{
	if(this->fileDevice) {
		module = OpenmptModulePtr(openmpt_module_create2(OpenmptIO,this->fileDevice.get(),nullptr,
										nullptr, nullptr, nullptr, nullptr,
										nullptr, cts),openmpt_module_destroy);
	}
}

void ModuleRenderer::setSubsong(int32_t subsong)
{
	openmpt_module_select_subsong(module.get(),subsong);
}

int32_t ModuleRenderer::getSubsong() const
{
	return openmpt_module_get_selected_subsong(module.get());
}

void ModuleRenderer::setRepeating(bool value)
{
	openmpt_module_set_repeat_count(module.get(), value ? -1 : 0);
}

bool ModuleRenderer::isRepeating() const
{
	return openmpt_module_get_repeat_count(module.get()) != 0;
}

double ModuleRenderer::getDuration() const
{
	return openmpt_module_get_duration_seconds(module.get());
}

double ModuleRenderer::setPosition(double seconds)
{
	return openmpt_module_set_position_seconds(module.get(),seconds);
}

double ModuleRenderer::getPosition() const
{
	return openmpt_module_get_position_seconds(module.get());
}

void ModuleRenderer::setMasterGain(int32_t value)
{
	openmpt_module_set_render_param( module.get(), OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL, value);
}

int32_t ModuleRenderer::getMasterGain() const
{
	int32_t val = 0;
	openmpt_module_get_render_param(module.get(),OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL,&val);
	return val;
}

void ModuleRenderer::setStereoSeparation(int32_t value)
{
	openmpt_module_set_render_param( module.get(), OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, value);
}

int32_t ModuleRenderer::getStereoSeparation() const
{
	int32_t val = 0;
	openmpt_module_get_render_param(module.get(),OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT,&val);
	return val;
}

void ModuleRenderer::setInterpolationFilter(int32_t value)
{
	openmpt_module_set_render_param( module.get(), OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, value);
}

int32_t ModuleRenderer::getInterpolationFilter() const
{
	int32_t val = 0;
	openmpt_module_get_render_param(module.get(),OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH,&val);
	return val;
}

void ModuleRenderer::setVolumeRamping(int32_t value)
{
	openmpt_module_set_render_param( module.get(), OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH, value);
}

int32_t ModuleRenderer::getVolumeRamping() const
{
	int32_t val = 0;
	openmpt_module_get_render_param(module.get(),OPENMPT_MODULE_RENDER_VOLUMERAMPING_STRENGTH,&val);
	return val;
}

const char* ModuleRenderer::getType() const
{
	return openmpt_module_get_metadata(module.get(),"type");
}

const char* ModuleRenderer::getTypeLong() const
{
	return openmpt_module_get_metadata(module.get(),"type_long");
}

const char* ModuleRenderer::getOriginalType() const
{
	return openmpt_module_get_metadata(module.get(),"originaltype");
}

const char* ModuleRenderer::getOriginalTypeLong() const
{
	return openmpt_module_get_metadata(module.get(),"originaltype_long");
}

const char* ModuleRenderer::getContainer() const
{
	return openmpt_module_get_metadata(module.get(),"container");
}

const char* ModuleRenderer::getContainerLong() const
{
	return openmpt_module_get_metadata(module.get(),"container_long");
}

const char* ModuleRenderer::getTracker() const
{
	return openmpt_module_get_metadata(module.get(),"tracker");
}

const char* ModuleRenderer::getArtist() const
{
	return openmpt_module_get_metadata(module.get(),"artist");
}

const char* ModuleRenderer::getTitle() const
{
	return openmpt_module_get_metadata(module.get(),"title");
}

const char* ModuleRenderer::getDate() const
{
	return openmpt_module_get_metadata(module.get(),"date");
}

FrameCount ModuleRenderer::readMono(SampleRate framerate, FrameCount frames, int16_t* mono)
{
	return FrameCount(openmpt_module_read_mono(module.get(),framerate.var,frames.var,mono));
}

FrameCount ModuleRenderer::readMono(SampleRate framerate, FrameCount frames, float* mono)
{
	return FrameCount(openmpt_module_read_float_mono(module.get(),framerate.var,frames.var,mono));
}

FrameCount ModuleRenderer::readStereo(SampleRate framerate, FrameCount frames, int16_t* left, int16_t* right)
{
	return FrameCount(openmpt_module_read_stereo(module.get(),framerate.var,frames.var, left, right));
}

FrameCount ModuleRenderer::readStereo(SampleRate framerate, FrameCount frames, float* left, float* right)
{
	return FrameCount(openmpt_module_read_float_stereo(module.get(),framerate.var,frames.var, left, right));
}

FrameCount ModuleRenderer::readQuad(SampleRate framerate, FrameCount frames, int16_t* left, int16_t* right, int16_t* rear_left, int16_t* rear_right)
{
	return FrameCount(openmpt_module_read_quad(module.get(),framerate.var,frames.var, left, right, rear_left, rear_right));
}

FrameCount ModuleRenderer::readQuad(SampleRate framerate, FrameCount frames, float* left, float* right, float* rear_left, float* rear_right)
{
	return FrameCount(openmpt_module_read_float_quad(module.get(),framerate.var,frames.var, left, right, rear_left, rear_right));
}

FrameCount ModuleRenderer::readInterleavedStereo(SampleRate framerate, FrameCount frames, int16_t* interleaved_stereo)
{
	return FrameCount(openmpt_module_read_interleaved_stereo(module.get(),framerate.var,frames.var,interleaved_stereo));
}

FrameCount ModuleRenderer::readInterleavedStereo(SampleRate framerate, FrameCount frames, float* interleaved_stereo)
{
	return FrameCount(openmpt_module_read_interleaved_float_stereo(module.get(),framerate.var,frames.var,interleaved_stereo));
}

FrameCount ModuleRenderer::readInterleavedQuad(SampleRate framerate, FrameCount frames, int16_t* interleaved_quad)
{
	return FrameCount(openmpt_module_read_interleaved_quad(module.get(),framerate.var,frames.var,interleaved_quad));
}

FrameCount ModuleRenderer::readInterleavedQuad(SampleRate framerate, FrameCount frames, float* interleaved_quad)
{
	return FrameCount(openmpt_module_read_interleaved_float_quad(module.get(),framerate.var,frames.var,interleaved_quad));
}

}
}
}
