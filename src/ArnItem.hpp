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

#ifndef ARNITEM_HPP
#define ARNITEM_HPP

#include "ArnLib_global.hpp"
#include "ArnError.hpp"
#include "ArnLink.hpp"
#include "MQFlags.hpp"
#include <QTextStream>
#include <QObject>
#include <QMetaMethod>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QAtomicInt>

class QTimer;


//! Handle for an _Arn Data Object_.
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
class ARNLIBSHARED_EXPORT ArnItem : public QObject
{
    Q_OBJECT
    friend class ArnClient;
    friend class ArnSync;

public:
    //! General global mode of an _Arn Data Object_
    struct Mode{
        enum E {
            //! A two way object, typically for validation or pipe
            BiDir = 0x01,
            //! Implies _BiDir_ and all data is preserved as a stream
            Pipe  = 0x02,
            //! Data is persistent and will be saved
            Save  = 0x04
        };
        MQ_DECLARE_FLAGS( Mode)
    };
    //! The client session sync mode of an _Arn Data Object_
    struct SyncMode{     // This mode is sent with sync-command
        enum E {
            //! default
            Normal      = 0x000,
            //! Monitor of server object for client
            Monitor     = 0x001,
            //! The client is default generator of data
            Master      = 0x100,
            //! Destroy this _Arn Data Object_ when client (tcp/ip) closes
            AutoDestroy = 0x200
        };
        MQ_DECLARE_FLAGS( SyncMode)
    };

    //! Standard constructor of a closed handle
    ArnItem( QObject* parent = 0);

    //! Construction of a handle to a path
    /*! \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \see open()
     */
    ArnItem( const QString& path, QObject* parent = 0);

    //! Construction of a handle to a path with a template for _modes_
    /*! \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \param[in] folder_template The template for setting _modes_
     */
    ArnItem( const ArnItem& folder_template, const QString& itemName_path, QObject* parent = 0);

    virtual  ~ArnItem();

    //! Open a handle to an _Arn Data Object_
    /*! \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \retval false if error
     */
    bool  open( const QString& path);

    //! Open a handle to an Arn Pipe Object with a unique uuid name
    /*! \param[in] path The prefix for Arn uuid pipe path e.g. "//Pipes/pipe"
     *  \retval false if error
     */
    bool  openUuidPipe( const QString& path);

    //! Open a handle to an Arn folder
    /*! \param[in] path The Arn folder path e.g. "//Measure/Water" (the / is appended)
     *  \retval false if error
     */
    bool  openFolder( const QString& path);

    //! Close the handle
    void  close();

    //! Destroy the _Arn Data Object_
    /*! The link (_Arn Data Object_) will be removed locally, from server and all
     *  connected clients.
     */
    void  destroyLink();

    //! State of the handle
    /*! \retval true if this ArnItem is open
     */
    bool  isOpen()  const;

    /*! \retval true if this ArnItem is a folder
     */
    bool  isFolder()  const;

    /*! \retval true if this ArnItem is bi-directional
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     */
    bool  isBiDir()  const;

    //! The type stored in the _Arn Data Object_
    /*! \return The type stored
     */
    ArnLink::Type  type()  const;

    //! Path of the _Arn Data Object_
    /*! \param[in] nameF The format of the returned path
     *  \return The object path
     */
    QString  path( ArnLink::NameF nameF = ArnLink::NameF::EmptyOk)  const;

    //! Name of the _Arn Data Object_
    /*! \param[in] nameF The format of the returned name
     *  \return The object name
     */
    QString  name( ArnLink::NameF nameF)  const;

    bool  isOnlyEcho()  const {return _isOnlyEcho;}
    void  setBlockEcho( bool blockEcho)  {_blockEcho = blockEcho;}

    //! Set skipping of equal value
    /*! \param[in] isIgnore If true, assignment of equal value don't give a changed signal.
     */
    void  setIgnoreSameValue( bool isIgnore = true);

