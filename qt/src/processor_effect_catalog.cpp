#include "processor_effect_catalog.h"

#include <algorithm>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace demo_processor
{
namespace
{
QString ResolveFile(const QDir& root, const QString& root_path,
    const QString& relative)
{
    if (relative.isEmpty() || QFileInfo(relative).isAbsolute()) return {};
    const QString candidate = QDir::fromNativeSeparators(
        QFileInfo(root.filePath(relative)).canonicalFilePath());
#if defined(Q_OS_WIN)
    constexpr Qt::CaseSensitivity comparison = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity comparison = Qt::CaseSensitive;
#endif
    if (candidate.startsWith(root_path + QString::fromUtf8("/"), comparison) &&
        QFileInfo(candidate).isFile())
        return candidate;
    return {};
}
} // namespace

EffectPack ProcessorEffectCatalog::Load(bool chinese)
{
    EffectPack pack;
    const QString configured = qEnvironmentVariable(
        "STREAMCORE_DEMO_QT_PROCESSOR_PACK").trimmed();
    const QString manifest_path = configured.isEmpty() ?
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QString::fromUtf8("processor_pack/manifest.json")) : configured;
    QFile manifest(manifest_path);
    if (!QFileInfo(manifest_path).exists()) return pack;
    pack.state = EffectPack::State::Invalid;
    if (!manifest.open(QIODevice::ReadOnly)) return pack;

    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll());
    if (!document.isObject()) return pack;
    const QJsonObject root = document.object();
    if (root.value(QString::fromUtf8("schema_version")).toInt() != 1)
        return pack;

    const QDir directory = QFileInfo(manifest_path).absoluteDir();
    const QString root_path = QDir::fromNativeSeparators(directory.canonicalPath());
    const QString module = ResolveFile(directory, root_path,
        root.value(QString::fromUtf8("module")).toString());
    if (module.isEmpty()) return pack;

    QSet<QString> ids;
    const QJsonArray effects = root.value(QString::fromUtf8("effects")).toArray();
    for (const QJsonValue& value : effects)
    {
        if (!value.isObject()) continue;
        const QJsonObject effect = value.toObject();
        EffectDescription item;
        item.id = effect.value(QString::fromUtf8("id")).toString();
        if (item.id.isEmpty() || item.id.size() > 64 || ids.contains(item.id))
            continue;
        item.sample_path = ResolveFile(directory, root_path,
            effect.value(QString::fromUtf8("sample")).toString());
        if (item.sample_path.isEmpty()) continue;
        const QString reference_text = effect.value(
            QString::fromUtf8("reference_image")).toString();
        if (!reference_text.isEmpty())
        {
            item.reference_path = ResolveFile(directory, root_path, reference_text);
            if (item.reference_path.isEmpty()) continue;
        }
        item.name = effect.value(QString::fromUtf8(
            chinese ? "name_zh" : "name_en")).toString(item.id);
        item.strength = std::clamp(effect.value(
            QString::fromUtf8("strength")).toDouble(0.85), 0.0, 1.0);
        ids.insert(item.id);
        pack.effects.push_back(std::move(item));
    }
    if (pack.effects.isEmpty()) return pack;
    pack.module_path = module;
    pack.state = EffectPack::State::Ready;
    return pack;
}

} // namespace demo_processor
