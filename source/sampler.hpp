#pragma once

#include <wpn114audio/graph.hpp>
#include <sndfile.h>

//=================================================================================================
class Sampler : public Node
//=================================================================================================
{
    Q_OBJECT

    //---------------------------------------------------------------------------------------------
    WPN_DECLARE_DEFAULT_MIDI_INPUT(midi_in, 1)

    //---------------------------------------------------------------------------------------------
    WPN_DECLARE_AUDIO_INPUT(play, 1)
    WPN_DECLARE_AUDIO_INPUT(loop, 1)
    WPN_DECLARE_AUDIO_INPUT(rate, 1)
    WPN_DECLARE_AUDIO_INPUT(startpos, 1)
    WPN_DECLARE_AUDIO_INPUT(endpos, 1)

    WPN_DECLARE_AUDIO_INPUT(attack, 1)
    WPN_DECLARE_AUDIO_INPUT(release, 1)
    WPN_DECLARE_AUDIO_INPUT(xfade, 1)

    enum inputs { midi_in, play, loop, rate, startpos, endpos, attack, release, xfade };

    //---------------------------------------------------------------------------------------------
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT(audio_out, 0)
    WPN_DECLARE_AUDIO_OUTPUT(end_out, 1)
    WPN_DECLARE_AUDIO_OUTPUT(loop_out, 1)

    enum outputs { audio_out, end_out, loop_out };

    Q_PROPERTY(QString path READ path WRITE setPath)

    //---------------------------------------------------------------------------------------------
    SNDFILE* m_file         = nullptr;
    sample_t* m_envelope    = nullptr;
    sample_t* m_buffer      = nullptr; // interleaved
    SF_INFO m_info;
    sample_t m_srate = 0;
    sample_t m_bufplayrate = 0;
    QString m_path;

    //---------------------------------------------------------------------------------------------
    bool
    m_playing    = false,
    m_attacking  = false,
    m_releasing  = false;

    //---------------------------------------------------------------------------------------------
    sample_t
    m_fphase = 0,
    m_aphase = 0,
    m_rphase = 0,
    m_inc_a  = 0,
    m_inc_r  = 0;

    static constexpr size_t
    esz = 32768;

public:

    //---------------------------------------------------------------------------------------------
    Sampler()
    {
        m_name = "Sampler";
        m_dispatch = Dispatch::Values::Downwards;
    }

    //---------------------------------------------------------------------------------------------
    virtual ~Sampler() override
    //---------------------------------------------------------------------------------------------
    {
        delete[] m_envelope;
        delete[] m_buffer;
    }

    //---------------------------------------------------------------------------------------------
    QString
    path() const { return m_path; }

    //---------------------------------------------------------------------------------------------
    void
    set_path(QString path) { m_path = path; }

    //---------------------------------------------------------------------------------------------
    virtual void
    componentComplete() override
    // this happens before allocation
    // load file, set n_outputs
    //---------------------------------------------------------------------------------------------
    {
        if (m_path.isEmpty())
            return;

        m_file = sf_open(CSTR(m_path), SFM_READ, &m_info);
        m_audio_out.set_nchannels(m_info.channels);
        Node::componentComplete();
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    initialize(const Graph::properties& properties) override
    // allocate envelope and buffer
    // setup playing rates, envelope size in frames, etc.
    //---------------------------------------------------------------------------------------------
    {
        m_srate     = properties.rate;
        m_envelope  = new sample_t[esz]();
        m_buffer    = new sample_t[m_info.frames*m_info.channels]();
        // note: buffer is interleaved

        // fill envelope
        for (size_t n = 0; n < esz; ++n)
            m_envelope[n] = sin(M_PI_4*static_cast<sample_t>(n)/esz);

        // read buffer
        sf_readf_double(m_file, m_buffer, m_info.frames);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    //---------------------------------------------------------------------------------------------
    {
        m_srate = rate;
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto play       = inputs[Sampler::play][0];
        auto loop       = inputs[Sampler::loop][0];
        auto rate       = inputs[Sampler::rate][0];
        auto startpos   = inputs[Sampler::startpos][0];
        auto endpos     = inputs[Sampler::endpos][0];
        auto attack     = inputs[Sampler::attack][0];
        auto release    = inputs[Sampler::release][0];
        auto xfade      = inputs[Sampler::xfade][0];

        auto out        = outputs[Sampler::audio_out];
        auto end_out    = outputs[Sampler::end_out][0];
        auto loop_out   = outputs[Sampler::loop_out][0];

        auto playing    = m_playing;
        auto attacking  = m_attacking;
        auto releasing  = m_releasing;

        auto fphase     = m_fphase;
        auto aphase     = m_aphase;
        auto rphase     = m_rphase;
        auto inc_a      = m_inc_a;
        auto inc_r      = m_inc_r;

        for (vector_t f = 0; f < nframes; ++f)
        {
            bool sigplay = play[f] != 0.;

            if (!playing && sigplay)
            {

            }

            else if (!playing && sigplay)
            {

            }

            else if (playing && sigplay)
            {

            }

            sample_t frame[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
            uint32_t endpos_f = endpos[f];
            uint32_t startpos_f = startpos[f];

            if (fphase >= endpos_f || fphase < startpos_f)
            {

            }

            // fetch data from buffer

            // attack/release/xfade ratios

            if (attacking)
            {

            }

            if (xfade >= xfade_point)
            {

            }

            if (releasing)
            {

            }



        }


    }

};
