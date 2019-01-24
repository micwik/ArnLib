// Copyright (C) 2010-2018 Michael Wiklund.
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

#ifndef ARNBASICITEM_HPP
#define ARNBASICITEM_HPP

#include "ArnLib_global.hpp"
#include "ArnCoreItem.hpp"
#include "ArnLinkHandle.hpp"
#include "ArnError.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QAtomicInt>
#include <QObject>

class ArnBasicItemPrivate;
class ArnLink;
class ArnEvent;


//! \cond ADV
class ArnBasicItemEventHandler : public QObject
{
    Q_OBJECT

public:
    explicit ArnBasicItemEventHandler( QObject* parent = 0);
    virtual ~ArnBasicItemEventHandler();

    static void  defaultEvent( QEvent* ev);

protected:
    virtual void  customEvent( QEvent* ev);
};
//! \endcond


//! Base class handle for an _Arn Data Object_.
/*!
[About ArnItem access](\ref gen_arnItem)

See ArnItem.

ArnBasicItem is the basic way to get a handle (pointer) for accessing an Arn Data Object.
It is fast, small and is not based on QObject. As such it can not use signals and slots,
but it can provide ArnEvents (based on QEvents) to be sent to any QObject based receiver.

There can be any amount of ArnBasicItem:s opened (pointing) to the same
_Arn Data object_. Deleting the ArnBasicItem won't effect the _Arn Data object_.

This class is not thread-safe, but the _Arn Data object_ is, so each thread should
have it's own handles i.e ArnBasicItem instances.

<b>Example usage</b> \n \code
    // In class declare
    ArnBasicItem  _arnTime;
    MyReceiver  _myRec;  // QObject derived

    // In class code
    _arnTime.open("//Chat/Time/value");
    _arnTime.setEventHandler( &_myRec);
    _arnTime = "Undefined ...";

void  MyReceiver::customEvent( QEvent* ev)
{
    // Is setup as ArnEvent handler for my ArnBasicItem.
    // Handler must finish with ArnBasicItemEventHandler::defaultEvent( ev).

    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::ValueChange:
    {
        ArnEvValueChange*  e = static_cast<ArnEvValueChange*>( ev);
        ArnBasicItem*  item = static_cast<ArnBasicItem*>( e->target());
        if (!item)  break;  // No target, deleted/closed ...

        QByteArray  val = e->valueData() ? *e->valueData() : item->toByteArray();
        qDebug() << "MyReceiver ArnEvValueChange: inItemPath=" << item->path()
                 << " value=" << val;
    }
    default:
        break;
    }

    ArnBasicItemEventHandler::defaultEvent( ev);
}
\endcode
*/
class ARNLIBSHARED_EXPORT ArnBasicItem : public ArnCoreItem
{
    Q_DECLARE_PRIVATE(ArnBasicItem)
    friend class ArnBasicItemEventHandler;

public:
    //! Standard constructor of a closed handle
    /*!
     */
    ArnBasicItem();

    virtual  ~ArnBasicItem();

    //! Open a handle to an _Arn Data Object_
    /*! \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \retval false if error
     */
    bool  open( const QString& path);

    //! Close the handle
    void  close();

    //! Destroy the _Arn Data Object_
    /*! The link (_Arn Data Object_) will be removed locally and optionally from server
     *  and all connected clients. Server is allways forcing global destroy.
     *  \param[in] isGlobal If true, removes from server and all connected clients,
     *                      otherwise only local link.
     *  \see destroyLinkLocal()
     */
    void  destroyLink( bool isGlobal = true);

    //! Destroy the local _Arn Data Object_
    /*! The link (_Arn Data Object_) will be removed locally. Server is allways forcing
     *  global destroy.
     *  \see destroyLink()
     */
    void  destroyLinkLocal()
    { destroyLink( false);}

    //! State of the handle
    /*! \retval true if this ArnItem is open
     */
    bool  isOpen()  const;

    //! Path of the _Arn Data Object_
    /*! \param[in] nameF The format of the returned path
     *  \return The object path
     */
    QString  path( Arn::NameF nameF = Arn::NameF::EmptyOk)  const;

    //! Name of the _Arn Data Object_
    /*! \param[in] nameF The format of the returned name
     *  \return The object name
     */
    QString  name( Arn::NameF nameF)  const;

    //! Set an associated external reference
    /*! This is typically used when having many _ArnItems_ changed signal connected
     *  to a common slot.
     *  The slot can then discover the signalling ArnItem:s associated structure
     *  for further processing.
     *  \param[in] reference Any external structure or id.
     *  \see reference()
     */
    void  setReference( void* reference);

    //! Get the stored external reference
    /*! \return The associated external reference
     *  \see setReference()
     */
    void*  reference()  const;

