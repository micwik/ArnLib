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

#ifndef ARNITEMB_HPP
#define ARNITEMB_HPP

#include "ArnLib_global.hpp"
#include "ArnLinkHandle.hpp"
#include "ArnError.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"
#include <QTextStream>
#include <QObject>
#include <QMetaMethod>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QAtomicInt>

class ArnItemBPrivate;
class QTimer;
class ArnLink;


//! Base class handle for an _Arn Data Object_.
/*!
[About Arn Data Object](\ref gen_arnobj)

This class contains the basic services, that should be apropriate for any derived class
as public methods. Other non generic services that might be needed is available as
protected methods. Typically derived classes can select among these protected methods and
make any of them public.

See ArnItem.
*/
class ARNLIBSHARED_EXPORT ArnItemB : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnItemB)

public:
    //! Code used in blob for arnExport() and arnImport()
    struct ExportCode {
        enum _ARN_ENUM_PACKED_  E {
            ByteArray  = 3,
            String     = 4,
            Variant    = 5,  // Legacy
            VariantTxt = 16,
            VariantBin = 17
        };
        MQ_DECLARE_ENUM( ExportCode)
    };

    //! Standard constructor of a closed handle
    /*! \param[in] parent
     */
    ArnItemB( QObject* parent = 0);

    virtual  ~ArnItemB();

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

signals:
    //! Signal emitted when the _Arn Data Object_ is destroyed.
    /*! When the link (_Arn Data Object_) is destroyed, this ArnItem is closed and
     *  will give this signal. It's ok to assign values etc to a closed ArnItem, it's
     *  thrown away like a null device.
     *  \see destroyLink()
     */
    void  arnLinkDestroyed();

    //! \cond ADV
protected:
    //! Open a handle to an Arn Object with a unique uuid name
    /*! If _path_ is marked as provider, the "!" marker will be moved to after uuid.
     *  \param[in] path The prefix for Arn uuid path e.g. "//Names/name"
     *  \retval false if error
     */
    bool  openUuid( const QString& path);

    //! Open a handle to an Arn Pipe Object with a unique uuid name
    /*! If _path_ is marked as provider, the "!" marker will be moved to after uuid.
     *  \param[in] path The prefix for Arn uuid pipe path e.g. "//Pipes/pipe"
     *  \retval false if error
     */
    bool  openUuidPipe( const QString& path);

    //! Open a handle to an Arn folder
    /*! \param[in] path The Arn folder path e.g. "//Measure/Water" (the / is appended)
     *  \retval false if error
     */
    bool  openFolder( const QString& path);

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
     *  \see addSyncMode()
     *  \see \ref gen_arnobjModes
     */
    Arn::ObjectSyncMode  syncMode()  const;

    //! Set _general mode_ as Bidirectional for this _Arn Data Object_
    /*! A two way object, typically for validation or pipe
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    ArnItemB&  setBiDirMode();

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
    ArnItemB&  setPipeMode();

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
    ArnItemB&  setSaveMode();

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
    ArnItemB&  setMaster();

    /*! \retval true if _Master mode_
     *  \see setMaster()
     *  \see \ref gen_arnobjModes
     */
    bool  isMaster()  const;

    //! Set client session _sync mode_ as _AutoDestroy_ for this ArnItem
    /*! This ArnItem at client side is setup for auto destruction.
     *  \pre This must be set before open().
     */
    ArnItemB&  setAutoDestroy();

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

    /*! \return Convert _Arn Data Object_ to a _integer_
     */
    int  toInt()  const;

    /*! \return Convert _Arn Data Object_ to a _double_
     */
    double  toDouble()  const;

    /*! \return Convert _Arn Data Object_ to an _ARNREAL_
     */
    ARNREAL  toReal()  const;

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

    void  setValue( const ArnItemB& other, int ignoreSame = Arn::SameValue::DefaultAction);

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

    //// To be reimplemented
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  modeUpdate( Arn::ObjectMode mode, bool isSetup = false);
    virtual void  itemCreatedBelow( const QString& path);
    virtual void  itemModeChangedBelow( const QString& path, uint linkId, Arn::ObjectMode mode);
    virtual void  customEvent( QEvent* ev);

    //// Methods not to be public
    bool  openWithFlags( const QString& path, Arn::LinkFlags linkFlags);
    void  setForceKeep( bool fk = true);
    bool  isForceKeep()  const;
    Arn::ObjectMode  getMode( ArnLink* link)  const;
    void  addSyncMode( Arn::ObjectSyncMode syncMode, bool linkShare);
    void  resetOnlyEcho();
    bool  isOnlyEcho()  const;
    void  setBlockEcho( bool blockEcho);
    uint  retireType();
    void  setEnableSetValue( bool enable);
    void  setEnableUpdNotify( bool enable);
    void  setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData);
    void  trfValue( const QByteArray& value, int sendId, bool forceKeep,
                    const ArnLinkHandle& handleData);
    void  arnImport( const QByteArray& data, int ignoreSame, ArnLinkHandle& handleData);
    QStringList  childItemsMain()  const;
    void  errorLog( const QString& errText, ArnError err = ArnError::Undef, void* reference = 0)  const;

    ArnItemB( ArnItemBPrivate& dd, QObject* parent);
    ArnItemBPrivate* const  d_ptr;
    //! \endcond

private slots:

private:
    void  init();
    void  setupOpenItem( bool isFolder);
    //bool  open( const ArnItemB& folder, const QString& itemName, bool isFolder);

    ArnLink*  _link;
};

#endif // ARNITEMB_HPP
