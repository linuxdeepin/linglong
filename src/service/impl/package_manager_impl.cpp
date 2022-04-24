/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong@uniontech.com
 *
 * Maintainer: huqinghong@uniontech.com
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "module/repo/repohelper_factory.h"
#include "module/repo/ostree_repohelper.h"
#include "module/util/app_status.h"
#include "module/util/appinfo_cache.h"
#include "module/util/httpclient.h"
#include "module/util/package_manager_param.h"
#include "module/util/sysinfo.h"
#include "module/util/runner.h"

#include "package_manager_impl.h"
#include "dbus_retcode.h"
#include "version.h"

using linglong::dbus::RetCode;
using namespace linglong::Status;
using namespace linglong::util;

const QString kAppInstallPath = "/deepin/linglong/layers/";
const QString kLocalRepoPath = "/deepin/linglong/repo";
const QString kRemoteRepoName = "repo";
/*
 * 将json字符串转化为软件包数据
 *
 * @param jsonString: 软件包对应的json字符串
 * @param appList: 转换结果
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerImpl::loadAppInfo(const QString &jsonString, linglong::package::AppMetaInfoList &appList,
                                     QString &err)
{
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(jsonString.toUtf8(), &parseJsonErr);
    if (QJsonParseError::NoError != parseJsonErr.error) {
        err = "parse json data err";
        return false;
    }

    QJsonObject jsonObject = document.object();
    if (jsonObject.size() == 0) {
        err = "receive data is empty";
        return false;
    }

    if (!jsonObject.contains("code") || !jsonObject.contains("data")) {
        err = "receive data format err";
        return false;
    }

    int code = jsonObject["code"].toInt();
    if (code != 0) {
        err = "app not found in repo";
        qCritical().noquote() << jsonString;
        return false;
    }

    QJsonValue arrayValue = jsonObject.value(QStringLiteral("data"));
    if (!arrayValue.isArray()) {
        err = "jsonString from server data format is not array";
        qCritical().noquote() << jsonString;
        return false;
    }

    // 多个结果以json 数组形式返回
    QJsonArray arr = arrayValue.toArray();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject dataObj = arr.at(i).toObject();
        const QString jsonString = QString(QJsonDocument(dataObj).toJson(QJsonDocument::Compact));
        // qInfo().noquote() << jsonString;
        auto appItem = linglong::util::loadJSONString<linglong::package::AppMetaInfo>(jsonString);
        appList.push_back(appItem);
    }
    return true;
}

/*
 * 从服务器查询指定包名/版本/架构的软件包数据
 *
 * @param pkgName: 软件包包名
 * @param pkgVer: 软件包版本号
 * @param pkgArch: 软件包对应的架构
 * @param appData: 查询结果
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerImpl::getAppInfofromServer(const QString &pkgName, const QString &pkgVer, const QString &pkgArch,
                                              QString &appData, QString &err)
{
    bool ret = G_HTTPCLIENT->queryRemote(pkgName, pkgVer, pkgArch, appData);
    if (!ret) {
        err = "getAppInfofromServer err";
        qCritical() << err;
        return false;
    }

    qDebug().noquote() << appData;
    return true;
}

/*
 * 将在线包数据部分签出到指定目录
 *
 * @param pkgName: 软件包包名
 * @param pkgVer: 软件包版本号
 * @param pkgArch: 软件包对应的架构
 * @param dstPath: 在线包数据部分存储路径
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerImpl::downloadAppData(const QString &pkgName, const QString &pkgVer, const QString &pkgArch,
                                         const QString &dstPath, QString &err)
{
    bool ret = G_OSTREE_REPOHELPER->ensureRepoEnv(kLocalRepoPath, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }

    // ref format --> app/org.deepin.calculator/1.2.2/x86_64
    QString matchRef = QString("%1/%2/%3").arg(pkgName).arg(pkgVer).arg(pkgArch);
    qInfo() << "downloadAppData ref:" << matchRef;

    // ret = repo.repoPull(repoPath, qrepoList[0], pkgName, err);
    ret = G_OSTREE_REPOHELPER->repoPullbyCmd(kLocalRepoPath, kRemoteRepoName, matchRef, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }
    // checkout 目录
    // const QString dstPath = repoPath + "/AppData";
    ret = G_OSTREE_REPOHELPER->checkOutAppData(kLocalRepoPath, kRemoteRepoName, matchRef, dstPath, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }
    qInfo() << "downloadAppData success, path:" << dstPath;

    return ret;
}

linglong::service::Reply PackageManagerImpl::Download(const linglong::service::DownloadParamOption &downloadParamOption)
{
    qInfo() << downloadParamOption.appId << downloadParamOption.version << downloadParamOption.arch;
    linglong::service::Reply reply;
    reply.code = 0;
    reply.message = downloadParamOption.appId;
    return reply;
}

/*
 * 安装应用runtime
 *
 * @param runtimeId: runtime对应的appId
 * @param runtimeVer: runtime版本号
 * @param runtimeArch: runtime对应的架构
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerImpl::installRuntime(const QString &runtimeId, const QString &runtimeVer, const QString &runtimeArch,
                                        QString &err)
{
    linglong::package::AppMetaInfoList appList;
    QString appData = "";

    bool ret = getAppInfofromServer(runtimeId, runtimeVer, runtimeArch, appData, err);
    if (!ret) {
        return false;
    }
    ret = loadAppInfo(appData, appList, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }
    // app runtime 只能匹配一个
    if (appList.size() != 1) {
        err = "app:" + runtimeId + ", version:" + runtimeVer + " not found in repo";
        return false;
    }

    auto pkgInfo = appList.at(0);
    const QString savePath = kAppInstallPath + runtimeId + "/" + runtimeVer + "/" + runtimeArch;
    // 创建路径
    linglong::util::createDir(savePath);
    ret = downloadAppData(runtimeId, runtimeVer, runtimeArch, savePath, err);
    if (!ret) {
        err = "installRuntime download runtime data err";
        return false;
    }

    // 更新本地数据库文件
    QString userName = getUserName();
    pkgInfo->kind = "runtime";
    insertAppRecord(pkgInfo, "user", userName);
    return true;
}

/*
 * 检查应用runtime安装状态
 *
 * @param runtime: 应用runtime字符串
 * @param err: 错误信息
 *
 * @return bool: true:安装成功或已安装返回true false:安装失败
 */
