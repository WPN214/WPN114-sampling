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

    enum inputs { play, loop, rate, startpos, endpos, attack, release, xfade };

    //---------------------------------------------------------------------------------------------
    WPN_DECLARE_DEFAULT_AUDIO_OUTPUT(audio_out, 0)
    WPN_DECLARE_AUDIO_OUTPUT(end_out, 1)
    WPN_DECLARE_AUDIO_OUTPUT(loop_out, 1)

    enum outputs { audio_out, end_out, loop_out };

    Q_PROPERTY (QString path READ path WRITE set_path)

    //---------------------------------------------------------------------------------------------
    SNDFILE*
    m_file = nullptr;

    sample_t*
    m_envelope = nullptr;

    sample_t*
    m_buffer = nullptr; // interleaved

    SF_INFO
    m_info;

    sample_t
    m_srate = 0,
    m_bufplayrate = 0;

    QString
    m_path;

    Interpolation
    m_interpolation = Interpolation::Linear,
    m_envelope_interpolation = Interpolation::Linear;

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
        m_dispatch = Dispatch::Values::Chain;
    }

    //---------------------------------------------------------------------------------------------
    virtual
    ~Sampler() override
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
    initialize(Graph::properties const& properties) override
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
        sf_readf_float(m_file, m_buffer, m_info.frames);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    on_rate_changed(sample_t rate) override
    //---------------------------------------------------------------------------------------------
    {
        m_srate = rate;
    }

    //---------------------------------------------------------------------------------------------
    sample_t
    lookup(sample_t* read, sample_t frame,
           nchannels_t channel, nchannels_t nchannels,
           Interpolation interpolation)
    //---------------------------------------------------------------------------------------------
    {
        nframes_t y = (nframes_t) frame;
        nframes_t sample = y*nchannels+channel;
        nframes_t next = sample+nchannels;
        sample_t x = frame-y;

        switch(interpolation) {
            case Interpolation::Sin4:
                return sininterp(x, read[sample], read[next]);
            case Interpolation::Linear:
            default:
                return lininterp(x, read[sample], read[next]);
        }
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    rwrite(pool& inputs, pool& outputs, vector_t nframes) override
    //---------------------------------------------------------------------------------------------
    {
        auto play       = inputs.audio[Sampler::play][0];
        auto loop       = inputs.audio[Sampler::loop][0];
        auto rate       = inputs.audio[Sampler::rate][0];
        auto startpos   = inputs.audio[Sampler::startpos][0];
        auto endpos     = inputs.audio[Sampler::endpos][0];
        auto attack     = inputs.audio[Sampler::attack][0];
        auto release    = inputs.audio[Sampler::release][0];
        auto xfade      = inputs.audio[Sampler::xfade][0];

        auto out        = outputs.audio[Sampler::audio_out];
        auto end_out    = outputs.audio[Sampler::end_out][0];
        auto loop_out   = outputs.audio[Sampler::loop_out][0];

        auto playing    = m_playing;
        auto attacking  = m_attacking;
        auto releasing  = m_releasing;

        auto fphase     = m_fphase;
        auto aphase     = m_aphase;
        auto rphase     = m_rphase;
        auto inc_a      = m_inc_a;
        auto inc_r      = m_inc_r;

        auto nchannels  = m_info.channels;
        auto bufnframes = m_info.frames;
        auto bufrate    = m_bufplayrate;
        auto env        = m_envelope;
        auto buf        = m_buffer;

        auto interp      = m_interpolation;
        auto env_interp  = m_envelope_interpolation;

        for (vector_t f = 0; f < nframes; ++f)
        {
            bool sigplay = play[f] != 0.f;

            if (!playing && sigplay)
            {
                //-------------------------------------------
                // OFF
                // sampler in off state
                // no play signal received yet
                //------------------------------------------
                fprintf(stderr, "sampler: OFF\n");

                // output zero
                for (nchannels_t c = 0; c < nchannels; ++c)
                     out[c][f] = 0;

                continue;
            }

            else if (!playing && sigplay)
            {
                //-------------------------------------------
                // PLAY
                // sampler in off state
                // play signal received
                //------------------------------------------
                fprintf(stderr, "sampler: PLAY\n");
                playing = true;

                if (attack)
                {
                     attacking  = true;
                     aphase     = inc_a;

                     // first frame in 'attack mode' will always be zero
                     // we can skip to the next one
                     for (nchannels_t ch = 0; ch < nchannels; ++ch)
                          out[ch][f] = 0;

                     fphase = startpos[f]*bufnframes + rate[f] * bufrate;
                     continue;
                }
            }

            else if (playing && sigplay)
            {
                //-------------------------------------------
                // STOP
                // stop signal received while playing
                //------------------------------------------
                fprintf(stderr, "sampler: STOP\n");

                if (release)
                {
                     releasing = true;
                     rphase = 0;
                }
                else
                {
                    for (nchannels_t ch = 0; ch < nchannels; ++ch)
                         out[ch][f] = 0;

                    playing = false;
                    continue;
                }
            }

            // PLAYING
            //-------------------------------------
            // 8 channels per soundfile max,
            // which is what .wav format can manage
            //-------------------------------------
            sample_t frame[8]    = { 0, 0, 0, 0, 0, 0, 0, 0 };
            uint32_t startpos_f  = startpos[f] * bufnframes;
            uint32_t endpos_f    = endpos[f] * bufnframes;
            uint32_t attack_f    = attack[f]/1000.f * bufnframes;
            uint32_t release_f   = release[f]/1000.f * bufnframes;
            uint32_t xfade_f     = xfade[f]/1000.f * bufnframes*bufrate;
            uint32_t xfade_point = endpos_f-xfade_f;

            sample_t frame_rate  = rate[f] * bufrate;

            if (fphase >= endpos_f || fphase < startpos_f)
            {
                fprintf(stderr, "sampler: out of bounds\n");

                if (loop[f] == 0.f)
                {
                    // if loop mode OFF, stop playing
                    fprintf(stderr, "sampler: ending\n");

                    for (nchannels_t ch = 0; ch < nchannels; ++ch)
                         out[ch][f] = 0;

                    playing = false;
                    end_out[f] = 0;
                    continue;
                }
                else
                {
                    // else restart from end of xfade
                    fprintf(stderr, "sampler: looping\n");

                    if  (fphase >= endpos_f)
                         fphase =  startpos_f + xfade_f + (fphase-endpos_f);
                    else fphase = fphase-startpos_f + endpos_f; // reverse case TODO

                    loop_out[f] = 1;
                }
            }

            // fetch data from buffer
            for (nchannels_t ch = 0; ch < nchannels; ++ch)
                 frame[ch] = lookup(buf, fphase, ch, nchannels, interp);

            // attack/release/xfade ratios
            if (attacking)
            {
                if (aphase < esz)
                {
                    auto attack_v = lookup(env, aphase, 0, 1, env_interp);

                    for (nchannels_t ch = 0; ch < nchannels; ++ch)
                        frame[ch] *= attack_v;

                    aphase += inc_a;
                }
                else
                {
                    attacking = false;
                    aphase = 0;
                }
            }

            if (xfade_f >= xfade_point)
            {
                // - absolute phase
                // - envelope phase
                // - mirror sample position
                sample_t xf_pabs = fphase-xfade_point;
                sample_t xf_p = xf_pabs/xfade_f*(sample_t)esz;
                sample_t xfm_p = startpos_f+xf_pabs;

                // fetch envelope value
                sample_t xf_v = lookup(env, xf_p, 0, 1, env_interp);

                for (nchannels_t ch = 0; ch < nchannels; ++ch) {
                    // fetch mirror sample value
                    // and mix the two values together
                    sample_t xfm_v = lookup(buf, xfm_p, ch, nchannels, env_interp);
                    frame[ch] = frame[ch] * (1-xf_v) + xfm_v * xf_v;
                }
            }

            if (releasing)
            {
                fprintf(stderr, "sampler: releasing\n");
                if (rphase >= release_f)
                {
                    for (nchannels_t ch = 0; ch < nchannels; ++ch)
                        out[ch][f] = 0;

                    releasing = false;
                    playing = false;
                    end_out[f] = 1;

                    continue;
                }

                sample_t release_v = 1-lookup(env, rphase, 0, 1, interp);
                for (nchannels_t ch = 0; ch < nchannels; ++ch)
                    frame[ch] *= release_v;

                rphase += inc_r;
            }

            for (nchannels_t ch = 0; ch < nchannels; ++ch)
                out[ch][f] = frame[ch];

            fphase += frame_rate;
        }

        m_playing    = playing;
        m_attacking  = attacking;
        m_releasing  = releasing;

        m_aphase = aphase;
        m_fphase = fphase;
        m_rphase = rphase;
    }
};
