#include "qml_plugin.hpp"

#include <source/sampler.hpp>
#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED(uri)

    qmlRegisterType<Sampler, 1>
    ("WPN114.Audio.Sampling", 1, 0, "Sampler");
}