bool PackageManagerImpl::checkAppRuntime(const QString &runtime, QString &err)
{
    // runtime ref in repo org.deepin.Runtime/20/x86_64
    QStringList runtimeInfo = runtime.split("/");
    if (runtimeInfo.size() != 3) {
        err = "app runtime:" + runtime + " runtime format err";
        return false;
    }
    const QString runtimeId = runtimeInfo.at(0);
    const QString runtimeVer = runtimeInfo.at(1);
    const QString runtimeArch = runtimeInfo.at(2);

    bool ret = true;
    // 判断app依赖的runtime是否安装
    QString userName = getUserName();
    if (!getAppInstalledStatus(runtimeId, runtimeVer, "", userName)) {
        ret = installRuntime(runtimeId, runtimeVer, runtimeArch, err);
    }
    return ret;
}

/*
 * 从给定的软件包列表中查找最新版本的软件包
 *
 * @param appList: 待搜索的软件包列表信息
 *
 * @return AppMetaInfo: 最新版本的软件包
 *
 */
linglong::package::AppMetaInfo *PackageManagerImpl::getLatestApp(const linglong::package::AppMetaInfoList &appList)
{
    linglong::package::AppMetaInfo *latestApp = appList.at(0);
    if (appList.size() == 1) {
        return latestApp;
    }

    QString curVersion = latestApp->version;
    QString arch = hostArch();
    for (auto item : appList) {
        linglong::AppVersion dstVersion(curVersion);
        linglong::AppVersion iterVersion(item->version);
        if (arch == item->arch && iterVersion.isBigThan(dstVersion)) {
            curVersion = item->version;
            latestApp = item;
        }
    }
    return latestApp;
}