    /*! \retval true if skipping equal values
     *  \see setIgnoreSameValue()
     */
    bool  isIgnoreSameValue();

    //! Set an associated external reference
    /*! This is typically used when having many _ArnItems_ changed signal connected
     *  to a common slot.
     *  The slot can then discover the signalling ArnItem:s associated structure
     *  for further processing.
     *  \param[in] reference Any external structure or id.
     *  \see reference()
     */
    void  setReference( void* reference)  {_reference = reference;}

    //! Get the stored external reference
    /*! \return The associated external reference
     *  \see setReference()
     */
    void*  reference()  const {return _reference;}

    //! Get the _id_ for this ArnItem
    /*! The ArnItem _id_ is unique within its running program. Even if 2 ArnItems are
     *  pointing to the same _Arn Data Object_, they have different _item id_.
     *  \return _id_ for this ArnItem
     *  \see linkId()
     */
    uint  itemId()  const {return _id;}

    //! Get the _id_ for this _Arn Data Object_
    /*! The link (_Arn Data Object_) _id_ is unique within its running program.
     *  If 2 ArnItems are pointing to the same _Arn Data Object_, they have same
     *  _link id_.
     *  \return Id for the _Arn Data Object_, 0 if closed
     *  \see itemId()
     */
    uint  linkId()  const;

    //! Add _general mode_ settings for this _Arn Data Object_
    /*! If this ArnItem is in closed state, the added modes will be stored and
     *  the real mode change is done when this ArnItem is opened to an
     *  _Arn Data Object_. This implies that ArnItems can benefit from setting
     *  _modes_ before opening.
     *  \param[in] mode The _modes_ to be added.
     *  \see getMode()
     *  \see \ref gen_arnobjModes
     */
    void  addMode( Mode mode);

    /*! \return The _general mode_ of the _Arn Data Object_
     *  \see addMode()
     *  \see \ref gen_arnobjModes
     */
    Mode  getMode()  const;

    /*! \return The client session _sync mode_ of an _Arn Data Object_
     *  \see addSyncMode()
     *  \see \ref gen_arnobjModes
     */
    SyncMode  syncMode()  const;

    //! Mark this ArnItem as a template
    /*! When marked as a template it can be setup with a combination of _modes_ which
     *  are used for other ArnItems using this template.
     *  The effected _modes_ can be both _general modes_ and _sync modes_.
     *  \param[in] isTemplate True for template mode.
     *  \see open()
     *  \see \ref gen_arnobjModes
     */
    ArnItem&  setTemplate( bool isTemplate = true);

    /*! \retval true if this is a template
     *  \see setTemplate()
     */
    bool  isTemplate()  const;

    //! Set _general mode_ as Bidirectional for this _Arn Data Object_
    /*! A two way object, typically for validation or pipe
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    ArnItem&  setBiDirMode();

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
    ArnItem&  setPipeMode();

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
    ArnItem&  setSaveMode();

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
    ArnItem&  setMaster();

    /*! \retval true if _Master mode_
     *  \see setMaster()
     *  \see \ref gen_arnobjModes
     */
    bool  isMaster()  const;

    //! Set client session _sync mode_ as _AutoDestroy_ for this ArnItem
    /*! This ArnItem at client side is setup for auto destruction.
     *  \pre This must be set before open().
     */
    ArnItem&  setAutoDestroy();

    /*! \retval true if _AutoDestroy mode_
     *  \see setAutoDestroy()
     */
    bool  isAutoDestroy()  const;

    //! Set _delay_ of data changed signal
    /*! Normally any change of the _Arn Data Object_ is immediately signalled.
     *  By setting this _delay_, intensive updates gives predictive and fewer signals.
     *  Signalling will not be faster than _delay_ as period time. The latency from
     *  a change to a signal will not be more than _delay_.
     *  \param[in] delay in ms.
     */
    void  setDelay( int delay);

