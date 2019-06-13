#pragma once

#include <wpn114audio/graph.hpp>
#include <sndfile.h>

#define WPN_DECLARE_AUDIO_INPUT(_name, _nchannels) \
    WPN_DECLARE_AUDIO_PORT(_name, Polarity::Input, _nchannels)

#define WPN_DECLARE_AUDIO_OUTPUT(_name, _nchannels) \
    WPN_DECLARE_AUDIO_PORT(_name, Polarity::Output, _nchannels)

class Sampler : public Node
{
    Q_OBJECT

    WPN_DECLARE_AUDIO_INPUT(play, 1)
    WPN_DECLARE_AUDIO_INPUT(loop, 1)
    WPN_DECLARE_AUDIO_INPUT(rate, 1)
    WPN_DECLARE_AUDIO_INPUT(startpos, 1)
    WPN_DECLARE_AUDIO_INPUT(endpos, 1)

    WPN_DECLARE_AUDIO_OUTPUT(end_out, 1)
    WPN_DECLARE_AUDIO_OUTPUT(loop_out, 1)
    WPN_DECLARE_AUDIO_OUTPUT(audio_out, 0)

    SNDFILE* m_file         = nullptr;
    sample_t* m_envelope    = nullptr;
    sample_t** m_buffer     = nullptr;
    SF_INFO m_info;
    sample_t m_srate = 0;
    sample_t m_bufplayrate = 0;

    uint32_t
    m_attack    = 0,
    m_release   = 0,
    m_xfade     = 0;

    bool
    m_playing    = false,
    m_attacking  = false,
    m_releasing  = false;

    sample_t
    m_fphase = 0,
    m_aphase = 0,
    m_rphase = 0,
    m_inc_a  = 0,
    m_inc_r  = 0;

public:

    Sampler()
    {

    }

    ~Sampler()
    {

    }

    virtual void
    initialize(const Graph::properties& properties) override
    {

    }

    virtual void
    on_rate_changed(sample_t rate) override
    {

    }

    virtual void
    rwrite(pool &inputs, pool &outputs, vector_t nframes) override
    {

    }

};
