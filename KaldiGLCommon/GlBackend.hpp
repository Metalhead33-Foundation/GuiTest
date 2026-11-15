#ifndef KALDIGLBACKEND_HPP
#define KALDIGLBACKEND_HPP
/**
 * @file GlBackend.hpp
 * @brief Dialect selector shared by the KaldiGL and KaldiGLES backends.
 *
 * `KaldiGLCommon` holds everything that is identical between the desktop OpenGL
 * and the OpenGL ES backends. A translation unit is compiled into *one* of the
 * two libraries, and the build system decides which dialect it targets:
 *
 *  - `KaldiGL.pro` defines `GL_IMPLEMENTATION`, so the sources target OpenGL 4.3
 *    core, live in namespace `GL` and include `<glad/gl.h>`.
 *  - `KaldiGLES.pro` defines `GLES_IMPLEMENTATION`, so the sources target
 *    OpenGL ES 3.2, live in namespace `GLES` and include `<glad/gles2.h>`.
 *
 * The two backends never share a translation unit, therefore a single macro
 * (`GLES_IMPLEMENTATION`) is enough to select the loader header, the namespace
 * and the few capability differences that GLSL ES imposes on us. Always use
 * the `KALDIGL_NS` / `KALDIGL_IS_GLES` macros below instead of repeating the
 * `#ifdef` pairs in every file - that is exactly how the duplicated legacy
 * copies of `GlUtil` and `GlSpirvShader` drifted apart.
 *
 * This header deliberately does @em not include glad: `GlSpirvShader.hpp` only
 * needs the namespace, and keeping the loader out of it lets tooling use the
 * SPIR-V translation without a GL context. `GlUtil.hpp` includes the right
 * loaders itself.
 */
#ifdef GLES_IMPLEMENTATION
/** @brief Namespace of the OpenGL / OpenGL ES backend the current unit is built for. */
#define KALDIGL_NS GLES
/** @brief 1 when the current translation unit targets OpenGL ES, 0 for desktop OpenGL. */
#define KALDIGL_IS_GLES 1
#else
/** @brief Namespace of the OpenGL / OpenGL ES backend the current unit is built for. */
#define KALDIGL_NS GL
/** @brief 1 when the current translation unit targets OpenGL ES, 0 for desktop OpenGL. */
#define KALDIGL_IS_GLES 0
#endif

namespace KALDIGL_NS {

/** @brief True when this translation unit was compiled for OpenGL ES. */
inline constexpr bool isOpenGLES = (KALDIGL_IS_GLES != 0);

}

/**
 * @brief Expands to @p desktopToken on desktop OpenGL and to @c 0 on OpenGL ES.
 *
 * Several format tokens simply do not exist in the GLES 3.2 header set
 * (`GL_BGR`, `GL_R16`, `GL_DOUBLE`, ...). A runtime `?:` is not enough, because
 * the token still has to be declared, so the dialect has to be decided by the
 * preprocessor. The macro keeps the format tables readable:
 *
 * @code
 * case Format::R16U: return KALDIGL_GL_DESKTOP_ONLY(GL_R16);
 * @endcode
 */
#if KALDIGL_IS_GLES
#define KALDIGL_GL_DESKTOP_ONLY(desktopToken) 0
#else
#define KALDIGL_GL_DESKTOP_ONLY(desktopToken) (desktopToken)
#endif

#endif // KALDIGLBACKEND_HPP
