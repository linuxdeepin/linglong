/*
 * SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.:
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "linglong/package/reference.h"

#include <qregularexpression.h>

#include <QStringList>

namespace linglong::package {

utils::error::Result<Reference> Reference::parse(const QString &raw) noexcept
{
    LINGLONG_TRACE("parse reference string");

    static QRegularExpression referenceRegExp(
      R"((?<channel>[^:]+):(?<id>[^\/]+)\/(?<version>[^\/]+)\/(?<architecture>[^\/]+))");

    auto matches = referenceRegExp.match(raw);
    if (not(matches.isValid() and matches.hasMatch())) {
        return LINGLONG_ERR("regexp mismatched.", -1);
    }

    auto channel = matches.captured("channel");
    auto id = matches.captured("id");
    auto versionStr = matches.captured("version");
    auto architectureStr = matches.captured("architecture");

    auto version = Version::parse(versionStr);
    if (!version) {
        return LINGLONG_ERR(version);
    }

    auto arch = Architecture::parse(architectureStr);
    if (!arch) {
        return LINGLONG_ERR(arch);
    }

    auto reference = create(channel, id, *version, *arch);
    if (!reference) {
        return LINGLONG_ERR(reference);
    }

    return *reference;
}

utils::error::Result<Reference> Reference::create(const QString &channel,
                                                  const QString &id,
                                                  const Version &version,
                                                  const Architecture &arch) noexcept
try {
    return Reference(channel, id, version, arch);
} catch (const std::exception &e) {
    LINGLONG_TRACE("create reference");
    return LINGLONG_ERR("invalid reference", e);
}

Reference::Reference(const QString &channel,
                     const QString &id,
                     const Version &version,
                     const Architecture &arch)
    : channel(channel)
    , id(id)
    , version(version)
    , arch(arch)
{
    if (channel.isEmpty()) {
        throw std::runtime_error("empty channel");
    }
    if (id.isEmpty()) {
        throw std::runtime_error("empty id");
    }
}

QString Reference::toString() const noexcept
{
    return QString("%1:%2/%3/%4").arg(channel, id, version.toString(), arch.toString());
}

} // namespace linglong::package
