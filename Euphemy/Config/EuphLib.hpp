#ifndef EUPHLIB_HPP
#define EUPHLIB_HPP
/**
 * @file EuphLib.hpp
 * @brief Declares the EuphLib API in the Euphemy/Config module.
 *
 * This header is part of the public declaration surface for Euphemy/Config.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Elvavena/Util/ElvUtilGlobals.hpp>

#if defined(MH_EUPH_EXPORT)
#define MH_EUPH_API MH33_API_EXPORT
#else
/** @brief Documents the MH_EUPH_API constant or macro. */
#define MH_EUPH_API MH33_API_IMPORT
#endif

#endif // EUPHLIB_HPP
