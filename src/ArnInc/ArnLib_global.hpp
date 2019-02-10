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

#if __cplusplus >= 201103L || (__cplusplus < 200000 && __cplusplus > 199711L)
// Use C++11 nullptr
#else
    struct nullptr_t
    {
        template <class T>
            operator T* ()  {return (T*)0;}
    } nullptr;
#endif

#endif // ARNLIB_GLOBAL_HPP
