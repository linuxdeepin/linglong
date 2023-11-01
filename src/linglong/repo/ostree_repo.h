/*
 * SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_REPO_OSTREE_REPO_H_
#define LINGLONG_SRC_MODULE_REPO_OSTREE_REPO_H_

#include "linglong/package/package.h"
#include "linglong/repo/repo.h"
#include "linglong/repo/repo_client.h"
#include "linglong/repo/repo_command.h"
#include "linglong/util/config/config.h"
#include "linglong/util/erofs.h"
#include "linglong/util/file.h"
#include "linglong/util/http/http_client.h"
#include "linglong/util/qserializer/json.h"
#include "linglong/utils/error/error.h"

#include <ostree-repo.h>

#include <QHttpPart>
#include <QPointer>
#include <QScopedPointer>
#include <QThread>

namespace linglong {
namespace repo {

class InfoResponse : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(InfoResponse)

    Q_JSON_PROPERTY(int, code);
    Q_JSON_PROPERTY(QString, msg);
    Q_JSON_PROPERTY(ParamStringMap, revs);
};

class RevPair : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(RevPair)

    Q_JSON_PROPERTY(QString, server);
    Q_JSON_PROPERTY(QString, client);
};

class UploadResponseData : public Serialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(UploadResponseData)

    Q_JSON_PROPERTY(QString, id);
    Q_JSON_PROPERTY(QString, watchId);
    Q_JSON_PROPERTY(QString, status);
};

} // namespace repo
} // namespace linglong

Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, InfoResponse)
Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, RevPair)
Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, UploadResponseData)

namespace linglong {
namespace repo {

class UploadTaskRequest : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(UploadTaskRequest)

    Q_JSON_PROPERTY(int, code);
    Q_JSON_PROPERTY(QStringList, objects);
    Q_PROPERTY(QMap<QString, QSharedPointer<linglong::repo::RevPair>> refs MEMBER refs);
    QMap<QString, QSharedPointer<linglong::repo::RevPair>> refs;
};

class UploadRequest : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(UploadRequest)

    Q_JSON_PROPERTY(QString, repoName);
    Q_JSON_PROPERTY(QString, ref);
};

class UploadTaskResponse : public Serialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(UploadTaskResponse)

    Q_JSON_PROPERTY(int, code);
    Q_JSON_PROPERTY(QString, msg);
    Q_JSON_PTR_PROPERTY(linglong::repo::UploadResponseData, data);
};

} // namespace repo
} // namespace linglong

Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, UploadRequest)
Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, UploadTaskRequest)
Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, UploadTaskResponse)

namespace linglong {
namespace repo {
// ostree 仓库对象信息
struct LingLongDir
{
    QString basedir;
    OstreeRepo *repo;
};

struct OstreeRepoObject
{
    QString objectName;
    QString rev;
    QString path;
};

class OSTreeRepo : public QObject, public Repo
{
    Q_OBJECT
public:
    explicit OSTreeRepo(const QString &path);
    explicit OSTreeRepo(const QString &localRepoPath,
                        const QString &remoteEndpoint,
                        const QString &remoteRepoName);

    ~OSTreeRepo() override;
    linglong::utils::error::Result<void> init(const QString &repoMode) override;

    linglong::utils::error::Result<void> remoteAdd(const QString &repoName,
                                                   const QString &repoUrl) override;

    linglong::utils::error::Result<QStringList> remoteList() override;

    linglong::utils::error::Result<void> importDirectory(const package::Ref &ref,
                                                         const QString &path) override;

    linglong::utils::error::Result<void> renameBranch(const package::Ref &oldRef,
                                                      const package::Ref &newRef) override;

    linglong::utils::error::Result<void> import(const package::Bundle &bundle) override;

    linglong::utils::error::Result<void> exportBundle(package::Bundle &bundle) override;

    linglong::utils::error::Result<QList<linglong::package::Ref>>
    list(const QString &filter) override;

    linglong::utils::error::Result<QList<linglong::package::Ref>>
    query(const QString &filter) override;

    linglong::utils::error::Result<void> push(const package::Ref &ref, bool force) override;

    linglong::utils::error::Result<void> push(const package::Ref &ref) override;

    linglong::utils::error::Result<void> push(const package::Bundle &bundle, bool force) override;

    linglong::utils::error::Result<void> pull(const package::Ref &ref, bool force) override;

    linglong::utils::error::Result<void> pullAll(const package::Ref &ref, bool force) override;

    linglong::utils::error::Result<void> checkout(const package::Ref &ref,
                                                  const QString &subPath,
                                                  const QString &target) override;

    linglong::utils::error::Result<void> removeRef(const package::Ref &ref) override;

    linglong::utils::error::Result<void> remoteDelete(const QString &repoName) override;

    linglong::utils::error::Result<void> checkoutAll(const package::Ref &ref,
                                                     const QString &subPath,
                                                     const QString &target) override;

    linglong::utils::error::Result<QString> compressOstreeData(const package::Ref &ref);

    QString rootOfLayer(const package::Ref &ref) override;

    bool isRefExists(const package::Ref &ref) override;

    QString remoteShowUrl(const QString &repoName) override;

    package::Ref localLatestRef(const package::Ref &ref) override;

    package::Ref remoteLatestRef(const package::Ref &ref) override;

    package::Ref latestOfRef(const QString &appId, const QString &appVersion) override;

    /*
     * 查询ostree远端仓库列表
     *
     * @param repoPath: 远端仓库对应的本地仓库路径
     * @param vec: 远端仓库列表
     * @param err: 错误信息
     *
     * @return bool: true:查询成功 false:失败
     */
    bool getRemoteRepoList(const QString &repoPath, QVector<QString> &vec, QString &err) override;

    /*
     * 查询远端仓库所有软件包索引信息refs
     *
     * @param repoPath: 远端仓库对应的本地仓库路径
     * @param remoteName: 远端仓库名称
     * @param outRefs: 远端仓库软件包索引信息(key:refs, value:commit值)
     * @param err: 错误信息
     *
     * @return bool: true:查询成功 false:失败
     */
    bool getRemoteRefs(const QString &repoPath,
                       const QString &remoteName,
                       QMap<QString, QString> &outRefs,
                       QString &err) override;

    /*
     * 将软件包数据从本地仓库签出到指定目录
     *
     * @param repoPath: 远端仓库对应的本地仓库路径
     * @param remoteName: 远端仓库名称
     * @param ref: 软件包对应的仓库索引
     * @param dstPath: 签出数据保存目录
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool checkOutAppData(const QString &repoPath,
                         const QString &remoteName,
                         const QString &ref,
                         const QString &dstPath,
                         QString &err) override;

    /*
     * 通过ostree命令将软件包数据从远端仓库pull到本地
     *
     * @param destPath: 仓库路径
     * @param remoteName: 远端仓库名称
     * @param ref: 软件包对应的仓库索引ref
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool repoPullbyCmd(const QString &destPath,
                       const QString &remoteName,
                       const QString &ref,
                       QString &err) override;

    /*
     * 删除本地repo仓库中软件包对应的ref分支信息及数据
     *
     * @param repoPath: 仓库路径
     * @param remoteName: 远端仓库名称
     * @param ref: 软件包对应的仓库索引ref
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool repoDeleteDatabyRef(const QString &repoPath,
                             const QString &remoteName,
                             const QString &ref,
                             QString &err) override;

    bool ensureRepoEnv(const QString &repoDir, QString &err) override;

    /*
     * 获取下载任务对应的进程Id
     *
     * @param ref: ostree软件包对应的ref
     *
     * @return int: ostree命令任务对应的进程id
     */
    int getOstreeJobId(const QString &ref)
    {
        if (jobMap.contains(ref)) {
            return jobMap[ref];
        } else {
            for (auto item : jobMap.keys()) {
                if (item.indexOf(ref) > -1) {
                    return jobMap[item];
                }
            }
        }
        return -1;
    }

    /*
     * 获取正在下载的任务列表
     *
     * @return QStringList: 正在下载的应用对应的ref列表
     */
    QStringList getOstreeJobList()
    {
        if (jobMap.isEmpty()) {
            return {};
        }
        return jobMap.keys();
    }