linglong::service::Reply PackageManagerImpl::Install(const linglong::service::InstallParamOption &installParamOption)
{
    linglong::service::Reply reply;

    QString userName = getUserName();
    QString appData = "";
    // 安装不查缓存
    auto ret = getAppInfofromServer(installParamOption.appId, installParamOption.version, installParamOption.arch,
                                    appData, reply.message);
    if (!ret) {
        reply.code = RetCode(RetCode::pkg_install_failed);
        return reply;
    }

    linglong::package::AppMetaInfoList appList;
    ret = loadAppInfo(appData, appList, reply.message);
    if (!ret) {
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::pkg_install_failed);
        return reply;
    }

    if (appList.size() < 1) {
        reply.message =
            "appId:" + installParamOption.appId + ", version:" + installParamOption.version + " not found in repo";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::pkg_install_failed);
        return reply;
    }

    // 查找最高版本
    linglong::package::AppMetaInfo *appInfo = getLatestApp(appList);
    // 判断对应版本的应用是否已安装 Fix to do 多用户
    if (getAppInstalledStatus(appInfo->appId, appInfo->version, "", "")) {
        reply.code = RetCode(RetCode::pkg_already_installed);
        reply.message = "appId:" + appInfo->appId + ", version: " + appInfo->version + " already installed";
        qCritical() << reply.message;
        return reply;
    }

    // 检查软件包依赖的runtime安装状态
    ret = checkAppRuntime(appInfo->runtime, reply.message);
    if (!ret) {
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::install_runtime_failed);
        return reply;
    }

    // 下载在线包数据到目标目录 安装完成
    // QString pkgName = "org.deepin.calculator";
    const QString savePath = kAppInstallPath + appInfo->appId + "/" + appInfo->version + "/" + appInfo->arch;
    ret = downloadAppData(appInfo->appId, appInfo->version, appInfo->arch, savePath, reply.message);
    if (!ret) {
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::load_pkg_data_failed);
        return reply;
    }

    // 链接应用配置文件到系统配置目录
    if (linglong::util::dirExists(savePath + "/outputs/share")) {
        const QString appEntriesDirPath = savePath + "/outputs/share";
        linglong::util::linkDirFiles(appEntriesDirPath, sysLinglongInstalltions);
    } else {
        const QString appEntriesDirPath = savePath + "/entries";
        linglong::util::linkDirFiles(appEntriesDirPath, sysLinglongInstalltions);
    }

    // 更新desktop database
    auto retRunner = linglong::runner::Runner("update-desktop-database", {sysLinglongInstalltions + "/applications/"},
                                              1000 * 60 * 1);
    if (!retRunner) {
        qWarning() << "warning: update desktop database of " + sysLinglongInstalltions + "/applications/ failed!";
    }

    // 更新mime type database
    if (linglong::util::dirExists(sysLinglongInstalltions + "/mime/packages")) {
        auto retUpdateMime =
            linglong::runner::Runner("update-mime-database", {sysLinglongInstalltions + "/mime/"}, 1000 * 60 * 1);
        if (!retUpdateMime) {
            qWarning() << "warning: update mime type database of " + sysLinglongInstalltions + "/mime/ failed!";
        }
    }

    // 更新本地数据库文件
    appInfo->kind = "app";
    insertAppRecord(appInfo, "user", userName);

    reply.code = RetCode(RetCode::pkg_install_success);
    reply.message = "install appId:" + appInfo->appId + ", version:" + appInfo->version + " success";
    qInfo() << reply.message;

    return reply;
}

/*!
 * 查询软件包
 * @param packageIdList: 软件包的appId
 *
 * @return linglong::package::AppMetaInfoList 查询结果列表
 */
linglong::package::AppMetaInfoList PackageManagerImpl::Query(const QStringList &packageIdList,
                                                             const ParamStringMap &paramMap)
{
    const QString pkgName = packageIdList.at(0).trimmed();
    if (pkgName == "installed") {
        return queryAllInstalledApp();
    }

    linglong::package::AppMetaInfoList pkgList;
    QString arch = hostArch();
    if (arch == "unknown") {
        qCritical() << "the host arch is not recognized";
        return pkgList;
    }

    bool ret = false;
    QString err = "";
    QString appData = "";
    int status = StatusCode::FAIL;

    if (!paramMap.contains(linglong::util::kKeyNoCache)) {
        status = queryLocalCache(pkgName, appData);
    }

    bool fromServer = false;
    // 缓存查不到从服务器查
    if (status != StatusCode::SUCCESS) {
        ret = getAppInfofromServer(pkgName, "", arch, appData, err);
        if (!ret) {
            qCritical() << err;
            return pkgList;
        }
        fromServer = true;
    }
    ret = loadAppInfo(appData, pkgList, err);
    qInfo().noquote() << appData;
    if (!ret) {
        qCritical() << err;
        return pkgList;
    }
    // 若从服务器查询得到正确的数据则更新缓存
    if (fromServer) {
        updateCache(pkgName, appData);
    }
    return pkgList;
}

