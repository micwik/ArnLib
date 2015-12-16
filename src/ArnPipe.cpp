// Copyright (C) 2010-2014 Michael Wiklund.
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

#include "ArnInc/ArnPipe.hpp"
#include "private/ArnPipe_p.hpp"
#include "ArnInc/Arn.hpp"
#include <QDebug>


ArnPipePrivate::ArnPipePrivate()
{
    _useSendSeq  = true;
    _useCheckSeq = true;
    _sendSeqNum  = 0;
    _checkSeqNum = -1;  // Indicate first time (no history)
}


ArnPipePrivate::~ArnPipePrivate()
{
}


void  ArnPipe::init()
{
    setPipeMode();
}


ArnPipe::ArnPipe( QObject* parent)
    : ArnItemB( *new ArnPipePrivate, parent)
{
    init();
}


ArnPipe::ArnPipe( const QString& path, QObject* parent)
    : ArnItemB( *new ArnPipePrivate, parent)
{
    init();
    open( path);
}


ArnPipe::ArnPipe( ArnPipePrivate& dd, QObject* parent)
    : ArnItemB( dd, parent)
{
}


ArnPipe::~ArnPipe()
{
}


void  ArnPipe::setValue( const QByteArray& value)
{
    if (isOpen()) {
        ArnLinkHandle  handleData;
        setupSeq( handleData);
        ArnItemB::setValue( value, Arn::SameValue::Accept, handleData);
    }
    else {
        errorLog( QString(tr("Assigning bytearray Pipe:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


ArnPipe&  ArnPipe::operator=( const QByteArray& value)
{
    setValue( value);
    return *this;
}


void  ArnPipe::setValueOverwrite( const QByteArray& value, const QRegExp& rx)
{
    if (isOpen()) {
        ArnLinkHandle  handleData;
        handleData.add( ArnLinkHandle::QueueFindRegexp, QVariant( rx));
        ArnItemB::setValue( value, Arn::SameValue::Accept, handleData);
    }
    else {
        errorLog( QString(tr("Assigning bytearray PipeOW:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnPipe::setupSeq( ArnLinkHandle& handleData)
{
    Q_D(ArnPipe);

    if (!d->_useSendSeq)  return;  // Sequence not used

    handleData.add( ArnLinkHandle::SeqNo, QVariant( d->_sendSeqNum));
    d->_sendSeqNum = (d->_sendSeqNum + 1) % 1000;
}


bool  ArnPipe::isSendSeq()  const
{
    Q_D(const ArnPipe);

    return d->_useSendSeq;
}


void  ArnPipe::setSendSeq( bool useSeq)
{
    Q_D(ArnPipe);

    d->_useSendSeq = useSeq;
}


bool  ArnPipe::isCheckSeq()  const
{
    Q_D(const ArnPipe);

    return d->_useCheckSeq;
}


void  ArnPipe::setCheckSeq( bool useCheckSeq)
{
    Q_D(ArnPipe);

    d->_useCheckSeq = useCheckSeq;
}


void  ArnPipe::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    Q_D(ArnPipe);

    ArnItemB::itemUpdated( handleData, value);

    if (d->_useCheckSeq && handleData.has( ArnLinkHandle::SeqNo)) {
        int seqNum = handleData.valueRef( ArnLinkHandle::SeqNo).toInt();
        if (seqNum != d->_checkSeqNum) {  // Sequence not matching
            if (d->_checkSeqNum != -1)  // If not initial, this is out of sequence
                emit outOfSequence();
            d->_checkSeqNum = seqNum;  // Resync to this received SeqNum
        }
        d->_checkSeqNum = (d->_checkSeqNum + 1) % 1000;
    }
    if (value)
        emit changed( *value);
    else
        emit changed( toByteArray());
}
