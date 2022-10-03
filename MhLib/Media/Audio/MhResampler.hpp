#ifndef MHRESAMPLER_HPP
#define MHRESAMPLER_HPP
#include <MhLib/Media/Audio/MhAudio.hpp>
#include <MhLib/Media/Audio/MhAudioIterator.hpp>
namespace MH33 {
namespace Audio {

enum class ResampleType : int {
	SINC_BEST_QUALITY = 0,
	SINC_MEDIUM_QUALITY = 1,
	SINC_FASTEST = 2,
	ZERO_ORDER_HOLD = 3,
	LINEAR = 4
};


class MH_AUDIO_API SimpleResampler {
private:
	void* handle;
	SimpleResampler( const SimpleResampler &cpy ) = delete;
	SimpleResampler& operator=( const SimpleResampler &cpy ) = delete;
public:
	~SimpleResampler( );
	SimpleResampler( );
	SimpleResampler( float *dataIn, FrameCount inputFrames, float *dataOut,
					FrameCount outputFrames, double src_ratio );
	SimpleResampler( SimpleResampler &&mov );
	SimpleResampler& operator=( SimpleResampler &&mov );

	int resample( ResampleType resamplerType, ChannelCount channels ) const;
	const float *getDataIn( ) const;
	float *getDataOut( ) const;
	FrameCount getInputFrames( ) const;
	FrameCount getOutputFrames( ) const;
	double getRatio( ) const;
	bool getEndOfInput( ) const;
	void setDataIn( const float *set_to ) const;
	void setDataOut( float *set_to ) const;
	void setInputFrames( FrameCount set_to ) const;
	void setOutputFrames( FrameCount set_to ) const;
	void setRatio( double set_to ) const;
	void setEndOfInput( bool set_to ) const;
	void set( const float *data_in, float *data_out, FrameCount input_frames,
			 FrameCount output_frames, double ratio ) const;
};

class MH_AUDIO_API FullResampler {
private:
	void* handleA;
	void* handleB;
	ChannelCount channels;
	ResampleType converterType;
	FullResampler( const FullResampler &cpy ) = delete;
	FullResampler& operator=( const FullResampler &cpy ) = delete;
public:
	~FullResampler( );
	FullResampler( ChannelCount channels, ResampleType converterType =
									ResampleType::SINC_FASTEST );
	FullResampler( FullResampler &&mov );
	FullResampler& operator=( FullResampler &&mov );
	int reset( ) const;
	int process( ) const;
	const float *getDataIn( ) const;
	float *getDataOut( ) const;
	FrameCount getInputFrames( ) const;
	FrameCount getOutputFrames( ) const;
	double getRatio( ) const;
	bool getEndOfInput( ) const;
	void setDataIn( const float *set_to ) const;
	void setDataOut( float *set_to ) const;
	void setInputFrames( FrameCount set_to ) const;
	void setOutputFrames( FrameCount set_to ) const;
	void setRatio( double set_to ) const;
	void setEndOfInput( bool set_to ) const;
	void set(const float *data_in, float *data_out, FrameCount input_frames,
			 FrameCount output_frames, double ratio ) const;

	ChannelCount getChannelCount( ) const;
	ResampleType getResamplerType( ) const;
};

typedef long ( *samplerate_callback )( void *cb_data, float **data );

class CallbackInterfaceBase {
public:
	typedef std::pair< long, float * > CallbackArgs;
	virtual ~CallbackInterfaceBase( ) = default;
	virtual CallbackArgs callBack( ) = 0;
};

class MH_AUDIO_API CallbackResampler {
private:
	int* error;
	ResampleType converterType;
	void* handle;
	ChannelCount channels;
	CallbackResampler( const CallbackResampler &cpy ) = delete;
	CallbackResampler& operator=( const CallbackResampler &cpy ) = delete;
public:
	virtual ~CallbackResampler();
	CallbackResampler( CallbackResampler &&mov );
	CallbackResampler& operator=( CallbackResampler &&mov );
	CallbackResampler( samplerate_callback func,
					  ResampleType converter_type, ChannelCount channels,
					  void *userdata );
	CallbackResampler( CallbackInterfaceBase *interface,
					  ResampleType converter_type, ChannelCount channels );
	FrameCount read( double src_ratio, FrameCount frames, float *data ) const;
	int reset( ) const;
	ChannelCount getChannelCount( ) const;
	ResampleType getResamplerType( ) const;
};

class MH_AUDIO_API CallbackInterface : public CallbackInterfaceBase,
						  public CallbackResampler {
public:
	virtual ~CallbackInterface( ) = default;
	CallbackInterface(ChannelCount channels, ResampleType converter_type =
											ResampleType::SINC_FASTEST );
	virtual CallbackArgs callBack( ) = 0;
};

}
}
#endif // MHRESAMPLER_HPP