/*
 * 卸载软件包
 *
 * @param packageIdList: 软件包的appId
 *
 * @return RetMessageList 卸载结果信息
 */
RetMessageList PackageManagerImpl::Uninstall(const QStringList &packageIdList, const ParamStringMap &paramMap)
{
    RetMessageList retMsg;
    auto info = QPointer<RetMessage>(new RetMessage);
    const QString pkgName = packageIdList.at(0).trimmed();

    // 获取版本信息
    QString version = "";
    if (!paramMap.empty() && paramMap.contains(linglong::util::kKeyVersion)) {
        version = paramMap[linglong::util::kKeyVersion];
    }

    QString arch = hostArch();
    // 判断是否已安装 不校验用户名 普通用户无法卸载预装应用 提示信息不对
    QString userName = getUserName();
    if (!getAppInstalledStatus(pkgName, version, arch, "")) {
        qCritical() << pkgName << " not installed";
        info->setcode(RetCode(RetCode::pkg_not_installed));
        info->setmessage(pkgName + " not installed");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    QString err = "";
    linglong::package::AppMetaInfoList pkgList;
    // 根据已安装文件查询已经安装软件包信息
    getInstalledAppInfo(pkgName, version, arch, "", pkgList);
    auto it = pkgList.at(0);
    bool isRoot = (getgid() == 0) ? true : false;
    qInfo() << "install app user:" << it->user << ", current user:" << userName << ", has root permission:" << isRoot;
    // 非root用户卸载不属于该用户安装的应用
    if (userName != it->user && !isRoot) {
        qCritical() << pkgName << " uninstall permission deny";
        info->setcode(RetCode(RetCode::pkg_uninstall_failed));
        info->setmessage(pkgName + " uninstall permission deny");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }

    const QString installPath = kAppInstallPath + it->appId + "/" + it->version;
    // 删掉安装配置链接文件
    if (linglong::util::dirExists(installPath + "/" + arch + "/outputs/share")) {
        const QString appEntriesDirPath = installPath + "/" + arch + "/outputs/share";
        linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
    } else {
        const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
        linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
    }
    linglong::util::removeDir(installPath);
    qInfo() << "Uninstall del dir:" << installPath;

    // 更新desktop database
    auto retRunner = linglong::runner::Runner("update-desktop-database", {sysLinglongInstalltions + "/applications/"},
                                              1000 * 60 * 1);
    if (!retRunner) {
        qWarning() << "warning: update desktop database of " + sysLinglongInstalltions + "/applications/ failed!";
    }

    // 更新mime type database
    if (linglong::util::dirExists(sysLinglongInstalltions + "/mime/packages")) {
        auto retUpdateMime =
            linglong::runner::Runner("update-mime-database", {sysLinglongInstalltions + "/mime/"}, 1000 * 60 * 1);
        if (!retUpdateMime) {
            qWarning() << "warning: update mime type database of " + sysLinglongInstalltions + "/mime/ failed!";
        }
    }

    // 更新本地repo仓库
    bool ret = G_OSTREE_REPOHELPER->ensureRepoEnv(kLocalRepoPath, err);
    if (!ret) {
        qCritical() << err;
        info->setcode(RetCode(RetCode::pkg_uninstall_failed));
        info->setmessage("uninstall local repo not exist");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    // 应从安装数据库获取应用所属仓库信息 to do fix
    QVector<QString> qrepoList;
    ret = G_OSTREE_REPOHELPER->getRemoteRepoList(kLocalRepoPath, qrepoList, err);
    if (!ret) {
        qCritical() << err;
        info->setcode(RetCode(RetCode::pkg_uninstall_failed));
        info->setmessage("uninstall remote repo not exist");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }

    // new ref format org.deepin.calculator/1.2.2/x86_64
    // QString matchRef = QString("app/%1/%2/%3").arg(it->appId).arg(arch).arg(it->version);
    QString matchRef = QString("%1/%2/%3").arg(it->appId).arg(it->version).arg(arch);
    qInfo() << "Uninstall app ref:" << matchRef;
    ret = G_OSTREE_REPOHELPER->repoDeleteDatabyRef(kLocalRepoPath, qrepoList[0], matchRef, err);
    if (!ret) {
        qCritical() << err;
        info->setcode(RetCode(RetCode::pkg_uninstall_failed));
        info->setmessage("uninstall " + pkgName + ", version:" + it->version + " failed");
        info->setstate(false);
        retMsg.push_back(info);
        return retMsg;
    }
    // A 用户 sudo 卸载 B 用户安装的软件
    if (isRoot) {
        userName = "";
    }
    // 更新安装数据库
    deleteAppRecord(pkgName, it->version, arch, userName);
    info->setcode(RetCode(RetCode::pkg_uninstall_success));
    info->setmessage("uninstall " + pkgName + ", version:" + it->version + " success");
    info->setstate(true);
    retMsg.push_back(info);
    return retMsg;
}

linglong::service::Reply PackageManagerImpl::Update(linglong::service::ParamOption paramOption)
{
    linglong::service::Reply reply;
    paramOption.arch = hostArch();
    qDebug() << "paramOption.arch:" << paramOption.arch;

    // 判断是否已安装
    QString userName = getUserName();
    if (!getAppInstalledStatus(paramOption.appId, paramOption.version, paramOption.arch, userName)) {
        reply.message = paramOption.appId + " not installed";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::pkg_not_installed);
        return reply;
    }

    // 检查是否存在版本更新
    linglong::package::AppMetaInfoList pkgList;
    // 根据已安装文件查询已经安装软件包信息
    getInstalledAppInfo(paramOption.appId, paramOption.version, paramOption.arch, userName, pkgList);
    auto installedApp = pkgList.at(0);
    if (pkgList.size() != 1) {
        reply.message = "query local app:" + paramOption.appId + " info err";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::ErrorPkgUpdateFailed);
        return reply;
    }

    QString currentVersion = installedApp->version;
    QString appData = QString();
    qDebug() << paramOption.appId << paramOption.arch;
    auto ret = getAppInfofromServer(paramOption.appId, "", paramOption.arch, appData, reply.message);
    if (!ret) {
        reply.message = "query server app:" + paramOption.appId + " info err";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::ErrorPkgUpdateFailed);
        return reply;
    }

    linglong::package::AppMetaInfoList serverPkgList;
    ret = loadAppInfo(appData, serverPkgList, reply.message);
    if (!ret) {
        reply.message = "load app:" + paramOption.appId + " info err";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::ErrorPkgUpdateFailed);
        return reply;
    }

    auto serverApp = getLatestApp(serverPkgList);
    if (currentVersion == serverApp->version) {
        reply.message = "app:" + paramOption.appId + ", version:" + currentVersion + " is latest";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::ErrorPkgUpdateFailed);
        return reply;
    }

    // FIXME 安装最新软件
    linglong::service::InstallParamOption installParamOption;
    installParamOption.appId = paramOption.appId;
    installParamOption.version = serverApp->version;
    installParamOption.arch = paramOption.arch;
    reply = Install({installParamOption});
    if (!reply.code) {
        reply.message = "install app:" + paramOption.appId + " err";
        qCritical() << reply.message;
        reply.code = RetCode(RetCode::ErrorPkgUpdateFailed);
        return reply;
    }

    QMap<QString, QString> uninstallParamMap;
    uninstallParamMap.insert(linglong::util::kKeyVersion, currentVersion);
    RetMessageList uninstallRet = Uninstall({paramOption.appId}, uninstallParamMap);
    if (uninstallRet.size() > 0) {
        auto it = uninstallRet.at(0);
        if (!it->state) {
            reply.message = "uninstall app:" + paramOption.appId + " err";
            qCritical() << reply.message;
            reply.code = RetCode(RetCode::ErrorPkgUpdateFailed);
            return reply;
        }
    }
    reply.code = RetCode(RetCode::ErrorPkgUpdateSuccess);
    reply.message =
        "update " + paramOption.appId + " success, version:" + currentVersion + " --> " + serverApp->version;
    return reply;
}