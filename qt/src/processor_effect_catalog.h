#pragma once

#include <QVector>
#include <QString>

namespace demo_processor
{

struct EffectDescription
{
    QString id;
    QString name;
    QString sample_path;
    QString reference_path;
    double strength = 0.85;
};

struct EffectPack
{
    enum class State { Missing, Invalid, Ready };
    State state = State::Missing;
    QString module_path;
    QVector<EffectDescription> effects;
};

// Reads the optional data-only effect-pack manifest. This is a Qt Demo
// convenience layer, not an SDK plugin loader: the SDK never scans folders.
// Paths named by the manifest must be existing files under the manifest's own
// directory. A pack must provide at least one effect with a sample clip/image
// so a fresh Demo install has an immediately runnable choice.
class ProcessorEffectCatalog final
{
public:
    static EffectPack Load(bool chinese);
};

} // namespace demo_processor
