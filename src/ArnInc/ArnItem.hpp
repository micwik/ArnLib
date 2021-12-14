// Copyright (C) 2010-2019 Michael Wiklund.
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

#ifndef ARNITEM_HPP
#define ARNITEM_HPP

#include "ArnLib_global.hpp"
#include "ArnItemB.hpp"
#include "ArnError.hpp"
#include <QTextStream>
#include <QObject>
#include <QMetaMethod>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QAtomicInt>

class ArnItemPrivate;


//! Handle for an _Arn Data Object_.
/*!
[About ArnItem access](\ref gen_arnItem)

See ArnBasicItem.

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
class ARNLIBSHARED_EXPORT ArnItem : public ArnItemB
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnItem)

public:
    //! Standard constructor of a closed handle
    /*! \param[in] parent
     */
    ArnItem( QObject* parent = 0);

    //! Construction of a handle to a path
    /*! \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \param[in] parent
     *  \see open()
     */
    ArnItem( const QString& path, QObject* parent = 0);

    //! Construction of a handle to a path with a template for _modes_
    /*! \param[in] itemTemplate The template for setting _modes_
     *  \param[in] path The _Arn Data Object_ path e.g. "//Measure/Water/Level/value"
     *  \param[in] parent
     */
    ArnItem( const ArnItem& itemTemplate, const QString& path, QObject* parent = 0);

    virtual  ~ArnItem();

    //! Open a handle to an Arn Object with a unique uuid name
    /*! \param[in] path The prefix for Arn uuid path e.g. "//Names/name"
     *  \retval false if error
     */
    bool  openUuid( const QString& path)
    {return ArnItemB::openUuid( path);}

    //! Open a handle to an Arn Pipe Object with a unique uuid name
    /*! \param[in] path The prefix for Arn uuid pipe path e.g. "//Pipes/pipe"
     *  \retval false if error
     */
    bool  openUuidPipe( const QString& path)
    {return ArnItemB::openUuidPipe( path);}

    //! Open a handle to an Arn folder
    /*! \param[in] path The Arn folder path e.g. "//Measure/Water" (the / is appended)
     *  \retval false if error
     */
    bool  openFolder( const QString& path)
    {return ArnItemB::openFolder( path);}

    /*! \retval true if this ArnItem is a folder
     */
    bool  isFolder()  const
    {return ArnItemB::isFolder();}

    /*! \retval true if this ArnItem is a provider
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     */
    bool  isProvider()  const
    {return ArnItemB::isProvider();}

    //! The type stored in the _Arn Data Object_
    /*! \return The type stored
     */
    Arn::DataType  type()  const
    {return ArnItemB::type();}

    //! Set skipping assignment of equal value
    /*! \param[in] isIgnore If true, assignment of equal value don't give a changed signal.
     */
    void  setIgnoreSameValue( bool isIgnore = true)
    {ArnItemB::setIgnoreSameValue( isIgnore);}

    /*! \retval true if skipping equal values
     *  \see setIgnoreSameValue()
     */
    bool  isIgnoreSameValue()
    {return ArnItemB::isIgnoreSameValue();}

    //! Add _general mode_ settings for this _Arn Data Object_
    /*! If this ArnItem is in closed state, the added modes will be stored and
     *  the real mode change is done when this ArnItem is opened to an
     *  _Arn Data Object_. This implies that ArnItems can benefit from setting
     *  _modes_ before opening.
     *  \param[in] mode The _modes_ to be added.
     *  \see getMode()
     *  \see \ref gen_arnobjModes
     */
    void  addMode( Arn::ObjectMode mode)
    {return ArnItemB::addMode( mode);}

    /*! \return The _general mode_ of the _Arn Data Object_
     *  \see addMode()
     *  \see \ref gen_arnobjModes
     */
    Arn::ObjectMode  getMode()  const
    {return ArnItemB::getMode();}

    /*! \return The client session _sync mode_ of an _Arn Data Object_
     *  \see \ref gen_arnobjModes
     */
    Arn::ObjectSyncMode  syncMode()  const
    {return ArnItemB::syncMode();}

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
    ArnItem&  setBiDirMode()
    {ArnItemB::setBiDirMode(); return *this;}

    /*! \retval true if Bidirectional
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    bool  isBiDirMode()  const
    {return ArnItemB::isBiDirMode();}

    //! Set _general mode_ as Pipe for this _Arn Data Object_
    /*! Implies _Bidir_.
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_pipeArnobj
     */
    ArnItem&  setPipeMode()
    {ArnItemB::setPipeMode(); return *this;}

    /*! \retval true if _Pipe mode_
     *  \see setPipeMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_pipeArnobj
     */
    bool  isPipeMode()  const
    {return ArnItemB::isPipeMode();}

    //! Set _general mode_ as _Save_ for this _Arn Data Object_
    /*! Data is persistent and will be saved
     *  \pre The persistent service must be started at the server.
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_persistArnobj
     */
    ArnItem&  setSaveMode()
    {ArnItemB::setSaveMode(); return *this;}

    /*! \retval true if _Save mode_
     *  \see setSaveMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_persistArnobj
     */
    bool  isSaveMode()  const
    {return ArnItemB::isSaveMode();}

    //! Set client session _sync mode_ as _Master_ for this ArnItem
    /*! This ArnItem at client side is set as default generator of data.
     *  \pre This must be set before open().
     *  \see \ref gen_arnobjModes
     */
    ArnItem&  setMaster()
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
    ArnItem&  setAutoDestroy()
    {ArnItemB::setAutoDestroy(); return *this;}

    /*! \retval true if _AutoDestroy mode_
     *  \see setAutoDestroy()
     */
    bool  isAutoDestroy()  const
    {return ArnItemB::isAutoDestroy();}

    //! Set a Bidirectional item as Uncrossed
    /*! The two way object is not twisted at writes, i.e. exactly the same object is read
     *  and written. This has no effect on an _Arn Data Object_ that not is in
     *  Bidirectional mode.
     *  \see isUncrossed()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    void  setUncrossed( bool isUncrossed = true)
    {return ArnItemB::setUncrossed( isUncrossed);}

    //! Get the Uncrossed state of an object
    /*! \retval true if Uncrossed is set or _Arn Data Object_ is not in Bidirectional mode.
     *  \see setUncrossed()
     *  \see setBiDirMode()
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_bidirArnobj
     */
    bool  isUncrossed()  const
    {return ArnItemB::isUncrossed();}

    //! Control echo cancellation for this item
    /*! When an ArnObject is changed via this item, the changed() signal on this item
     *  can be blocked.
     *  \param[in] blockEcho if true echo is blocked.
     */
    void  setBlockEcho( bool blockEcho = true)
    {return ArnItemB::setBlockEcho( blockEcho);}

    //! Set _delay_ of data changed signal
    /*! Normally any change of the _Arn Data Object_ is immediately signalled.
     *  By setting this _delay_, intensive updates gives predictive and fewer signals.
     *  Signalling will not be faster than _delay_ as period time. The latency from
     *  a change to a signal will not be more than _delay_.
     *  \param[in] delay in ms.
     *  \see delay()
     *  \see isDelayPending()
     *  \see bypassDelayPending()
     */
    void  setDelay( int delay);

    //! Get _delay_ of data changed signal
    /*! Read current value of the delay.
     *  \return the delay in ms.
     *  \see setDelay()
     */
    int  delay()  const;

    /*! Delay pending status
     *  \retval true if the _Arn Data Object_ is changed, but the changed signal is
     *          pending in a delay.
     *  \see setDelay()
     *  \see bypassDelayPending()
     */
    bool  isDelayPending()  const;

    /*! For delay pending, immediately signal changed
     *  If the changed signal is pending in a delay, the changed signal is immediately
     *  emitted and the delay is canceled. Otherwise nothing is done.
     *  \see setDelay()
     *  \see isDelayPending()
     */
    void  bypassDelayPending();

    //! Import data to an _Arn Data Object_
    /*! Data blob from a previos \p arnExport() can be imported.
     *  This is essentially assigning the _Arn Data Object_ with same as exported.
     *  \param[in] data is the data blob
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see arnExport()
     *  \see setIgnoreSameValue()
     */
    void  arnImport( const QByteArray& data, int ignoreSame = Arn::SameValue::DefaultAction)
    {ArnItemB::arnImport( data, ignoreSame);}

    /*! \return A data blob representing the _Arn Data Object_
     *  \see arnImport()
     */
    QByteArray  arnExport()  const
    {return ArnItemB::arnExport();}

    /*! \return Convert _Arn Data Object_ to a _integer_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    int  toInt( bool* isOk = 0)  const
    {return ArnItemB::toInt( isOk);}

    /*! \return Convert _Arn Data Object_ to a _double_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    double  toDouble( bool* isOk = 0)  const
    {return ArnItemB::toDouble( isOk);}

    /*! \return Convert _Arn Data Object_ to an _ARNREAL_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    ARNREAL  toReal( bool* isOk = 0)  const
    {return ArnItemB::toReal( isOk);}

    /*! \return Convert _Arn Data Object_ to a _bool_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    bool  toBool( bool* isOk = 0)  const
    {return ArnItemB::toBool( isOk);}

    /*! \return Convert _Arn Data Object_ to a _QString_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    QString  toString( bool* isOk = 0)  const
    {return ArnItemB::toString( isOk);}

    /*! \return Convert _Arn Data Object_ to a _QByteArray_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    QByteArray  toByteArray( bool* isOk = 0)  const
    {return ArnItemB::toByteArray( isOk);}

    /*! \return Convert _Arn Data Object_ to a _QVariant_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     */
    QVariant  toVariant( bool* isOk = 0)  const
    {return ArnItemB::toVariant( isOk);}

    /*! \return Convert _Arn Data Object_ to an _unsigned int_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from ByteArray.
     */
    uint  toUInt( bool* isOk = 0)  const
    {return ArnItemB::toUInt( isOk);}

    /*! \return Convert _Arn Data Object_ to an _int 64 bit_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from ByteArray.
     */
    qint64  toInt64( bool* isOk = 0)  const
    {return ArnItemB::toInt64( isOk);}

    /*! \return Convert _Arn Data Object_ to an _unsigned int 64 bit_
     *  \param[out] isOk If not 0 when a conversion error occurs, *isOk is set to false,
     *                   otherwise *isOk is set to true.
     *  \note Not native ARN datatype. It's converted from ByteArray.
     */
    quint64  toUInt64( bool* isOk = 0)  const
    {return ArnItemB::toUInt64( isOk);}

    ArnItem&  operator=( const ArnItem& other);
    ArnItem&  operator=( int val);
    ArnItem&  operator=( ARNREAL other);
    ArnItem&  operator=( const QString& val);
    ArnItem&  operator=( const QByteArray& val);
    ArnItem&  operator=( const QVariant& val);
    ArnItem&  operator=( const char* val);
    ArnItem&  operator=( uint val);
    ArnItem&  operator=( qint64 val);
    ArnItem&  operator=( quint64 val);

    ArnItem&  operator+=( int val);
    ArnItem&  operator+=( ARNREAL val);

    //! Assign the value of an other ArnItem to an _Arn Data Object_
    /*! \param[in] other is the ArnItem containing the value to assign
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const ArnItem& other, int ignoreSame = Arn::SameValue::DefaultAction)
    {ArnItemB::setValue( other, ignoreSame);}

    //! Assign an _unsigned int_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     *  \note Not native ARN datatype. ByteArray is assigned.
     */
    void  setValue( uint value, int ignoreSame = Arn::SameValue::DefaultAction)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign an _int 64 bit_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     *  \note Not native ARN datatype. ByteArray is assigned.
     */
    void  setValue( qint64 value, int ignoreSame = Arn::SameValue::DefaultAction)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign an _unsigned int 64 bit_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     *  \note Not native ARN datatype. ByteArray is assigned.
     */
    void  setValue( quint64 value, int ignoreSame = Arn::SameValue::DefaultAction)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign an _integer_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( int value, int ignoreSame)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign an _ARNREAL_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