    //! Import data to an _Arn Data Object_
    /*! Data blob from a previos \p arnExport() can be imported.
     *  This is essentially assigning the _Arn Data Object_ with same as exported.
     *  \param[in] data is the data blob
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see arnExport()
     *  \see setIgnoreSameValue()
     */
    void  arnImport( const QByteArray& data, int ignoreSame = -1);

    /*! \return A data blob representing the _Arn Data Object_
     *  \see arnImport()
     */
    QByteArray  arnExport()  const;

    /*! \return Convert _Arn Data Object_ to a _integer_
     */
    int  toInt()  const;

    /*! \return Convert _Arn Data Object_ to a _double_
     */
    double  toDouble()  const;

    /*! \return Convert _Arn Data Object_ to a _bool_
     */
    bool  toBool()  const;

    /*! \return Convert _Arn Data Object_ to a _QString_
     */
    QString  toString()  const;

    /*! \return Convert _Arn Data Object_ to a _QByteArray_
     */
    QByteArray  toByteArray()  const;

    /*! \return Convert _Arn Data Object_ to a _QVariant_
     */
    QVariant  toVariant()  const;

    ArnItem&  operator=( const ArnItem& other);
    ArnItem&  operator=( int other);
    ArnItem&  operator=( double other);
    ArnItem&  operator=( const QString& other);
    ArnItem&  operator=( const QByteArray& other);
    ArnItem&  operator=( const QVariant& other);
    ArnItem&  operator=( const char* other);

    //! Assign a _QByteArray_ to a _Pipe_ by overwrite Regexp match in sendqueue
    /*! \param[in] value to be assigned
     *  \param[in] rx is regexp to be matched with items in send queue.
     */
    void  setValuePipeOverwrite( const QByteArray& value, const QRegExp& rx);

public slots:    
    //! Assign an _integer_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( int value, int ignoreSame = -1);

    //! Assign a _double_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( double value, int ignoreSame = -1);

    //! Assign a _bool_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( bool value, int ignoreSame = -1);

    //! Assign a _QString_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QString& value, int ignoreSame = -1);

    //! Assign a _QByteArray_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QByteArray& value, int ignoreSame = -1);

    //! Assign a _QVariant_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QVariant& value, int ignoreSame = -1);

    //! Assign a _char*_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame -1 = don't care, otherwise overide ignoreSame setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const char* value, int ignoreSame = -1);

    //! Toggle the _bool_ at the _Arn Data Object_
    /*! The _Arn Data Object_ is first converted to a _bool_, then the toggled value
     *  is assigned back to the _Arn Data Object_.
     */
    void  toggleBool();

signals:
    //! Signals emitted when _Arn Data Object_ is changed.
    /*! Only the connected (used) signals are emitted for efficiency.
     *  When using pipes with queued connection to a slot, it's strongly advised to
     *  use the signal that carries the updated data. Otherwise some stream data can
     *  be lost and other will be doubled, because reading is done late in the slot.
     *
     *  changed(...) is using connectNotify & disconnectNotify.
     *  Must be updated if new types are added
     *  \see setIgnoreSameValue()
     */
    void  changed();

    /*! \see changed()
     */
    void  changed( int value);

    /*! \see changed()
     */
    void  changed( double value);

    /*! \see changed()
     */
    void  changed( bool value);

    /*! \see changed()
     */
    void  changed( QString value);

    /*! \see changed()
     */
    void  changed( QByteArray value);

    /*! \see changed()
     */
    void  changed( QVariant value);

    //! Signal emitted when an _Arn Data Object_ is created in the tree below.
    /*! The ArnItem is a folder. Created objects in this folder or its children
     *  will give this signal.
     *  Only created non folder objects will give this signal.
     *  \param[in] path to the created _Arn Data Object_
     */
    void  arnItemCreated( QString path);

