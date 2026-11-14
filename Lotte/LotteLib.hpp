#ifndef LOTTELIB_HPP
#define LOTTELIB_HPP
/**
 * @file LotteLib.hpp
 * @brief Declares the LotteLib API in the Lotte module.
 *
 * This header is part of the public declaration surface for Lotte.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */

#include <Elvavena/Util/ElvUtilGlobals.hpp>

#if defined(MH_LOTTE_EXPORT)
#define MH_LOTTE_API MH33_API_EXPORT
#else
/** @brief Documents the MH_LOTTE_API constant or macro. */
#define MH_LOTTE_API MH33_API_IMPORT
#endif

#endif // LOTTELIB_HPP