    //! Get the _id_ for this ArnItem
    /*! The ArnItem _id_ is unique within its running program. Even if 2 ArnItems are
     *  pointing to the same _Arn Data Object_, they have different _item id_.
     *  \return _id_ for this ArnItem
     *  \see linkId()
     */
    uint  itemId()  const;

    //! Get the _id_ for this _Arn Data Object_
    /*! The link (_Arn Data Object_) _id_ is unique within its running program.
     *  If 2 ArnItems are pointing to the same _Arn Data Object_, they have same
     *  _link id_.
     *  \return Id for the _Arn Data Object_, 0 if closed
     *  \see itemId()
     */
    uint  linkId()  const;

    //! Get the number of refs to this _Arn Data Object_
    /*! \return The number of refs for the _Arn Data Object_, -1 if closed
     */
    int  refCount()  const;

    /*! \retval true if this ArnItem is a folder
     */
    bool  isFolder()  const;

    /*! \retval true if this ArnItem is a provider
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     */
    bool  isProvider()  const;

    //! The type stored in the _Arn Data Object_
    /*! \return The type stored
     */
    Arn::DataType  type()  const;

    //! Set skipping of equal value
    /*! \param[in] isIgnore If true, assignment of equal value don't give a changed signal.
     */
    void  setIgnoreSameValue( bool isIgnore = true);

    /*! \retval true if skipping equal values
     *  \see setIgnoreSameValue()
     */
    bool  isIgnoreSameValue()  const;

    //! Add _general mode_ settings for this _Arn Data Object_
    /*! If this ArnItem is in closed state, the added modes will be stored and
     *  the real mode change is done when this ArnItem is opened to an
     *  _Arn Data Object_. This implies that ArnItems can benefit from setting
     *  _modes_ before opening.
     *  \param[in] mode The _modes_ to be added.
     *  \see getMode()
     *  \see \ref gen_arnobjModes
     */
    void  addMode( Arn::ObjectMode mode);

    /*! \return The _general mode_ of the _Arn Data Object_
     *  \see addMode()
     *  \see \ref gen_arnobjModes
     */
    Arn::ObjectMode  getMode()  const;

    /*! \return The client session _sync mode_ of an _Arn Data Object_
     *  \see \ref gen_arnobjModes
     */
    Arn::ObjectSyncMode  syncMode()  const;

    //! Set _general mode_ as Bidirectional for this _Arn Data Object_
    /*! A two way object, typically for validation or pipe
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    ArnBasicItem&  setBiDirMode();

    /*! \retval true if Bidirectional
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    bool  isBiDirMode()  const;

    //! Set _general mode_ as Pipe for this _Arn Data Object_
    /*! Implies _Bidir_.
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_pipeArnobj
     */
    ArnBasicItem&  setPipeMode();

    /*! \retval true if _Pipe mode_
     *  \see setPipeMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_pipeArnobj
     */
    bool  isPipeMode()  const;

    //! Set _general mode_ as _Save_ for this _Arn Data Object_
    /*! Data is persistent and will be saved
     *  \pre The persistent service must be started at the server.
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_persistArnobj
     */
    ArnBasicItem&  setSaveMode();

    /*! \retval true if _Save mode_
     *  \see setSaveMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_persistArnobj
     */
    bool  isSaveMode()  const;

    //! Set client session _sync mode_ as _Master_ for this ArnItem
    /*! This ArnItem at client side is set as default generator of data.
     *  \pre This must be set before open().
     *  \see \ref gen_arnobjModes
     */
    ArnBasicItem&  setMaster();

    /*! \retval true if _Master mode_
     *  \see setMaster()
     *  \see \ref gen_arnobjModes
     */
    bool  isMaster()  const;

    //! Set client session _sync mode_ as _AutoDestroy_ for this ArnItem
    /*! This ArnItem at client side is setup for auto destruction.
     *  \pre This must be set before open().
     */
    ArnBasicItem&  setAutoDestroy();

    /*! \retval true if _AutoDestroy mode_
     *  \see setAutoDestroy()
     */
    bool  isAutoDestroy()  const;

    //! Import data to an _Arn Data Object_
    /*! Data blob from a previos \p arnExport() can be imported.
     *  This is essentially assigning the _Arn Data Object_ with same as exported.
     *  \param[in] data is the data blob
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see arnExport()
     *  \see setIgnoreSameValue()
     */
    void  arnImport( const QByteArray& data, int ignoreSame = Arn::SameValue::DefaultAction);

    /*! \return A data blob representing the _Arn Data Object_
     *  \see arnImport()
     */
    QByteArray  arnExport()  const;

