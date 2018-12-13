#include "multisampler.hpp"
#include <cstdlib>

Urn::Urn(quint16 size) : m_size(size) { }

inline quint16 Urn::get()
{
    float rand = (float) std::rand()/RAND_MAX;
    quint16 srand = m_size*rand;

    return srand;
}

quint16 Urn::draw()
{
    if ( m_draws.size() == m_size )
        m_draws.clear();

    auto draw = get();
    while ( m_draws.contains(draw))
        draw = get();

    m_draws.push_back(draw);
    return draw;
}

MultiSampler::MultiSampler() : m_dir(nullptr)
{
    SETN_IN     ( 0 );
    SETN_OUT    ( 0 );
    SETTYPE     ( StreamType::Generator );
}

MultiSampler::~MultiSampler()
{
    delete m_dir;

    for ( const auto& sampler : m_samplers )
        delete sampler;
}

void MultiSampler::setPath(QString path)
{
    m_path = path;
    if ( m_dir ) delete m_dir;

    m_dir = new QDir(path);
    m_dir->setNameFilters(QStringList{"*.wav"});

    m_files = m_dir->entryList();

    for ( const auto& file : m_files )
    {
        Sampler* sampler = new Sampler;

        sampler->setPath(m_path+"/"+file);
        sampler->componentComplete();
        sampler->setActive(true);

        m_samplers.push_back(sampler);

        setNumOutputs( qMax( m_num_outputs, sampler->numOutputs()) );
    }

    m_urn = Urn(m_files.size());

    emit filesChanged();
}

void MultiSampler::initialize(qint64 nsamples)
{
    for ( const auto& sampler : m_samplers )
    {
        sampler->setActive(false);
        sampler->preinitialize(m_stream_properties);
    }
}

float** MultiSampler::process(float** buf, qint64 nsamples)
{
    auto out = m_out;
    auto nout = m_num_outputs;

    StreamNode::resetBuffer(out, nout, nsamples);

    for ( const auto& sampler : m_samplers )
    {
        if ( !sampler->active() ) continue;

        StreamNode::mergeBuffers( out, sampler->preprocess(nullptr, nsamples),
                                 nout, sampler->numOutputs(), nsamples );
    }

    return out;
}

void MultiSampler::play(QVariant variant)
{
    if ( variant.type() == QMetaType::Int )
        m_samplers[variant.toInt()]->play();

    else if ( variant.type() == QMetaType::QString )
    {
        auto idx = m_files.indexOf(variant.toString());
        if ( idx < 0 ) return;
        m_samplers[idx]->play();
    }
}

void MultiSampler::playRandom()
{
    play(m_urn.draw());
}

void MultiSampler::stop(QVariant variant)
{
    if ( variant.type() == QMetaType::Int )
        m_samplers[variant.toInt()]->stop();

    else if ( variant.type() == QMetaType::QString )
    {
        auto idx = m_files.indexOf(variant.toString());
        if ( idx < 0 ) return;
        m_samplers[idx]->stop();
    }
}

