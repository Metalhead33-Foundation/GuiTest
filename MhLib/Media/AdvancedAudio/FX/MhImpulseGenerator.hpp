#ifndef MHIMPULSEGENERATOR_HPP
#define MHIMPULSEGENERATOR_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/FX/MhConvolver.hpp>
namespace MH33 {
namespace Audio {
namespace  FX {

fftconvolver::sIrBuffer MH_ADV_AUDIO_API createLowpassFilter(FrameRate inputSamplerate,FrameRate CutoffFrequency, bool normalize=false);
fftconvolver::sIrBuffer MH_ADV_AUDIO_API createHighpassFilter(FrameRate inputSamplerate,FrameRate CutoffFrequency, bool normalize=false);
fftconvolver::sIrBuffer MH_ADV_AUDIO_API createBandpassFilter(FrameRate inputSamplerate,FrameRate lowCutoff, FrameRate highCutoff, bool normalize=false);
fftconvolver::sIrBuffer MH_ADV_AUDIO_API createBandrejectFilter(FrameRate inputSamplerate,FrameRate lowCutoff, FrameRate highCutoff, bool normalize=false);
fftconvolver::sIrBuffer MH_ADV_AUDIO_API createDelay(FrameRate inputSamplerate, SampleIndex sampleOffset, float delayVolume=1.0f);

}
}
}

#endif // MHIMPULSEGENERATOR_HPP
