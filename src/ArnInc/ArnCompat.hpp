// Copyright (C) 2010-2020 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE_LGPL.txt included in the packaging of this
// file. In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General Public
// License version 3.0 as published by the Free Software Foundation and appearing
// in the file LICENSE_GPL.txt included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and conditions
// contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//

#ifndef ARNCOMPAT_HPP
#define ARNCOMPAT_HPP

#include "ArnInc/ArnLib_global.hpp"

#if QT_VERSION >= 0x060000
  #include <QRegularExpression>
  #include <QRegularExpressionValidator>
  #include <QRecursiveMutex>
  #define ARN_RegExp   ArnRegExp
  #define ARN_RegExpValidator   QRegularExpressionValidator
  #define ARN_ToRegExp  toRegularExpression
  #define ARN_RecursiveMutex    QRecursiveMutex
  #define ARN_ModeRecursiveMutex
  #define ARN_SIZETYPE  qsizetype

class ARNLIBSHARED_EXPORT ArnRegExp : public QRegularExpression
{
public:
    ArnRegExp();
    ArnRegExp( const QString& pattern);
    ArnRegExp( const QRegularExpression& re);

    int  indexIn( const QString& str)  const;
    QString  cap( int nth)  const;

private:
    mutable QRegularExpressionMatch  _regMatch;
};

#else
  #include <QRegExp>
  #include <QMutex>
  #if (QT_VERSION >= 0x050000) && defined (QT_GUI_LIB)
    #include <QRegExpValidator>
  #endif
  #define ARN_RegExp   QRegExp
  #define ARN_RegExpValidator   QRegExpValidator
  #define ARN_ToRegExp  toRegExp
  #define ARN_RecursiveMutex    QMutex
  #define ARN_ModeRecursiveMutex    QMutex::Recursive
  #define ARN_SIZETYPE  int
#endif

#endif // ARNCOMPAT_HPP
