#ifndef ARNLIB_GLOBAL_HPP
#define ARNLIB_GLOBAL_HPP

#include <QtCore/qglobal.h>

#if defined(ARNLIB_LIBRARY)
#  define ARNLIBSHARED_EXPORT Q_DECL_EXPORT
#elif defined(ARNLIB_COMPILE)
#  define ARNLIBSHARED_EXPORT
#else
#  define ARNLIBSHARED_EXPORT Q_DECL_IMPORT
#endif

#if defined (__GNUC__)
#define _ARN_ENUM_PACKED_ __attribute__((packed))
#else
#define _ARN_ENUM_PACKED_
#endif

#endif // ARNLIB_GLOBAL_HPP
