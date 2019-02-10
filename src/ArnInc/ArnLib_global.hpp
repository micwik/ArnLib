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
#  define ArnNullPtr  nullptr
#else
#  if 1
    struct nullptr_t
    {
        template <class T>
            inline operator T* ()  {return (T*)0;}
    };
    static struct nullptr_t __attribute__((used)) ArnNullPtr;
#  else
#    define ArnNullPtr  0
#  endif
#endif

#endif // ARNLIB_GLOBAL_HPP