#ifdef ARNREAL_FLOAT
    void  setValue( float value, int ignoreSame)
#else
    void  setValue( double value, int ignoreSame)
#endif
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign a _bool_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( bool value, int ignoreSame)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign a _QString_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QString& value, int ignoreSame)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign a _QByteArray_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QByteArray& value, int ignoreSame)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign a _QVariant_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QVariant& value, int ignoreSame)
    {ArnItemB::setValue( value, ignoreSame);}

    //! Assign a _char*_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setIgnoreSameValue()
     */
    void  setValue( const char* value, int ignoreSame)
    {ArnItemB::setValue( value, ignoreSame);}

    //! AtomicOp assign an _integer_ to specified bits in an _Arn Data Object_
    /*! Operation is done atomicly.
     *  If bidir, it can also be done remotely by an AtomicOpProvider
     *  \param[in] mask to specify bits that is affected
     *  \param[in] value to be assigned to affected bits
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see setAtomicOpProvider()
     *  \see setIgnoreSameValue()
     */
    void  setBits( int mask, int value, int ignoreSame = Arn::SameValue::DefaultAction)
    {ArnItemB::setBits( mask, value, ignoreSame);}

    //! AtomicOp adds an _integer_ to an _Arn Data Object_
    /*! Operation is done atomicly.
     *  If bidir, it can also be done remotely by an AtomicOpProvider
     *  \param[in] value to be added to this _Arn Data Object_
     *  \see setAtomicOpProvider()
     */
    void  addValue( int value)
    {ArnItemB::addValue( value);}

    //! AtomicOp adds an _ARNREAL_ to an _Arn Data Object_
    /*! Operation is done atomicly.
     *  If bidir, it can also be done remotely by an AtomicOpProvider
     *  \param[in] value to be added to this _Arn Data Object_
     *  \see setAtomicOpProvider()
     */
    void  addValue( ARNREAL value)
    {ArnItemB::addValue( value);}

