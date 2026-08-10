#ifndef HARUKALIB_HPP
#define HARUKALIB_HPP
/**
 * @file HarukaLib.hpp
 * @brief Declares the HarukaLib API in the Haruka module.
 *
 * This header is part of the public declaration surface for Haruka.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Elvavena/Util/ElvUtilGlobals.hpp>

#if defined(MH_HARUKA_EXPORT)
#define MH_HARUKA_API MH33_API_EXPORT
#else
/** @brief Documents the MH_HARUKA_API constant or macro. */
#define MH_HARUKA_API MH33_API_IMPORT
#endif

#endif // HARUKALIB_HPP
