#ifndef MHIO_HPP
#define MHIO_HPP
#include <MhLib/Util/MhGlobals.hpp>

#if defined(MH_IO_EXPORT)
#define MH_IO_API MH33_API_EXPORT
#else
#define MH_IO_API MH33_API_IMPORT
#endif

#endif // MHIO_HPP
