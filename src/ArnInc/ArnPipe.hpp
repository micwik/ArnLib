// Copyright (C) 2010-2016 Michael Wiklund.
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

#ifndef ARNPIPE_HPP
#define ARNPIPE_HPP

#include "ArnLib_global.hpp"
#include "ArnItemB.hpp"

class ArnPipePrivate;


//! ArnItem specialized as a pipe.
/*!
[About Pipes](\ref gen_pipeArnobj)

This class is not thread-safe, but the _Arn Data object_ is, so each thread should
have it's own handles i.e ArnPipe instances.

<b>Example usage</b> \n \code
    // In class declare
    ArnPipe  _arnPipe;

    // In class code
    _arnPipe.open("//Pipes/Pipe/value");
    _arnPipe.setSendSeq( true);
    _arnPipe.setCheckSeq( true);
    connect( &_arnPipe., SIGNAL(outOfSequence()), this, SLOT(doOutOfSequence()));
    connect( &_arnPipe, SIGNAL(changed(QByteArray)), this, SLOT(doPipeInput(QByteArray)));

    QRegExp rx("^ping\\b");
    _arnPipe.setValueOverwrite( "ping new", rx);
\endcode
*/
class ARNLIBSHARED_EXPORT ArnPipe : public ArnItemB
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnPipe)

public:
    //! Standard constructor of a closed handle
    /*! \param[in] parent
     */
    ArnPipe( QObject* parent = 0);

    //! Construction of a pipe handle to a _path_
    /*! The mode for this handle is set to Arn::ObjectMode::Pipe.
     *  \param[in] path The _Arn Data Object_ path e.g. "//Pipes/myPipe/value"
     *  \param[in] parent
     *  \see open()
     */
    ArnPipe( const QString& path, QObject* parent = 0);

    virtual  ~ArnPipe();

    //! Open a handle to an Arn Pipe Object with a unique uuid name
    /*! If _path_ is marked as provider, the "!" marker will be moved to after uuid.
     *  \param[in] path The prefix for Arn uuid pipe path e.g. "//Pipes/pipe"
     *  \retval false if error
     */
    bool  openUuid( const QString& path)
    {return ArnItemB::openUuidPipe( path);}

    //! Set client session _sync mode_ as _Master_ for this ArnItem
    /*! This ArnItem at client side is set as default generator of data.
     *  \pre This must be set before open().
     *  \see \ref gen_arnobjModes
     */
    ArnPipe&  setMaster()
    {ArnItemB::setMaster(); return *this;}

    /*! \retval true if _Master mode_
     *  \see setMaster()
     *  \see \ref gen_arnobjModes
     */
    bool  isMaster()  const
    {return ArnItemB::isMaster();}

    //! Set client session _sync mode_ as _AutoDestroy_ for this ArnItem
    /*! This ArnItem at client side is setup for auto destruction.
     *  \pre This must be set before open().
     */
    ArnPipe&  setAutoDestroy()
    {ArnItemB::setAutoDestroy(); return *this;}

    /*! \retval true if _AutoDestroy mode_
     *  \see setAutoDestroy()
     */
    bool  isAutoDestroy()  const
    {return ArnItemB::isAutoDestroy();}

    ArnPipe&  operator=( const QByteArray& value);

    //! Assign a _QByteArray_ to a _Pipe_ by using _Anti congest_ logic
    /*! This is used to limit the filling of sendqueue with recuring messages during
     *  some kind of client disconnection. Matched message in sendqueue is overwritten
     *  by the new message _value_. Unmatched message is added to send queue as usual.
     *
     *  Example:
     *  > // Messages starts with a function name  <Br>
     *  > // We want message with equal function name to overwrite   <Br>
     *  > QRegExp rx("^" + funcName + "\\b");      <Br>
     *  > _pipe->setValueOverwrite( message, rx);  <Br>
     *  \param[in] value to be assigned
     *  \param[in] rx is regexp to be matched with items in send queue.
     *  \see \ref gen_pipeAntiCongest
     */
    void  setValueOverwrite( const QByteArray& value, const QRegExp& rx);

    //! Returns true if sending sequence numbers
    /*! \retval true if sending sequence numbers
     *  \see setSendSeq()
     */
    bool  isSendSeq()  const;

    //! Change usage of sending sequence numbers
    /*! \param[in] useSendSeq is true for activation
     *  \see isSendSeq()
     *  \see setCheckSeq()
     *  \see outOfSequence()
     *  \see \ref gen_pipeSeqCheck
     */
    void  setSendSeq( bool useSendSeq);

    //! Returns true if checking received sequence numbers
    /*! \retval true if checking received sequence numbers
     *  \see setCheckSeq()
     */
    bool  isCheckSeq()  const;

    //! Change usage of checking received sequence numbers
    /*! \param[in] useCheckSeq is true for activation
     *  \see isCheckSeq()
     *  \see setSendSeq()
     *  \see outOfSequence()
     *  \see \ref gen_pipeSeqCheck
     */
    void  setCheckSeq( bool useCheckSeq);

public slots:
    //! Assign a _QByteArray_ to a _Pipe_
    /*! \param[in] value to be assigned
     */
    void  setValue( const QByteArray& value);

signals:
    //! Signal emitted when _Pipe_ has received data
    /*! This is implied by the _Arn Data Object_ is changed.
     *  \param[in] value is the received bytes
     */
    void  changed( const QByteArray& value);

    //! Signal emitted when the received sequence numbers are "out of sequence"
    /*! \see setCheckSeq()
     *  \see setSendSeq()
     *  \see \ref gen_pipeSeqCheck
     */
    void  outOfSequence();

    //! \cond ADV
protected:
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);

    ArnPipe( ArnPipePrivate& dd, QObject* parent);
    //! \endcond

private slots:

private:
    void  init();
    void  setupSeq( ArnLinkHandle& handleData);
};

#endif // ARNPIPE_HPP
