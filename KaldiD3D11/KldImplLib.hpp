#ifndef KLDIMPLLIB_HPP
#define KLDIMPLLIB_HPP
#include <Elvavena/Util/ElvUtilGlobals.hpp>

#if defined(MH_KALDI_IMPL_EXPORT)
#define MH_KALDI_IMPL_API MH33_API_EXPORT
#else
#define MH_KALDI_IMPL_API MH33_API_IMPORT
#endif

#endif // KLDIMPLLIB_HPP
