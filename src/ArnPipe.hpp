// Copyright (C) 2010-2013 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#ifndef ARNPIPE_HPP
#define ARNPIPE_HPP

#include "ArnLib_global.hpp"
#include "ArnItem.hpp"


//! Handle for an _Arn Data Object_ specialized as a pipe.
/*!
[About Arn Data Object](\ref gen_arnobj)

When opening an ArnItem to an _Arn Data object_, the ArnItem act as a handle (pointer)
to the object. There can be any amount of ArnItem:s opened (pointing) to the same
_Arn Data object_. Deleting the ArnItem won't effect the _Arn Data object_.

This class is not thread-safe, but the _Arn Data object_ is, so each thread should
have it's own handles i.e ArnItem instances.

<b>Example usage</b> \n \code
    // In class declare
    ArnItem  _arnTime;

    // In class code
    _arnTime.open("//Chat/Time/value");
    connect( &_arnTime, SIGNAL(changed(QString)), this, SLOT(doTimeUpdate(QString)));
    _arnTime = "Undefined ...";
\endcode
*/

class ARNLIBSHARED_EXPORT ArnPipe : public ArnItem
{
    Q_OBJECT

public:
    //! Standard constructor of a closed handle
    ArnPipe( QObject* parent = 0);

    //! Construction of a handle to a path
    /*! \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \see open()
     */
    ArnPipe( const QString& path, QObject* parent = 0);

    virtual  ~ArnPipe();

    //! Open a handle to an Arn Pipe Object with a unique uuid name
    /*! \param[in] path The prefix for Arn uuid pipe path e.g. "//Pipes/pipe"
     *  \retval false if error
     */
    // bool  openUuid( const QString& path);

    //! Assign a _QByteArray_ to a _Pipe_.
    /*! \param[in] value to be assigned
     */
    void  setValue( const QByteArray& value);

    ArnItem&  operator=( const QByteArray& value)
        {setValue( value); return *this;}

    //! Assign a _QByteArray_ to a _Pipe_ by overwrite Regexp match in sendqueue
    /*! \param[in] value to be assigned
     *  \param[in] rx is regexp to be matched with items in send queue.
     */
    void  setValueOverwrite( const QByteArray& value, const QRegExp& rx);

    bool  useSendSeq()  const;
    void  setUseSendSeq( bool useSendSeq);
    bool  useCheckSeq()  const;
    void  setUseCheckSeq( bool useCheckSeq);

signals:
    //! Signal emitted when _Pipe_ _Arn Data Object_ is changed.
    /*!
     *
     */
    void  changed( QByteArray value);

    void  outOfSequence();

    //! \cond ADV
protected:
    virtual void  itemUpdateStart( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  itemUpdateEnd();
    //! \endcond

private slots:

private:
    void  init();
    void  setupSeq( ArnLinkHandle& handleData);
    /// Source for unique id to all ArnItem ..
    //static QAtomicInt  _idCount;
    bool  _useSendSeq;
    bool  _useCheckSeq;
    int  _sendSeqNum;
    int  _checkSeqNum;
};

#endif // ARNPIPE_HPP
