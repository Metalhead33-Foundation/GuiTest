#include <MhLib/Media/Audio/MhResampler.hpp>
#include <cstdlib>
#include <cstring>
#include <samplerate.h>

long default_sf_callback( void *self, float **data );
namespace MH33 {
namespace Audio {

#define SIMP_HNDL reinterpret_cast<SRC_DATA*>(handle)

SimpleResampler::~SimpleResampler()
{
	if(handle) free(handle);
}

SimpleResampler::SimpleResampler() : handle(malloc(sizeof(SRC_DATA)) )
{
	memset(handle,0,sizeof(SRC_DATA));
}

SimpleResampler::SimpleResampler(float *dataIn, FrameCount inputFrames, float *dataOut, FrameCount outputFrames, double src_ratio)
	: handle(malloc(sizeof(SRC_DATA)) )
{
	memset(handle,0,sizeof(SRC_DATA));
	SIMP_HNDL->data_in = dataIn;
	SIMP_HNDL->input_frames = inputFrames.var;
	SIMP_HNDL->data_out = dataOut;
	SIMP_HNDL->output_frames = outputFrames.var;
	SIMP_HNDL->src_ratio = src_ratio;
}

SimpleResampler::SimpleResampler(SimpleResampler &&mov) : handle(mov.handle)
{
	mov.handle = nullptr;
}

SimpleResampler &SimpleResampler::operator=(SimpleResampler &&mov)
{
	if(handle) free(handle);
	this->handle = mov.handle;
	mov.handle = nullptr;
	return *this;
}

int SimpleResampler::resample(ResampleType resamplerType, ChannelCount channels) const
{
	return src_simple(SIMP_HNDL,int(resamplerType),channels.var);
}

const float *SimpleResampler::getDataIn() const
{
	return SIMP_HNDL->data_in;
}

float *SimpleResampler::getDataOut() const
{
	return SIMP_HNDL->data_out;
}

FrameCount SimpleResampler::getInputFrames() const
{
	return FrameCount(SIMP_HNDL->input_frames);
}

FrameCount SimpleResampler::getOutputFrames() const
{
	return FrameCount(SIMP_HNDL->output_frames);
}

double SimpleResampler::getRatio() const
{
	return SIMP_HNDL->src_ratio;
}
void SimpleResampler::setDataIn( const float *set_to ) const {
	SIMP_HNDL->data_in = set_to;
}
void SimpleResampler::setDataOut( float *set_to ) const {
	SIMP_HNDL->data_out = set_to;
}
void SimpleResampler::setInputFrames( FrameCount set_to ) const {
	SIMP_HNDL->input_frames = set_to.var;
}
void SimpleResampler::setOutputFrames( FrameCount set_to ) const {
	SIMP_HNDL->output_frames = set_to.var;
}
void SimpleResampler::setRatio( double set_to ) const {
	SIMP_HNDL->src_ratio = set_to;
}
void SimpleResampler::setEndOfInput( bool set_to ) const {
	SIMP_HNDL->end_of_input = set_to;
}
void SimpleResampler::set( const float *data_in, float *data_out, FrameCount input_frames,
		 FrameCount output_frames, double ratio ) const {
	SIMP_HNDL->data_in = data_in;
	SIMP_HNDL->input_frames = input_frames.var;
	SIMP_HNDL->data_out = data_out;
	SIMP_HNDL->output_frames = output_frames.var;
	SIMP_HNDL->src_ratio = ratio;
}

/*
	void* handleA;
	void* handleB;
	ChannelCount channels;
	ResampleType converterType;
*/
#define FULL_STAT reinterpret_cast<SRC_STATE*>(handleA)
#define FULL_DAT reinterpret_cast<SRC_DATA*>(handleB)

FullResampler::~FullResampler()
{
	if(handleA) src_delete(FULL_STAT);
	if(handleB) free(handleB);
}

FullResampler::FullResampler(ChannelCount channels, ResampleType converterType)
	: handleA(nullptr), handleB( malloc(sizeof(SRC_DATA)) ), channels(channels), converterType(converterType)
{
	memset(handleB,0,sizeof(SRC_DATA));
	handleA = src_new(int(this->converterType),channels.var,&(FULL_DAT->end_of_input));
}

FullResampler::FullResampler(FullResampler &&mov) : handleA(mov.handleA), handleB(mov.handleB), channels(mov.channels), converterType(mov.converterType)
{
	mov.handleA = nullptr;
	mov.handleB = nullptr;
}

FullResampler &FullResampler::operator=(FullResampler &&mov)
{
	this->channels = mov.channels;
	this->converterType = mov.converterType;
	if(handleA) src_delete(FULL_STAT);
	this->handleA = mov.handleA;
	mov.handleA = nullptr;
	if(handleB) free(handleB);
	this->handleB = mov.handleB;
	mov.handleB = nullptr;
	return *this;
}

int FullResampler::reset( ) const {
	return src_reset(FULL_STAT);
}
int FullResampler::process( ) const {
	return src_process(FULL_STAT,FULL_DAT);
}
const float *FullResampler::getDataIn() const
{
	return FULL_DAT->data_in;
}

float *FullResampler::getDataOut() const
{
	return FULL_DAT->data_out;
}

FrameCount FullResampler::getInputFrames() const
{
	return FrameCount(FULL_DAT->input_frames);
}

FrameCount FullResampler::getOutputFrames() const
{
	return FrameCount(FULL_DAT->output_frames);
}

double FullResampler::getRatio() const
{
	return FULL_DAT->src_ratio;
}
void FullResampler::setDataIn( const float *set_to ) const {
	FULL_DAT->data_in = set_to;
}
void FullResampler::setDataOut( float *set_to ) const {
	FULL_DAT->data_out = set_to;
}
void FullResampler::setInputFrames( FrameCount set_to ) const {
	FULL_DAT->input_frames = set_to.var;
}
void FullResampler::setOutputFrames( FrameCount set_to ) const {
	FULL_DAT->output_frames = set_to.var;
}
void FullResampler::setRatio( double set_to ) const {
	FULL_DAT->src_ratio = set_to;
}
void FullResampler::setEndOfInput( bool set_to ) const {
	FULL_DAT->end_of_input = set_to;
}
void FullResampler::set( const float *data_in, float *data_out, FrameCount input_frames,
						FrameCount output_frames, double ratio ) const {
	FULL_DAT->data_in = data_in;
	FULL_DAT->input_frames = input_frames.var;
	FULL_DAT->data_out = data_out;
	FULL_DAT->output_frames = output_frames.var;
	FULL_DAT->src_ratio = ratio;
}


ChannelCount FullResampler::getChannelCount( ) const { return channels; }
ResampleType FullResampler::getResamplerType( ) const { return converterType; }

/*
	int* error;
	ResampleType converterType;
	void* handle;
	ChannelCount channels;
*/

#define CALLBACK_HNDL reinterpret_cast<SRC_STATE*>(handle)

CallbackResampler::~CallbackResampler()
{
	if(error) free(error);
	if(handle) src_delete(CALLBACK_HNDL);
}

CallbackResampler::CallbackResampler(CallbackResampler &&mov)
	: error(mov.error), converterType(mov.converterType), handle(mov.handle), channels(mov.channels)
{

}

CallbackResampler &CallbackResampler::operator=(CallbackResampler &&mov)
{
	this->converterType = mov.converterType;
	this->channels = mov.channels;
	if(error) free(error);
	this->error = mov.error;
	mov.error = nullptr;
	if(handle) src_delete(CALLBACK_HNDL);
	this->handle = mov.handle;
	mov.handle = nullptr;
	return *this;
}

CallbackResampler::CallbackResampler(samplerate_callback func, ResampleType converter_type, ChannelCount channels, void *userdata)
	: error(reinterpret_cast<int*>(malloc(sizeof(int)))), converterType(converter_type), handle(nullptr), channels(channels)
{
	handle = src_callback_new(func,int(converter_type),channels.var,error,userdata);
}

CallbackResampler::CallbackResampler(CallbackInterfaceBase *interface, ResampleType converter_type, ChannelCount channels)
{
	handle = src_callback_new(default_sf_callback,int(converter_type),channels.var,error,interface);
}

FrameCount CallbackResampler::read(double src_ratio, FrameCount frames, float *data) const
{
	return FrameCount(src_callback_read(CALLBACK_HNDL,src_ratio,frames.var,data));
}

int CallbackResampler::reset() const
{
	return src_reset(CALLBACK_HNDL);
}

ChannelCount CallbackResampler::getChannelCount() const
{
	return channels;
}

ResampleType CallbackResampler::getResamplerType() const
{
	return converterType;
}

CallbackInterface::CallbackInterface(ChannelCount channels, ResampleType converter_type)
	: CallbackResampler( this, converter_type, channels )
{

}


}
}
long default_sf_callback( void *self, float **data ) {
	if ( self ) {
		auto sampler = reinterpret_cast< MH33::Audio::CallbackInterfaceBase * >( self );
		auto def = sampler->callBack( );
		*data = def.second;
		return def.first;
	} else
		return 0;
}