public slots:
    //! Assign an _integer_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
    void  setValue( int value)
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Assign an _ARNREAL_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
#ifdef ARNREAL_FLOAT
    void  setValue( float value)
#else
    void  setValue( double value)
#endif
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Assign a _bool_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
    void  setValue( bool value)
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Assign a _QString_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QString& value)
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Assign a _QByteArray_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QByteArray& value)
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Assign a _QVariant_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
    void  setValue( const QVariant& value)
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Assign a _char*_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     *  \see setIgnoreSameValue()
     */
    void  setValue( const char* value)
    {ArnItemB::setValue( value, Arn::SameValue::DefaultAction);}

    //! Toggle the _bool_ at the _Arn Data Object_
    /*! The _Arn Data Object_ is first converted to a _bool_, then the toggled value
     *  is assigned back to the _Arn Data Object_.
     */
    void  toggleBool();

signals:
    //! Signals emitted when data in _Arn Data Object_ is changed.
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
#ifdef ARNREAL_FLOAT
    void  changed( float value);
#else
    void  changed( double value);
#endif

    /*! \see changed()
     */
    void  changed( bool value);

    /*! \see changed()
     */
    void  changed( const QString& value);

    /*! \see changed()
     */
    void  changed( const QByteArray& value);

    /*! \see changed()
     */
    void  changed( const QVariant& value);

    //! Signal emitted when mode in _Arn Data Object_ is changed.
    /*! Object changing _general mode_ will give this signal.
     *  \param[in] mode is the new _general mode_
     *  \see \ref gen_arnobjModes
     */
    void  modeChanged( Arn::ObjectMode mode);

    //! Signal emitted when an _Arn Data Object_ is created in the tree below.
    /*! The ArnItem is a folder. Created objects in this folder or its children
     *  will give this signal.
     *  Only created non folder objects will give this signal.
     *  \param[in] path to the created _Arn Data Object_
     *  \deprecated use ArnMonitor instead.
     */
    void  arnItemCreated( const QString& path);

    //! Signal emitted when an _Arn Data Object_ in the tree below has a _general mode_ change.
    /*! The ArnItem is a folder. Objects changing _general mode_ in this folder or
     *  its children will give this signal.
     *  \param[in] path to the _general mode_ changing _Arn Data Object_
     *  \param[in] linkId for the _general mode_ changing _Arn Data Object_
     *  \param[in] mode is the new _general mode_
     *  \see linkId()
     *  \see \ref gen_arnobjModes
     *  \deprecated use ArnMonitor instead.
     */
    void  arnModeChanged( const QString& path, uint linkId, Arn::ObjectMode mode);

    //! \cond ADV
