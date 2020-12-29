#include "ArnInc/ArnCompat.hpp"


#if QT_VERSION >= 0x060000

ArnRegExp::ArnRegExp()
{
}


ArnRegExp::ArnRegExp( const QString& pattern)
    : QRegularExpression( pattern)
{
}


ArnRegExp::ArnRegExp( const QRegularExpression& re)
    : QRegularExpression( re)
{
}


int  ArnRegExp::indexIn( const QString& str)  const
{
    _regMatch = match( str);
    return _regMatch.capturedStart(0);
}


QString  ArnRegExp::cap( int nth)  const
{
    return _regMatch.captured( nth);
}

#else
#endif
