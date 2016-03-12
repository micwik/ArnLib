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

#ifndef ARNITEMB_HPP
#define ARNITEMB_HPP

#include "ArnLib_global.hpp"
#include "ArnLinkHandle.hpp"
#include "ArnError.hpp"
#include "Arn.hpp"
#include "ArnBasicItem.hpp"
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariant>

class ArnItemBPrivate;


//! Base class handle for an _Arn Data Object_.
/*!
[About Arn Data Object](\ref gen_arnobj)

This class contains the basic services, that should be apropriate for any derived class
as public methods. Other non generic services that might be needed is available as
protected methods. Typically derived classes can select among these protected methods and
make any of them public.

See ArnItem.
*/
class ARNLIBSHARED_EXPORT ArnItemB : public QObject, public ArnBasicItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnItemB)

public:
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

    //! Import data to an _Arn Data Object_
    /*! Data blob from a previos \p arnExport() can be imported.
     *  This is essentially assigning the _Arn Data Object_ with same as exported.
     *  \param[in] data is the data blob
     *  \param[in] ignoreSame can override default ignoreSameValue setting.
     *  \see arnExport()
     *  \see setIgnoreSameValue()
     */
    void  arnImport( const QByteArray& data, int ignoreSame = Arn::SameValue::DefaultAction);

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

    //// To be reimplemented
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  modeUpdate( Arn::ObjectMode mode, bool isSetup = false);
    virtual void  itemCreatedBelow( const QString& path);
    virtual void  itemModeChangedBelow( const QString& path, uint linkId, Arn::ObjectMode mode);

    virtual void  arnEvent( QEvent* ev, bool isAlienThread);
    virtual void  customEvent( QEvent* ev);

    //// Methods not to be public
    bool  openWithFlags( const QString& path, Arn::LinkFlags linkFlags);
    void  setBlockEcho( bool blockEcho);
    void  setEnableSetValue( bool enable);
    void  setEnableUpdNotify( bool enable);
    void  setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData);
    void  arnImport( const QByteArray& data, int ignoreSame, ArnLinkHandle& handleData);

    ArnItemB( ArnItemBPrivate& dd, QObject* parent);
    ArnItemBPrivate* const  d_ptr;

    using ArnBasicItem::isFolder;
    using ArnBasicItem::isProvider;
    using ArnBasicItem::type;
    using ArnBasicItem::setIgnoreSameValue;
    using ArnBasicItem::addMode;
    using ArnBasicItem::getMode;
    using ArnBasicItem::syncMode;
    using ArnBasicItem::setBiDirMode;
    using ArnBasicItem::isBiDirMode;
    using ArnBasicItem::setPipeMode;
    using ArnBasicItem::isPipeMode;
    using ArnBasicItem::setSaveMode;
    using ArnBasicItem::isSaveMode;
    using ArnBasicItem::setMaster;
    using ArnBasicItem::isMaster;
    using ArnBasicItem::setAutoDestroy;
    using ArnBasicItem::isAutoDestroy;
    using ArnBasicItem::arnExport;
    using ArnBasicItem::toInt;
    using ArnBasicItem::toDouble;
    using ArnBasicItem::toReal;
    using ArnBasicItem::toBool;
    using ArnBasicItem::toByteArray;
    using ArnBasicItem::toString;
    using ArnBasicItem::toVariant;
    //! \endcond

private:
    void  init();
    inline void  doEvent( QEvent* ev);
};

#endif // ARNITEMB_HPP
