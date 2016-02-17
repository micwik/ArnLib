// Copyright (C) 2010-2015 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these
// other libraries is subject to their respective license agreements.
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

#include <ArnInc/ArnEvent.hpp>
#include <ArnLink.hpp>


ArnEvent::ArnEvent( QEvent::Type type)
    : QEvent( type)
    , _target(0)
{
}


int  ArnEvent::baseType( int setVal)
{
    static int bt = -1;

    if (bt > 0)  return bt;

    //// Set base type and check its validity
    bt = setVal;
    if (bt < 0)
        bt = 2022;  // Arn default
    Q_ASSERT_X((bt >= QEvent::User) && (bt + Idx::N <= QEvent::MaxUser), "ArnEvent::baseType()",
               "Selected base for ArnEvent number is out of boundary for Qt User event");

    //// Register all ArnEvents
    for (int i = 0; i < Idx::N; ++i) {
        int wantType = bt + i;
        int gotType  = QEvent::registerEventType( wantType);
        Q_ASSERT_X(gotType == wantType, "ArnEvent::baseType()",
                   "Assigning event number for ArnEvent is already taken,"
                   " use ArnEvent::baseType() to set other base event number for ArnEvents.");
    }

    return bt;
}


bool  ArnEvent::isArnEvent( int evType)
{
    int  bt = baseType();

    return (evType >= bt) && (evType < bt + Idx::N);
}


#define TO_IDX_RETVAL(evType) \
    int  retVal = (evType) - baseType(); \
    retVal = ((retVal >= 0) && (retVal < Idx::N)) ? retVal : Idx::QtEvent;


int  ArnEvent::toIdx( QEvent::Type type)
{
    TO_IDX_RETVAL(type)
    return retVal;
}


int  ArnEvent::toIdx()  const
{
    TO_IDX_RETVAL(type())
    return retVal;
/*
    int  retVal = type() - baseType();

    if ((retVal >= 0) && (retVal < Idx::N))
        return retVal;
    else
        return Idx::QtEvent;
*/
}


QString  ArnEvent::toString( QEvent::Type type)
{
    return Idx::txt().getTxtString( toIdx( type)) +
            "(" + QString::number( type) + ")";
}


QString  ArnEvent::toString()  const
{
    return toString( type());
}


ArnEvent*  ArnEvent::copyOpt( const ArnEvent* other)
{
    _target = other->_target;

    return this;
}



ArnEvValueChange::ArnEvValueChange( int sendId, const QByteArray* valueData, const ArnLinkHandle& handleData)
    : ArnEvent( type())
    , _sendId( sendId)
    , _valueData( valueData)
    , _handleData( &handleData)
{
    if (_valueData)
        _valueData = new QByteArray( *valueData);
    if (_handleData && !_handleData->isNull())
        _handleData = new ArnLinkHandle( *_handleData);
    else
        _handleData = 0;
}


ArnEvValueChange::~ArnEvValueChange()
{
    if (_valueData)
        delete _valueData;
    if (_handleData)
        delete _handleData;
}


QEvent::Type  ArnEvValueChange::type()
{
    static int evType = baseType() + Idx::ValueChange;

    return Type( evType);
}


ArnEvent*  ArnEvValueChange::makeHeapClone()
{
    return (new ArnEvValueChange( _sendId, _valueData, *_handleData))->copyOpt( this);
}



ArnEvLinkCreate::ArnEvLinkCreate( const QString& path, ArnLink* arnLink, bool isLastLink)
    : ArnEvent( type())
    , _path( path)
    , _arnLink( arnLink)
    , _isLastLink( isLastLink)
{
}


QEvent::Type  ArnEvLinkCreate::type()
{
    static int evType = baseType() + Idx::LinkCreate;

    return Type( evType);
}


ArnEvent*  ArnEvLinkCreate::makeHeapClone()
{
    return (new ArnEvLinkCreate( _path, _arnLink, _isLastLink))->copyOpt( this);
}



ArnEvModeChange::ArnEvModeChange( const QString& path, uint linkId, Arn::ObjectMode mode)
    : ArnEvent( type())
    , _path( path)
    , _linkId( linkId)
    , _mode( mode)
{
}


QEvent::Type  ArnEvModeChange::type()
{
    static int evType = baseType() + Idx::ModeChange;

    return Type( evType);
}


ArnEvent*  ArnEvModeChange::makeHeapClone()
{
    return (new ArnEvModeChange( _path, _linkId, _mode))->copyOpt( this);
}



ArnEvRetired::ArnEvRetired( ArnLink* startLink, bool isBelow, bool isGlobal)
    : ArnEvent( type())
    , _startLink( startLink)
    , _isBelow( isBelow)
    , _isGlobal( isGlobal)
{
}


QEvent::Type ArnEvRetired::type()
{
    static int evType = baseType() + Idx::Retired;

    return Type( evType);
}


ArnEvent*  ArnEvRetired::makeHeapClone()
{
    return (new ArnEvRetired( _startLink, _isBelow, _isGlobal))->copyOpt( this);
}



ArnEvZeroRef::ArnEvZeroRef( ArnLink* arnLink)
    : ArnEvent( type())
    , _arnLink (arnLink)
{
}


QEvent::Type  ArnEvZeroRef::type()
{
    static int evType = baseType() + Idx::ZeroRef;

    return Type( evType);
}


ArnEvent*  ArnEvZeroRef::makeHeapClone()
{
    return (new ArnEvZeroRef( _arnLink))->copyOpt( this);
}