    //! Signal emitted when an _Arn Data Object_ in the tree below has a _general mode_ change.
    /*! The ArnItem is a folder. Objects changing _general mode_ in this folder or
     *  its children will give this signal.
     *  \param[in] path to the _general mode_ changing _Arn Data Object_
     *  \param[in] linkId for the _general mode_ changing _Arn Data Object_
     *  \param[in] mode is the new _general mode_
     *  \see linkId()
     *  \see \ref gen_arnobjModes
     */
    void  arnModeChanged( QString path, uint linkId, ArnItem::Mode mode);

    //! Signal emitted when the _Arn Data Object_ is destroyed.
    /*! When the link (_Arn Data Object_) is destroyed, this ArnItem is closed and
     *  will give this signal. It's ok to assign values etc to a closed ArnItem, it's
     *  thrown away like a null device.
     *  \see destroyLink()
     */
    void  arnLinkDestroyed();

    //! \cond ADV
protected slots:
    virtual void  modeUpdate( bool isSetup = false);

protected:
    bool  open( const ArnItem& folder, const QString& itemName);
    void  setForceKeep( bool fk = true)  {_useForceKeep = fk;}
    Mode  getMode( ArnLink* link)  const;
    void  addSyncMode( SyncMode syncMode, bool linkShare);
    void  resetOnlyEcho()  {_isOnlyEcho = true;}
    virtual void  itemUpdateStart( const ArnLinkHandle& handleData)
                  {Q_UNUSED(handleData);}
    virtual void  itemUpdateEnd();
    QStringList  childItemsMain()  const;
    void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);

    ArnLink*  _link;
    //! \endcond

private slots:
    void  linkValueUpdated( uint sendId, const ArnLinkHandle& handleData);
    void  linkValueUpdated( uint sendId, QByteArray value, ArnLinkHandle handleData);
    void  timeoutItemUpdate();
    void  arnLinkCreatedBelow( ArnLink* link);
    void  arnModeChangedBelow( QString path, uint linkId);
    void  doArnLinkDestroyed();

private:
    void  init();
    void  setupOpenItem( bool isFolder);
    bool  open( const QString& path, bool isFolder);
    bool  open( const ArnItem& folder, const QString& itemName, bool isFolder);
    void  doItemUpdate( const ArnLinkHandle& handleData);
    void  setValue( const QByteArray& value, int ignoreSame, const ArnLinkHandle& handleData);
    void  trfValue( const QByteArray& value, int sendId, bool forceKeep,
                    const ArnLinkHandle& handleData = ArnLinkHandle());
    void  arnImport( const QByteArray& data, int ignoreSame, const ArnLinkHandle& handleData);

#if QT_VERSION >= 0x050000
    void  connectNotify( const QMetaMethod & signal);
    void  disconnectNotify( const QMetaMethod & signal);
    static QMetaMethod  _metaSignalChanged;
    static QMetaMethod  _metaSignalChangedInt;
    static QMetaMethod  _metaSignalChangedDouble;
    static QMetaMethod  _metaSignalChangedBool;
    static QMetaMethod  _metaSignalChangedString;
    static QMetaMethod  _metaSignalChangedByteArray;
    static QMetaMethod  _metaSignalChangedVariant;
#else
    void  connectNotify( const char* signal);
    void  disconnectNotify( const char* signal);
#endif

    /// Source for unique id to all ArnItem ..
    static QAtomicInt  _idCount;

    QTimer*  _delayTimer;

    int  _emitChanged;
    int  _emitChangedInt;
    int  _emitChangedDouble;
    int  _emitChangedBool;
    int  _emitChangedString;
    int  _emitChangedByteArray;
    int  _emitChangedVariant;

    SyncMode  _syncMode;
    Mode  _mode;
    bool  _syncModeLinkShare;
    bool  _useForceKeep;
    bool  _blockEcho;
    bool  _ignoreSameValue;
    bool  _isTemplate;
    bool  _isOnlyEcho;
    uint  _id;
    void*  _reference;
};

QTextStream&  operator<<(QTextStream& out, const ArnItem& item);

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnItem::Mode)
MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnItem::SyncMode)

#endif // ARNITEM_HPP
