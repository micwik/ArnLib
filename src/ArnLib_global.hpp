#ifndef ARNLIB_GLOBAL_HPP
#define ARNLIB_GLOBAL_HPP

#include <QtCore/qglobal.h>

#if defined(ARNLIB_LIBRARY)
#  define ARNLIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define ARNLIBSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // ARNLIB_GLOBAL_HPP
