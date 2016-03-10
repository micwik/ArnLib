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

#ifndef ARNITEMVALVE_HPP
#define ARNITEMVALVE_HPP

#include "ArnLib_global.hpp"
#include "ArnItemB.hpp"

class ArnItemValvePrivate;


//! Valve for controlling stream to/from an ArnItemB.
/*!
[About Arn Data Object](\ref gen_arnobj)

This valve class can control data stream to/from any ArnItemB derived class.
The class itself is derived from ArnItemB, so it could also be controlled by another
ArnItemValve. But most importent, it has a subset of ArnItem's methods to make it shareable
in the ARN tree.

ArnItemValve can be used "standalone", i.e. not beeing opened to the ARN tree. In this case
it is used by its setValue method and locally emits its changed() signal.

When opened to the ARN tree it can be used by its setValue method and it can also be remote
controlled as any other ArnItem. If locally set, this will as usual be reflected in the
ARN tree.

It's possible to use one ArnItemValve for controling _InStream_ and another for controlling
_OutStream_. The valve for each stream direction can then be set independently. The default
is using one valve for both stream directions.

This class is not thread-safe, but the _Arn Data object_ is, so this valve can be remote
controlled by an ArnItem.

<b>Example usage</b> \n \code
    // In class code
    _commonSapi = new ChatSapi( this);
    _commonSapi->open("//Chat/Pipes/pipeCommon", ArnSapi::Mode::Provider);
    _commonSapi->batchConnectTo( this, "sapi");

    // Control message flow to and from service api _commonSapi
    ArnItemValve*  arnValve = new ArnItemValve( this);
    arnValve->setTarget( _commonSapi->pipe());
    arnValve->open("//Chat/Valves/pipeCommon");
    *arnValve = true;  // Set valve open for message flow
\endcode
*/
class ARNLIBSHARED_EXPORT ArnItemValve : public ArnItemB
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnItemValve)

public:
    struct SwitchMode {
        enum E {
            //! Control target item notifying (signal) updated value
            InStream    = 0x01,
            //! Control target item accepting assign of value (setValue)
            OutStream   = 0x02,
            //! Convenience, combined _InStream_ and _OutStream_
            InOutStream = InStream | OutStream
        };
        MQ_DECLARE_FLAGS( SwitchMode)
    };

    explicit ArnItemValve( QObject* parent = 0);

    bool  setTarget( ArnItemB* targetItem, SwitchMode mode = SwitchMode::InOutStream);

    SwitchMode  switchMode()  const;

    //! Set _general mode_ as _Save_ for this _Arn Data Object_
    /*! Data is persistent and will be saved
     *  \pre The persistent service must be started at the server.
     *  \see \ref gen_arnobjModes
     *  \see \ref gen_persistArnobj
     */
    ArnItemValve&  setSaveMode()
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
    ArnItemValve&  setMaster()
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
    ArnItemValve&  setAutoDestroy()
    {ArnItemB::setAutoDestroy(); return *this;}

    /*! \retval true if _AutoDestroy mode_
     *  \see setAutoDestroy()
     */
    bool  isAutoDestroy()  const
    {return ArnItemB::isAutoDestroy();}

    /*! \return state of this valve 1 = Enabled selected stream(s)
     */
    bool  toBool()  const;

    ArnItemValve&  operator=( bool value);


public slots:
    //! Assign a _bool_ to an _Arn Data Object_
    /*! \param[in] value to be assigned
     */
    void  setValue( bool value);

signals:
    /*! Signals emitted when data in _Arn Data Object_ is changed.
     */
    void  changed( int value);

    //! \cond ADV
protected:
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);

    ArnItemValve( ArnItemValvePrivate& dd, QObject* parent);
    //! \endcond

private:
    void  doControl();
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnItemValve::SwitchMode)

#endif // ARNITEMVALVE_HPP