protected:
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  modeUpdate( Arn::ObjectMode mode, bool isSetup = false);
    virtual void  itemCreatedBelow( const QString& path);
    virtual void  itemModeChangedBelow( const QString& path, uint linkId, Arn::ObjectMode mode);
    virtual void  timerEvent( QTimerEvent* ev);

    int  delayTimerId()  const;

    ArnItem( ArnItemPrivate& dd, QObject* parent);
    //! \endcond

private slots:

private:
    void  init();
    void  doItemUpdate( const ArnLinkHandle& handleData);

#if QT_VERSION >= 0x050000
    void  connectNotify( const QMetaMethod & signal);
    void  disconnectNotify( const QMetaMethod & signal);
    static QMetaMethod  _metaSignalChanged;
    static QMetaMethod  _metaSignalChangedInt;
    static QMetaMethod  _metaSignalChangedReal;
    static QMetaMethod  _metaSignalChangedBool;
    static QMetaMethod  _metaSignalChangedString;
    static QMetaMethod  _metaSignalChangedByteArray;
    static QMetaMethod  _metaSignalChangedVariant;
#else
    void  connectNotify( const char* signal);
    void  disconnectNotify( const char* signal);
#endif
};

QTextStream&  operator<<(QTextStream& out, const ArnItem& item);

#endif // ARNITEM_HPP
