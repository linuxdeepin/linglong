/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <DSingleton>

#include "module/package/package.h"
#include "package_manager_proxy_base.h"
#include "qdbus_retmsg.h"

class PackageManagerFlatpakImpl
    : public PackageManagerProxyBase
    , public Dtk::Core::DSingleton<PackageManagerFlatpakImpl>
{
public:
    RetMessageList Download(const QStringList &packageIDList, const QString &savePath) { return {}; }
    RetMessageList Install(const QStringList &packageIDList, const ParamStringMap &paramMap = {});
    RetMessageList Uninstall(const QStringList &packageIDList, const ParamStringMap &paramMap = {});
    AppMetaInfoList Query(const QStringList &packageIDList, const ParamStringMap &paramMap = {});
};