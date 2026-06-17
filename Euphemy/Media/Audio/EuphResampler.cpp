#include "EuphResampler.hpp"
long default_sf_callback( void *self, float **data );
namespace Euph {
namespace Media {
namespace Audio {
void SrcStateDeleter::operator( )( SRC_STATE *state ) const
{
	if ( state ) src_delete( state );
}

SimpleResampler::~SimpleResampler() = default;

SimpleResampler::SimpleResampler() : handle { } { }

SimpleResampler::SimpleResampler(float *dataIn, FrameCount inputFrames, float *dataOut, FrameCount outputFrames, double src_ratio)
	: handle { }
{
	handle.data_in = dataIn;
	handle.input_frames = inputFrames.var;
	handle.data_out = dataOut;
	handle.output_frames = outputFrames.var;
	handle.src_ratio = src_ratio;
}

SimpleResampler::SimpleResampler(SimpleResampler &&mov) = default;
SimpleResampler &SimpleResampler::operator=(SimpleResampler &&mov) = default;

int SimpleResampler::resample(ResampleType resamplerType, ChannelCount channels) const
{
	return src_simple(&handle,int(resamplerType),channels.var);
}

const float *SimpleResampler::getDataIn() const
{
	return handle.data_in;
}

float *SimpleResampler::getDataOut() const
{
	return handle.data_out;
}

FrameCount SimpleResampler::getInputFrames() const
{
	return FrameCount(handle.input_frames);
}

FrameCount SimpleResampler::getInputFramesUsed() const
{
	return FrameCount(handle.input_frames_used);
}

FrameCount SimpleResampler::getOutputFrames() const
{
	return FrameCount(handle.output_frames);
}

FrameCount SimpleResampler::getOutputFramesGenerated() const
{
	return FrameCount(handle.output_frames_gen);
}

double SimpleResampler::getRatio() const
{
	return handle.src_ratio;
}
bool SimpleResampler::getEndOfInput() const
{
	return handle.end_of_input;
}
void SimpleResampler::setDataIn( const float *set_to ) const {
	handle.data_in = set_to;
}
void SimpleResampler::setDataOut( float *set_to ) const {
	handle.data_out = set_to;
}
void SimpleResampler::setInputFrames( FrameCount set_to ) const {
	handle.input_frames = set_to.var;
}
void SimpleResampler::setOutputFrames( FrameCount set_to ) const {
	handle.output_frames = set_to.var;
}
void SimpleResampler::setRatio( double set_to ) const {
	handle.src_ratio = set_to;
}
void SimpleResampler::setEndOfInput( bool set_to ) const {
	handle.end_of_input = set_to;
}
void SimpleResampler::set( const float *data_in, float *data_out, FrameCount input_frames,
						  FrameCount output_frames, double ratio ) const {
	handle.data_in = data_in;
	handle.input_frames = input_frames.var;
	handle.data_out = data_out;
	handle.output_frames = output_frames.var;
	handle.src_ratio = ratio;
}

FullResampler::~FullResampler() = default;

FullResampler::FullResampler(ChannelCount channels, ResampleType converterType)
	: handleA(nullptr), handleB { }, channels(channels), converterType(converterType)
{
	handleA.reset(src_new(int(this->converterType),channels.var,&(handleB.end_of_input)));
}

FullResampler::FullResampler(FullResampler &&mov) = default;
FullResampler &FullResampler::operator=(FullResampler &&mov) = default;

int FullResampler::reset( ) const {
	return src_reset(handleA.get());
}
int FullResampler::process( ) const {
	return src_process(handleA.get(),&handleB);
}
const float *FullResampler::getDataIn() const
{
	return handleB.data_in;
}

float *FullResampler::getDataOut() const
{
	return handleB.data_out;
}

FrameCount FullResampler::getInputFrames() const
{
	return FrameCount(handleB.input_frames);
}

FrameCount FullResampler::getInputFramesUsed() const
{
	return FrameCount(handleB.input_frames_used);
}

FrameCount FullResampler::getOutputFrames() const
{
	return FrameCount(handleB.output_frames);
}

FrameCount FullResampler::getOutputFramesGenerated() const
{
	return FrameCount(handleB.output_frames_gen);
}

double FullResampler::getRatio() const
{
	return handleB.src_ratio;
}
bool FullResampler::getEndOfInput() const
{
	return handleB.end_of_input;
}
void FullResampler::setDataIn( const float *set_to ) const {
	handleB.data_in = set_to;
}
void FullResampler::setDataOut( float *set_to ) const {
	handleB.data_out = set_to;
}
void FullResampler::setInputFrames( FrameCount set_to ) const {
	handleB.input_frames = set_to.var;
}
void FullResampler::setOutputFrames( FrameCount set_to ) const {
	handleB.output_frames = set_to.var;
}
void FullResampler::setRatio( double set_to ) const {
	handleB.src_ratio = set_to;
}
void FullResampler::setEndOfInput( bool set_to ) const {
	handleB.end_of_input = set_to;
}
void FullResampler::set( const float *data_in, float *data_out, FrameCount input_frames,
						FrameCount output_frames, double ratio ) const {
	handleB.data_in = data_in;
	handleB.input_frames = input_frames.var;
	handleB.data_out = data_out;
	handleB.output_frames = output_frames.var;
	handleB.src_ratio = ratio;
}


ChannelCount FullResampler::getChannelCount( ) const { return channels; }
ResampleType FullResampler::getResamplerType( ) const { return converterType; }

CallbackResampler::~CallbackResampler() = default;

CallbackResampler::CallbackResampler(CallbackResampler &&mov)
	= default;

CallbackResampler &CallbackResampler::operator=(CallbackResampler &&mov) = default;

CallbackResampler::CallbackResampler(samplerate_callback func, ResampleType converter_type, ChannelCount channels, void *userdata)
	: error(0), converterType(converter_type), handle(nullptr), channels(channels)
{
	handle.reset(src_callback_new(func,int(converter_type),channels.var,&error,userdata));
}

CallbackResampler::CallbackResampler(CallbackInterfaceBase *interface, ResampleType converter_type, ChannelCount channels)
	: error(0), converterType(converter_type), handle(nullptr), channels(channels)
{
	handle.reset(src_callback_new(default_sf_callback,int(converter_type),channels.var,&error,interface));
}

FrameCount CallbackResampler::read(double src_ratio, FrameCount frames, float *data) const
{
	return FrameCount(src_callback_read(handle.get(),src_ratio,frames.var,data));
}

int CallbackResampler::reset() const
{
	return src_reset(handle.get());
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
}

long default_sf_callback( void *self, float **data ) {
	if ( self ) {
		auto sampler = reinterpret_cast< Euph::Media::Audio::CallbackInterfaceBase * >( self );
		auto def = sampler->callBack( );
		*data = def.second;
		return def.first;
	} else
		return 0;
}