private:
    /*
     * 查询远端ostree仓库描述文件Summary信息
     *
     * @param repo: 远端仓库对应的本地仓库OstreeRepo对象
     * @param name: 远端仓库名称
     * @param outSummary: 远端仓库的Summary信息
     * @param outSummarySig: 远端仓库的Summary签名信息
     * @param cancellable: GCancellable对象
     * @param error: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool fetchRemoteSummary(OstreeRepo *repo,
                            const char *name,
                            GBytes **outSummary,
                            GBytes **outSummarySig,
                            GCancellable *cancellable,
                            GError **error);

    /*
     * 从ostree仓库描述文件Summary信息中获取仓库所有软件包索引refs
     *
     * @param summary: 远端仓库Summary信息
     * @param outRefs: 远端仓库软件包索引信息
     */
    void getPkgRefsBySummary(GVariant *summary, std::map<std::string, std::string> &outRefs);

    /*
     * 从summary中的refMap中获取仓库所有软件包索引refs
     *
     * @param ref_map: summary信息中解析出的ref map信息
     * @param outRefs: 仓库软件包索引信息
     */
    void getPkgRefsFromRefsMap(GVariant *ref_map, std::map<std::string, std::string> &outRefs);

    /*
     * 解析仓库软件包索引ref信息
     *
     * @param fullRef: 目标软件包索引ref信息
     * @param result: 解析结果
     *
     * @return bool: true:成功 false:失败
     */
    bool resolveRef(const std::string &fullRef, std::vector<std::string> &result);

    /*
     * 启动一个ostree 命令任务
     *
     * @param cmd: 需要运行的命令
     * @param ref: ostree软件包对应的ref
     * @param argList: 参数列表
     * @param timeout: 任务超时时间
     *
     * @return bool: true:成功 false:失败
     */
    bool startOstreeJob(const QString &cmd,
                        const QString &ref,
                        const QStringList &argList,
                        const int timeout);

    /*
     * 在/tmp目录下创建一个临时repo子仓库
     *
     * @param parentRepo: 父repo仓库路径
     *
     * @return QString: 临时repo路径
     */
    QString createTmpRepo(const QString &parentRepo);

    /*
     * 保存本地仓库信息
     *
     * @param basedir: 临时仓库路径
     * @param repo: 错误信息
     *
     */
    void setDirInfo(const QString &basedir, OstreeRepo *repo)
    {
        pLingLongDir->basedir = basedir;
        pLingLongDir->repo = repo;
    }

    linglong::utils::error::Result<void> ostreeRun(const QStringList &args,
                                                   QByteArray *stdout = nullptr)
    {
        QProcess ostree;
        ostree.setProgram("ostree");

        QStringList ostreeArgs = { "-v", "--repo=" + ostreePath };
        ostreeArgs.append(args);
        ostree.setArguments(ostreeArgs);

        QProcess::connect(&ostree, &QProcess::readyReadStandardOutput, [&]() {
            // ostree.readAllStandardOutput() can only be read once.
            if (stdout) {
                *stdout += ostree.readAllStandardOutput();
                qDebug() << QString::fromLocal8Bit(*stdout);
            } else {
                qDebug() << QString::fromLocal8Bit(ostree.readAllStandardOutput());
            }
        });

        qDebug() << "start" << ostree.arguments().join(" ");
        ostree.start();
        ostree.waitForFinished(-1);
        qDebug() << ostree.exitStatus() << "with exit code:" << ostree.exitCode();

        return LINGLONG_ERR(ostree.exitCode(),
                            QString::fromLocal8Bit(ostree.readAllStandardError()));
    }

    static QByteArray glibBytesToQByteArray(GBytes *bytes)
    {
        const void *data;
        gsize length;
        data = g_bytes_get_data(bytes, &length);
        return { reinterpret_cast<const char *>(data), static_cast<int>(length) };
    }

    static GBytes *glibInputStreamToBytes(GInputStream *inputStream)
    {
        g_autoptr(GOutputStream) outStream = nullptr;
        g_autoptr(GError) gErr = nullptr;

        if (inputStream == nullptr)
            return g_bytes_new(nullptr, 0);

        outStream = g_memory_output_stream_new(nullptr, 0, g_realloc, g_free);
        g_output_stream_splice(outStream,
                               inputStream,
                               G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                               nullptr,
                               &gErr);
        g_assert_no_error(gErr);

        return g_memory_output_stream_steal_as_bytes(G_MEMORY_OUTPUT_STREAM(outStream));
    }

    // FIXME: use tmp path for big file.
    // FIXME: free the glib pointer.
    static linglong::utils::error::Result<QByteArray> compressFile(const QString &filepath)
    {
        g_autoptr(GError) gErr = nullptr;
        g_autoptr(GBytes) inputBytes = nullptr;
        g_autoptr(GInputStream) memInput = nullptr;
        g_autoptr(GInputStream) zlibStream = nullptr;
        g_autoptr(GBytes) zlibBytes = nullptr;
        g_autoptr(GFile) file = nullptr;
        g_autoptr(GFileInfo) info = nullptr;
        g_autoptr(GVariant) xattrs = nullptr;

        file = g_file_new_for_path(filepath.toStdString().c_str());
        if (file == nullptr) {
            return { LINGLONG_EWRAP(QByteArray(),
                                    LINGLONG_ERR(errno, "open file failed: " + filepath).value()) };
        }

        info = g_file_query_info(file,
                                 G_FILE_ATTRIBUTE_UNIX_MODE,
                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                 nullptr,
                                 nullptr);
        guint32 mode = g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_MODE);

        info = g_file_query_info(file,
                                 G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK,
                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                 nullptr,
                                 nullptr);
        if (g_file_info_get_is_symlink(info)) {
            info = g_file_query_info(file,
                                     G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     nullptr,
                                     nullptr);
            g_file_info_set_file_type(info, G_FILE_TYPE_SYMBOLIC_LINK);
            g_file_info_set_size(info, 0);
        } else {
            info = g_file_query_info(file,
                                     G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                     G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                     nullptr,
                                     nullptr);
            qDebug() << "fize size:" << g_file_info_get_size(info);

            // Q_ASSERT(g_file_info_get_size(info) > 0);
            g_file_info_set_size(info, g_file_info_get_size(info));
            inputBytes = g_file_load_bytes(file, nullptr, nullptr, nullptr);
        }
        // TODO: set uid/gid with G_FILE_ATTRIBUTE_UNIX_UID/G_FILE_ATTRIBUTE_UNIX_GID
        g_file_info_set_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_MODE, mode);

        if (inputBytes) {
            memInput = g_memory_input_stream_new_from_bytes(inputBytes);
        }

        xattrs = g_variant_ref_sink(g_variant_new_array(G_VARIANT_TYPE("(ayay)"), nullptr, 0));

        ostree_raw_file_to_archive_z2_stream(memInput, info, xattrs, &zlibStream, nullptr, &gErr);

        zlibBytes = glibInputStreamToBytes(zlibStream);
        return glibBytesToQByteArray(zlibBytes);
    }

    static OstreeRepo *openRepo(const QString &path)
    {
        GError *gErr = nullptr;
        std::string repoPathStr = path.toStdString();

        auto repoPath = g_file_new_for_path(repoPathStr.c_str());
        auto repo = ostree_repo_new(repoPath);
        if (!ostree_repo_open(repo, nullptr, &gErr)) {
            qCritical() << "open repo" << path << "failed"
                        << QString::fromStdString(std::string(gErr->message));
        }
        g_object_unref(repoPath);

        Q_ASSERT(nullptr != repo);
        return repo;
    }

    QString getObjectPath(const QString &objName)
    {
        return QDir::cleanPath(
          QStringList{ ostreePath, "objects", objName.left(2), objName.right(objName.length() - 2) }
            .join(QDir::separator()));
    }

    // FIXME: return {Error, QStringList}
    QStringList traverseCommit(const QString &rev, int maxDepth)
    {
        GHashTable *hashTable = nullptr;
        GError *gErr = nullptr;
        QStringList objects;

        std::string str = rev.toStdString();

        if (!ostree_repo_traverse_commit(repoPtr,
                                         str.c_str(),
                                         maxDepth,
                                         &hashTable,
                                         nullptr,
                                         &gErr)) {
            qCritical() << "ostree_repo_traverse_commit failed"
                        << "rev" << rev << QString::fromStdString(std::string(gErr->message));
            return {};
        }

        GHashTableIter iter;
        g_hash_table_iter_init(&iter, hashTable);

        GVariant *object;
        for (; g_hash_table_iter_next(&iter, reinterpret_cast<gpointer *>(&object), nullptr);) {
            char *checksum;
            OstreeObjectType objectType;

            // TODO: check error
            g_variant_get(object, "(su)", &checksum, &objectType);
            QString objectName(ostree_object_to_string(checksum, objectType));
            objects.push_back(objectName);
        }

        return objects;
    }

    linglong::utils::error::Result<QList<OstreeRepoObject>>
    findObjectsOfCommits(const QStringList &revs)
    {
        QList<OstreeRepoObject> objects;
        for (const auto &rev : revs) {
            // FIXME: check error
            auto revObjects = traverseCommit(rev, 0);
            for (const auto &objName : revObjects) {
                auto path = getObjectPath(objName);
                objects.push_back(
                  OstreeRepoObject{ .objectName = objName, .rev = rev, .path = path });
            }
        }
        return objects;
    }

    linglong::utils::error::Result<QString> resolveRev(const QString &ref)
    {
        GError *gErr = nullptr;
        char *commitID = nullptr;
        std::string refStr = ref.toStdString();
        // FIXME: should free commitID?
        if (!ostree_repo_resolve_rev(repoPtr, refStr.c_str(), false, &commitID, &gErr)) {
            return LINGLONG_EWRAP("ostree_repo_resolve_rev failed: " + ref,
                                  LINGLONG_ERR(gErr->code, gErr->message).value());
        }
        return QString::fromLatin1(commitID);
    }

    // TODO: support multi refs?
    linglong::utils::error::Result<void> pull(const QString &ref)
    {
        GError *gErr = nullptr;
        // OstreeAsyncProgress *progress;
        // GCancellable *cancellable;
        auto repoNameStr = remoteRepoName.toStdString();
        auto refStr = ref.toStdString();
        auto refsSize = 1;
        const char *refs[refsSize + 1];
        for (decltype(refsSize) i = 0; i < refsSize; i++) {
            refs[i] = refStr.c_str();
        }
        refs[refsSize] = nullptr;

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

        // g_variant_builder_add(&builder, "{s@v}",
        // "override-url",g_variant_new_string(remote_name_or_baseurl));

        g_variant_builder_add(&builder,
                              "{s@v}",
                              "gpg-verify",
                              g_variant_new_variant(g_variant_new_boolean(false)));
        g_variant_builder_add(&builder,
                              "{s@v}",
                              "gpg-verify-summary",
                              g_variant_new_variant(g_variant_new_boolean(false)));

        auto flags = OSTREE_REPO_PULL_FLAGS_NONE;
        g_variant_builder_add(&builder,
                              "{s@v}",
                              "flags",
                              g_variant_new_variant(g_variant_new_int32(flags)));

        g_variant_builder_add(
          &builder,
          "{s@v}",
          "refs",
          g_variant_new_variant(g_variant_new_strv((const char *const *)refs, -1)));

        auto options = g_variant_ref_sink(g_variant_builder_end(&builder));

        if (!ostree_repo_pull_with_options(repoPtr,
                                           repoNameStr.c_str(),
                                           options,
                                           nullptr,
                                           nullptr,
                                           &gErr)) {
            qCritical() << "ostree_repo_pull_with_options failed"
                        << QString::fromStdString(std::string(gErr->message));
        }
        return LINGLONG_OK;
    }

    QSharedPointer<InfoResponse> getRepoInfo(const QString &repoName)
    {
        QUrl url(QString("%1/%2/%3").arg(remoteEndpoint, "api/v1/repos", repoName));

        QNetworkRequest request(url);

        auto reply = httpClient.get(request);
        auto data = reply->readAll();
        qDebug() << "url" << url << "repo info" << data;
        {
            auto [info, err] = util::fromJSON<QSharedPointer<InfoResponse>>(data);
            return info;
        }
    }

    linglong::utils::error::Result<QString> newUploadTask(QSharedPointer<UploadRequest> req)
    {
        QUrl url(QString("%1/api/v1/upload-tasks").arg(remoteEndpoint));
        QNetworkRequest request(url);
        qDebug() << "upload url" << url;
        auto data = std::get<0>(util::toJSON(req));

        request.setRawHeader(QByteArray("X-Token"), remoteToken.toLocal8Bit());
        auto reply = httpClient.post(request, data);
        data = reply->readAll();

        auto info(util::loadJsonBytes<UploadTaskResponse>(data));
        qDebug() << "new upload task" << data;
        if (info->code != 200) {
            return LINGLONG_ERR(-1, info->msg);
        }

        return info->data->id;
    }

    linglong::utils::error::Result<QString> newUploadTask(const QString &repoName,
                                                          QSharedPointer<UploadTaskRequest> req)
    {
        QUrl url(QString("%1/api/v1/blob/%2/upload").arg(remoteEndpoint, repoName));
        QNetworkRequest request(url);

        auto data = std::get<0>(util::toJSON(req));

        request.setRawHeader(QByteArray("X-Token"), remoteToken.toLocal8Bit());
        auto reply = httpClient.post(request, data);
        data = reply->readAll();

        QSharedPointer<UploadTaskResponse> info(util::loadJsonBytes<UploadTaskResponse>(data));
        qDebug() << "new upload task" << data;
        if (info->code != 200) {
            return LINGLONG_ERR(-1, info->msg);
        }
        return info->data->id;
    }

    linglong::utils::error::Result<void> doUploadTask(const QString &taskID,
                                                      const QString &filePath)
    {
        linglong::utils::error::Result<void> err;
        QByteArray fileData;

        QUrl uploadUrl(QString("%1/api/v1/upload-tasks/%2/tar").arg(remoteEndpoint, taskID));
        QNetworkRequest request(uploadUrl);
        request.setRawHeader(QByteArray("X-Token"), remoteToken.toLocal8Bit());

        QScopedPointer<QHttpMultiPart> multiPart(new QHttpMultiPart(QHttpMultiPart::FormDataType));

        // FIXME: add link support
        QList<QHttpPart> partList;
        partList.reserve(filePath.size());

        partList.push_back(QHttpPart());
        auto filePart = partList.last();

        auto *file = new QFile(filePath, multiPart.data());
        file->open(QIODevice::ReadOnly);
        filePart.setHeader(
          QNetworkRequest::ContentDispositionHeader,
          QVariant(QString(R"(form-data; name="%1"; filename="%2")").arg("file", filePath)));
        filePart.setBodyDevice(file);

        multiPart->append(filePart);
        qDebug() << "send " << filePath;

        auto reply = httpClient.put(request, multiPart.data());
        auto data = reply->readAll();

        qDebug() << "doUpload" << data;

        QSharedPointer<UploadTaskResponse> info(util::loadJsonBytes<UploadTaskResponse>(data));

        return LINGLONG_OK;
    }

    linglong::utils::error::Result<void> doUploadTask(const QString &repoName,
                                                      const QString &taskID,
                                                      const QList<OstreeRepoObject> &objects)
    {
        linglong::utils::error::Result<void> err;
        QByteArray fileData;

        QUrl url(QString("%1/api/v1/blob/%2/upload/%3").arg(remoteEndpoint, repoName, taskID));
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("X-Token"), remoteToken.toLocal8Bit());

        QScopedPointer<QHttpMultiPart> multiPart(new QHttpMultiPart(QHttpMultiPart::FormDataType));

        // FIXME: add link support
        QList<QHttpPart> partList;
        partList.reserve(objects.size());

        for (const auto &obj : objects) {
            partList.push_back(QHttpPart());
            auto filePart = partList.last();

            QFileInfo fi(obj.path);
            if (fi.isSymLink() && false) {
                filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                   QVariant(QString("form-data; name=\"%1\"; filename=\"%2\"")
                                              .arg("link", obj.objectName)));
                filePart.setBody(QString("%1:%2").arg(obj.objectName, obj.path).toUtf8());
            } else {
                QString objectName = obj.objectName;
                if (fi.fileName().endsWith(".file")) {
                    objectName += "z";
                    auto result = compressFile(obj.path);
                    filePart.setHeader(
                      QNetworkRequest::ContentDispositionHeader,
                      QVariant(
                        QString(R"(form-data; name="%1"; filename="%2")").arg("file", objectName)));
                    filePart.setBody(*result);
                } else {
                    auto *file = new QFile(obj.path, multiPart.data());
                    file->open(QIODevice::ReadOnly);
                    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                       QVariant(QString(R"(form-data; name="%1"; filename="%2")")
                                                  .arg("file", obj.objectName)));
                    filePart.setBodyDevice(file);
                }
            }
            multiPart->append(filePart);
            qDebug() << fi.isSymLink() << "send " << obj.objectName << obj.path;
        }

        auto reply = httpClient.put(request, multiPart.data());
        auto data = reply->readAll();

        qDebug() << "doUpload" << data;

        QSharedPointer<UploadTaskResponse> info(util::loadJsonBytes<UploadTaskResponse>(data));
        if (200 != info->code) {
            return LINGLONG_ERR(-1, info->msg);
        }

        return LINGLONG_OK;
    }

    linglong::utils::error::Result<void> cleanUploadTask(const QString &repoName,
                                                         const QString &taskID)
    {
        QUrl url(QString("%1/api/v1/blob/%2/upload/%3").arg(remoteEndpoint, repoName, taskID));
        QNetworkRequest request(url);
        // FIXME: check error
        httpClient.del(request);
        return LINGLONG_OK;
    }

    linglong::utils::error::Result<void> cleanUploadTask(const package::Ref &ref,
                                                         const QString &filePath)
    {
        const auto savePath =
          QStringList{ util::getUserFile(".linglong/builder"), ref.appId }.join(QDir::separator());

        util::removeDir(savePath);

        QFile file(filePath);
        file.remove();

        qDebug() << "clean Upload Task";
        return LINGLONG_OK;
    }

    linglong::utils::error::Result<void> getUploadStatus(const QString &taskID)
    {
        QUrl url(QString("%1/%2/%3/status").arg(remoteEndpoint, "api/v1/upload-tasks", taskID));
        QNetworkRequest request(url);

        while (1) {
            auto reply = httpClient.get(request);
            auto data = reply->readAll();

            qDebug() << "url" << url << "repo info" << data;

            auto info = util::loadJsonBytes<UploadTaskResponse>(data);

            if (200 != info->code) {
                return LINGLONG_ERR(-1, "get upload status faild, remote server is unreachable");
            }

            if ("complete" == info->data->status) {
                break;
            } else if ("failed" == info->data->status) {
                return LINGLONG_ERR(-1, info->data->status);
            }

            qInfo().noquote() << info->data->status;
            QThread::sleep(1);
        }

        return LINGLONG_OK;
    }

private:
    // multi-thread
    linglong::utils::error::Result<void> getToken() { return LINGLONG_OK; };

    QString repoRootPath;
    QString remoteEndpoint;
    QString remoteRepoName;

    QString remoteToken;

    OstreeRepo *repoPtr = nullptr;
    QString ostreePath;

    util::HttpRestClient httpClient;

    repo::RepoClient repoClient;

    // ostree 仓库对象信息
    LingLongDir *pLingLongDir;

    QMap<QString, int> jobMap;
};

} // namespace repo
} // namespace linglong

#endif // LINGLONG_SRC_MODULE_REPO_OSTREE_REPO_H_
