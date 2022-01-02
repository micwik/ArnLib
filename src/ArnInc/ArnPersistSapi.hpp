// Copyright (C) 2010-2022 Michael Wiklund.
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

#ifndef ARNPERSISTSAPI_HPP
#define ARNPERSISTSAPI_HPP

#include "ArnSapi.hpp"


//! \cond ADV

/// Persist Service API
class ArnPersistSapi : public ArnSapi
{
    Q_OBJECT
public:
    explicit ArnPersistSapi( QObject* parent = 0) : ArnSapi("//.sys/Persist/Pipes/CommonPipe", parent)  {}

signals:
MQ_PUBLIC_ACCESS
    //// Provider API
    void  pv_vcsLog( int numLog = 100);
    void  pv_vcsFiles( QString ref = QString());
    void  pv_vcsCommit( QString commitMsg,
                         QStringList files = QStringList(),
                         QString name = QString(), QString email = QString());
    void  pv_vcsTag( QString name, QString ref = QString());
    void  pv_vcsDiff( QString ref = QString(), QStringList files = QStringList());
    void  pv_vcsLogRecord( QString ref = QString());
    void  pv_vcsTags();
    void  pv_vcsBranches();
    void  pv_vcsStatus();
    void  pv_vcsUserSettings();
    void  pv_vcsSetUserSettings( QString userName, QString userEmail);
    void  pv_vcsCheckout( QString ref = QString(), QStringList files = QStringList());
    void  pv_flush( const QString& path = QString());
    void  pv_test( QString str = "Hello", int i=10);
    void  pv_ls( QString path = QString());
    void  pv_load();
    void  pv_rm( QString path);
    void  pv_touch( QString path);
    void  pv_dbMandatory( QString path, bool isMandatory);
    void  pv_dbMandatoryLs( QString path);
    void  pv_dbLs( QString path, bool isUsed = true);
    void  pv_dbMarkUnused( QString path);
    void  pv_info();

    //// Requester API
    void  rq_flushR( bool isOk, const QString& path);
    void  rq_lsR( QStringList files);
    void  rq_rmR( bool isOk);
    void  rq_touchR( bool isOk);
    void  rq_dbMandatoryR( bool isOk);
    void  rq_dbMandatoryLsR( QStringList paths);
    void  rq_dbLsR( QStringList paths);
    void  rq_dbMarkUnusedR( bool isOk);
    void  rq_infoR( QString name, QString ver);
    void  rq_vcsNotify( QString msg);
    void  rq_vcsProgress( int percent, QString msg=QString());
    void  rq_vcsUserSettingsR( QString name, QString eMail);
    void  rq_vcsFilesR( QStringList files);
    void  rq_vcsTagR();
    void  rq_vcsCommitR();
    void  rq_vcsCheckoutR();
    void  rq_vcsBranchesR( QStringList branches);
    void  rq_vcsTagsR( QStringList tags);
    void  rq_vcsStatusR( QString status);
    void  rq_vcsDiffR( QString txt);
    void  rq_vcsLogRecordR( QString txt);
    void  rq_vcsLogR( QStringList refs, QStringList msgs);
};
//! \endcond


#endif // ARNPERSISTSAPI_HPP
