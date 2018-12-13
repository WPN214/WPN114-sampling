#include "qml_plugin.hpp"

#include <source/sampler/sampler.hpp>
#include <source/multisampler/multisampler.hpp>

#include <QQmlEngine>
#include <qqml.h>

void qml_plugin::registerTypes(const char *uri)
{
    Q_UNUSED    ( uri );

    qmlRegisterType<Sampler, 1>         ( "WPN114.Audio.Sampling", 1, 0, "Sampler" );
    qmlRegisterType<StreamSampler, 1>   ( "WPN114.Audio.Sampling", 1, 0, "StreamSampler" );
    qmlRegisterType<MultiSampler, 1>    ( "WPN114.Audio.Sampling", 1, 0, "MultiSampler" );
}