    /*! \return Convert _Arn Data Object_ to an _integer_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    int  toInt( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to a _double_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    double  toDouble( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to an _ARNREAL_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    ARNREAL  toReal( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to a _QString_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    QString  toString( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to a _QByteArray_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    QByteArray  toByteArray( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to a _QVariant_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    QVariant  toVariant( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to a _bool_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from Int.
     */
    bool  toBool( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to an _unsigned int_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from ByteArray.
     */
    uint  toUInt( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to an _int 64 bit_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from ByteArray.
     */
    qint64  toInt64( bool* isOk = 0)  const;

    /*! \return Convert _Arn Data Object_ to an _unsigned int 64 bit_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from ByteArray.
     */
    quint64  toUInt64( bool* isOk = 0)  const;

    ArnBasicItem&  operator=( const ArnBasicItem& other);
    ArnBasicItem&  operator=( int val);
    ArnBasicItem&  operator=( ARNREAL val);
    ArnBasicItem&  operator=( const QString& val);
    ArnBasicItem&  operator=( const QByteArray& val);
    ArnBasicItem&  operator=( const QVariant& val);
    ArnBasicItem&  operator=( const char* val);
    ArnBasicItem&  operator=( uint val);
    ArnBasicItem&  operator=( qint64 val);
    ArnBasicItem&  operator=( quint64 val);

    void  setValue( const ArnBasicItem& other, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign an _integer_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( int value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign an _ARNREAL_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( ARNREAL value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign a _bool_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( bool value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign a _QString_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QString& value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign a _QByteArray_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QByteArray& value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign a _QVariant_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QVariant& value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign a _char*_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const char* value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign an _unsigned int_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     *  \note Not native ARN datatype. ByteArray is assigned.
     */
    void  setValue( uint value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign an _int 64 bit_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     *  \note Not native ARN datatype. ByteArray is assigned.
     */
    void  setValue( qint64 value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Assign an _unsigned int 64 bit_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     *  \note Not native ARN datatype. ByteArray is assigned.
     */
    void  setValue( quint64 value, int ignoreSame = Arn::SameValue::DefaultAction);

    //! Get the thread affinity of this ArnBasicItem
    /*! The affinity (see QObject) is set when the ArnBasicItem is created and bound to an
     *  internal QObject based event handler. When a custom event handler is set, its
     *  affinity is used.
     *  \return the thread affinity
     *  \see setEventHandler()
     */
    QThread*  thread()  const;

    //! Set event handler for this ArnBasicItem
    /*! The event handler must be QObject based
     *  \param[in] eventHandler to be assigned
     *  \see eventHandler()
     *  \see thread()
     */
    void  setEventHandler( QObject* eventHandler);

    //! Get the event handler of this ArnBasicItem
    /*! \return the event handler
     *  \see setEventHandler()
     *  \see thread()
     */
    QObject*  eventHandler()  const;

    //! Set a Bidirectional item as Unidirectional
    /*! The two way object is not twisted at writes, i.e. exactly the same object is read
     *  and written. This has no effect on an _Arn Data Object_ that not is in
     *  Bidirectional mode.
     *  \see isUniDir()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    void  setUniDir( bool isUnidir = true);

    //! Get the Unidirectional state of an object
    /*! \retval true if Unidirectional is set or _Arn Data Object_ is not in Bidirectional mode.
     *  \see setUniDir()
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    bool  isUniDir()  const;

    //! \cond ADV
    bool  sendArnEventLink( ArnEvent* ev);
    void  sendArnEventItem( ArnEvent* ev, bool isAlienThread, bool isLocked = false);
    quint32  localUpdateCount()  const;

protected:
    virtual void  arnEvent( QEvent* ev, bool isAlienThread);

    //// Methods not to be public
    bool  openWithFlags( const QString& path, Arn::LinkFlags linkFlags);
    /*! \obsolete
     */
    void  setForceKeep( bool fk = true);
    /*! \obsolete
     */
    bool  isForceKeep()  const;
    Arn::ObjectMode  getMode( ArnLink* link)  const;
    void  addSyncMode( Arn::ObjectSyncMode syncMode, bool linkShare);
    void  resetOnlyEcho();
    void  addIsOnlyEcho( quint32 sendId);
    bool  isOnlyEcho()  const;
    uint  retireType();
    void  setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData);
    void  setValue( const QVariant& value, int ignoreSame, ArnLinkHandle& handleData);
    void  arnImport( const QByteArray& data, int ignoreSame, ArnLinkHandle& handleData);
    QStringList  childItemsMain()  const;
    void  errorLog( const QString& errText, ArnError err = ArnError::Undef, void* reference = 0)  const;

    ArnBasicItem( ArnBasicItemPrivate& dd);
    //! \endcond

private:
    void  init();
    void  setupOpenItem( bool isFolder);
    ArnBasicItemEventHandler*  getThreadEventHandler();

    ArnLink*  _link;
};

#endif // ARNBASICITEM_HPP
