#include "sampler.hpp"

#include <cmath>
#include <QtDebug>

#define BUFSR m_soundfile->sampleRate()

StreamSampler::StreamSampler() : StreamNode()
{
    SETTYPE( StreamType::Generator );
    // building sin envelopes
    for ( int i = 0; i < ENV_RESOLUTION; ++ i )
    {
        auto env_value    = sin ((float)i/ENV_RESOLUTION*M_PI_2);
        m_attack_env[i]   = env_value;
        m_release_env[i]  = 1.f-env_value;
    }

    setActive( false );
}

StreamSampler::~StreamSampler()
{
    m_streamer_thread.quit();

    if ( !m_streamer_thread.wait( 3000 ) )
    {
        m_streamer_thread.terminate();
        m_streamer_thread.wait();
        m_streamer->deleteLater();
    }

    delete m_xfade_buffer;
    delete m_current_buffer;
    delete m_next_buffer;
}

inline quint64 ms_to_samples(quint64 x, quint64 sr)
{
    return (x/1000.f)*sr;
}

void StreamSampler::setLoop(bool loop)
{
    m_loop = loop;
    if ( m_streamer ) m_streamer->setWrap(loop);
}

void StreamSampler::setXfade(quint32 xfade)
{
    m_xfade = xfade;
    m_xfade_length = ms_to_samples(xfade, SAMPLERATE);
}

void StreamSampler::setAttack(quint32 attack)
{
    m_attack = attack;
    m_attack_end = ms_to_samples(attack, SAMPLERATE);
}

void StreamSampler::setRelease(quint32 release)
{
    m_release = release;
}

void StreamSampler::setStart(qreal start)
{
    m_start = start;

    if ( m_streamer ) m_streamer->setStartSample( m_start*BUFSR );
}

void StreamSampler::setEnd(qreal end)
{
    m_end       = end;
    m_length    = end-m_start;

    if ( m_streamer ) m_streamer->setEndSample ( m_end*BUFSR);
}

void StreamSampler::setLength(qreal length)
{
    m_length    = length;
    m_end       = m_start + length;

    if ( m_streamer ) m_streamer->setEndSample ( m_end*BUFSR );
}

void StreamSampler::setRate(qreal rate)
{
    m_rate = rate;
}

void StreamSampler::setPath(QString path)
{
    m_path = path;
}

void StreamSampler::componentComplete()
{
    if ( m_path.isEmpty() ) return;

    m_soundfile  = new Soundfile(m_path);
    m_streamer   = new SoundfileStreamer(m_soundfile);

    quint64 nch       = m_soundfile->nchannels();
    quint64 srate     = m_soundfile->sampleRate();
    quint64 nsamples  = m_soundfile->nsamples();

    if ( m_length = 0 ) setLength((qreal) m_soundfile->nsamples()/srate);

    SETN_IN  ( 0   );
    SETN_OUT ( nch );

    if ( m_length == 0 )
    {
        m_streamer->setEndSample ( nsamples );
        setLength(nsamples/srate);
        m_play_size = nsamples-(m_start*srate);
    }
    else
    {
        m_streamer->setEndSample ( m_end*srate );
        m_play_size = (m_end-m_start)*srate;
    }

    m_streamer->setBufferSize  ( BUFSTREAM_NSAMPLES_DEFAULT );
    m_streamer->setWrap        ( m_loop );

    // allocate buffers (in frames, interleaved)
    m_current_buffer  = new float[ BUFSTREAM_NSAMPLES_DEFAULT*nch ]();
    m_next_buffer     = new float[ BUFSTREAM_NSAMPLES_DEFAULT*nch ]();
    m_buffer_size     = BUFSTREAM_NSAMPLES_DEFAULT;

    m_streamer->moveToThread(&m_streamer_thread);
    QObject::connect(this, SIGNAL(next(float*)), m_streamer, SLOT(next(float*)));
    QObject::connect(this, SIGNAL(reset(float*)), m_streamer, SLOT(reset(float*)));
    QObject::connect(m_streamer, SIGNAL(bufferLoaded()), this, SLOT(onNextBufferReady()));
}

void StreamSampler::initialize(qint64)
{
    m_xfade_inc     = static_cast<float>(ENV_RESOLUTION/(float)ms_to_samples(m_xfade, SAMPLERATE));
    m_attack_inc    = static_cast<float>(ENV_RESOLUTION/(float)ms_to_samples(m_attack, SAMPLERATE));
    m_attack_end    = ms_to_samples(m_attack, SAMPLERATE);
    m_release_inc   = static_cast<float>(ENV_RESOLUTION/(float)ms_to_samples(m_release, SAMPLERATE));
    m_release_end   = ENV_RESOLUTION;
    m_xfade_length  = ms_to_samples(m_xfade, SAMPLERATE);
    m_xfade_point   = m_play_size-m_xfade_length;

    quint64 srate   = m_soundfile->sampleRate();

    // load crossfade buffer
    m_xfade_buffer = new float[ BUFSTREAM_MAX_XFADELEN*SAMPLERATE*m_num_outputs ]();
    m_soundfile->buffer( m_xfade_buffer, m_start*srate, BUFSTREAM_MAX_XFADELEN*SAMPLERATE );

    // start streamer thread, load first buffer
    // start sample is the end of the 'up' xfade
    // as it is already handled by the xfade buffer
    m_streamer->setStartSample( m_xfade_length );
    m_streamer_thread.start( QThread::LowPriority );
    emit next( m_current_buffer );
}

void StreamSampler::onNextBufferReady()
{
    m_next_buffer_ready = true;
}

void StreamSampler::play()
{
    if ( m_playing )
    {
        // if already playing, reset
        reset();
    }

    else m_playing = true;
    if ( !m_active ) setActive( true );
}

void StreamSampler::stop()
{
    if ( m_active && m_playing )
        m_releasing = true;

    else
    {
        reset();

        m_playing      = false;
        m_releasing    = false;
    }
}

inline float lininterp(float x, float a, float b)
{
    return a + x * (b - a);
}

inline void StreamSampler::reset()
{
    m_phase             = 0;
    m_stream_phase      = 0;
    m_xfade_buf_phase   = 0;
    m_attack_phase      = 0;
    m_xfade_phase       = 0;
    m_release_phase     = 0;
    m_first_play        = true;
}

float** StreamSampler::process(float** buf, qint64 nsamples)
{
    auto bufdata        = m_current_buffer;
    auto xfdata         = m_xfade_buffer;
    auto bufnsamples    = m_buffer_size;
    auto playnsamples   = m_play_size;
    auto first          = m_first_play;
    auto spos           = m_phase;
    auto bpos           = m_stream_phase;
    auto xpos           = m_xfade_buf_phase;
    auto out            = m_out;
    auto nch            = m_num_outputs;
    auto loop           = m_loop;

    auto release        = m_release_env;
    auto release_end    = m_release_end;
    auto release_phase  = m_release_phase;
    auto release_inc    = m_release_inc;

    auto attack         = m_attack_env;
    auto attack_end     = m_attack_end;
    auto attack_phase   = m_attack_phase;
    auto attack_inc     = m_attack_inc;
    auto xfade_point    = m_xfade_point;
    auto xfade_phase    = m_xfade_phase;
    auto xfade_inc      = m_xfade_inc;
    auto xfade_length   = m_xfade_length;

    // get buffers back in position
    bufdata  += bpos*nch;
    xfdata   += xpos*nch;

    if ( loop && spos > attack_end && spos > xfade_length )
    {
        first         = false;
        m_first_play  = first;
    }

    for ( qint64 s = 0; s < nsamples; ++s )
    {        
        if ( !m_playing )
        {
            // filling with zeroes before going inactive
            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s] = 0.f;
            return out;
        }

        if ( spos == xfade_length && bpos == 0 ) emit next( m_next_buffer );
        if ( bpos == bufnsamples )
        {
            // if reaching the end of current buffer
            // swap buffers and request a new one (asynchronous)
            float* tmp         = m_current_buffer;
            m_current_buffer   = m_next_buffer;
            bufdata            = m_next_buffer;

            m_next_buffer         = tmp;
            m_next_buffer_ready   = false;

            emit next( m_next_buffer );
            bpos = 0;
        }

        if ( first && spos < attack_end )
        {
            //          if first play && phase is in the 'attack zone'
            //          get interpolated data from envelope
            int y       = floor ( attack_phase );
            float x     = (float) attack_phase-y;
            float e     = lininterp(x, attack[y], attack[y+1]);

            if ( loop )
            {
                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = *xfdata++*e;
                xpos++;
            }
            else
            {
                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = *bufdata++*e;
                bpos++;
            }

            spos++;
            attack_phase += attack_inc;
        }

        else if ( first && spos < xfade_length )
        {
            // if still behind the xfade 'up' end
            // but after the attack
            // continue to get data from xfade buffer
            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s] = *xfdata++;

            spos++;
            xpos++;
        }

        else if ( first && spos == xfade_length )
        {
            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s] = *bufdata++;

            spos++;
            bpos++;
            xpos = 0;
        }

        else if ( loop && spos >= xfade_point && spos < playnsamples )
        {
            //          if in loop mode and phase is in the 'crossfade zone'
            //          get interpolated data from attack envelope
            int y       = floor(xfade_phase);
            float x     = (float) xfade_phase-y;
            float xfu   = lininterp(x, attack[y], attack[y+1]);
            float xfd   = 1.f - xfu;

            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s]  = *bufdata++*xfd + *xfdata++*xfu;

            spos++;
            bpos++;
            xpos++;
            xfade_phase += xfade_inc;
        }

        else if ( spos == playnsamples )
        {
            if ( loop )
            {
                // if phase reaches end of 'crossfade zone'
                // main phase continues from end of 'up' crossfade
                // reset envelope phase
                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = *bufdata++;

                spos = xfade_length+1;
                xpos = 0;
                xfade_phase = 0;
                bpos++;
            }
            else
            {
                // if not looping, stop and set inactive
                // fill rest of buffer with zeroes
                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = 0.f;

                reset();

                m_playing      = false;
                setActive      ( false );
                m_releasing    = false;

                emit reset(m_current_buffer);
            }
        }
        else
        {
            // normal behaviour
            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s] = *bufdata++;

            spos++;
            bpos++;
        }

        if ( m_releasing )
        {
            // if reaching end of release envelope
            if ( release_phase >= release_end || m_attack == 0)
            {                
                reset();
                emit reset( m_current_buffer );

                m_playing       = false;
                setActive       ( false );
                m_releasing     = false;

                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = 0.f;
            }
            else
            {
                int y       = floor ( release_phase );
                float x     = (float) release_phase-y;
                float rel   = lininterp(x, release[y], release[y+1]);

                for ( quint16 ch = 0; ch < nch; ++ch )               
                    out[ch][s] *= rel;

                release_phase += release_inc;
            }
        }
    }

    m_phase             = spos;
    m_stream_phase      = bpos;
    m_xfade_buf_phase   = xpos;
    m_attack_phase      = attack_phase;
    m_xfade_phase       = xfade_phase;
    m_release_phase     = release_phase;

    return out;
}

//-----------------------------------------------------------------------------------

Sampler::Sampler() : StreamNode()
{    
    SETTYPE( StreamType::Generator );
    // building sin envelopes
    for ( int i = 0; i < ENV_RESOLUTION; ++ i )
    {
        auto env_value    = sin ((float)i/ENV_RESOLUTION*M_PI_2);
        m_attack_env[i]   = env_value;
        m_release_env[i]  = 1.f-env_value;
    }

    setActive(false);
}

Sampler::~Sampler()
{
    delete m_soundfile;
    delete m_buffer;
}

void Sampler::componentComplete()
{
    if ( m_path.isEmpty() ) return;

    m_soundfile     = new Soundfile(m_path);
    quint16 nch     = m_soundfile->nchannels();
    quint64 srate   = m_soundfile->sampleRate();

    if ( m_length == 0 ) // if length unspecified take from start to the end of the fle
    {
        quint64 nsamples = m_soundfile->nsamples()-(m_start*srate);
        m_length = (qreal) nsamples/srate;
        m_buffer = new float[nsamples*nch]();
        m_buffer_size = nsamples;

        m_soundfile->buffer(m_buffer, m_start*srate, nsamples);
    }
    else
    {
        quint64 len = (m_end-m_start)*srate;
        m_buffer = new float[len*nch]();
        m_buffer_size = len;

        m_soundfile->buffer(m_buffer, m_start*srate, len);
    }

    SETN_IN  ( 0 );
    SETN_OUT ( nch );
}

void Sampler::setPath(QString path)
{
    m_path = path;
}

void Sampler::setLoop(bool loop)
{
    m_loop = loop;
}

void Sampler::setXfade(quint32 xfade)
{
    m_xfade         = xfade;
    m_xfade_length  = ms_to_samples(xfade, SAMPLERATE);
}

void Sampler::setAttack(quint32 attack)
{
    m_attack        = attack;
    m_attack_end    = ms_to_samples(attack, SAMPLERATE);
}

void Sampler::setRelease(quint32 release)
{
    m_release = release;
}

void Sampler::setStart(qreal start)
{
    m_start = start;
}

void Sampler::setEnd(qreal end)
{
    m_end       = end;
    m_length    = end-m_start;
}

void Sampler::setLength(qreal length)
{
    m_length    = length;
    m_end       = m_start + length;
}

void Sampler::setRate(qreal rate)
{
    m_rate = rate;
}

void Sampler::play()
{
    m_playing = true;
    if ( !m_active ) setActive(true);
}

void Sampler::stop()
{
    m_releasing = true;
}

void Sampler::initialize(qint64)
{   
    m_xfade_inc     = static_cast<float>(ENV_RESOLUTION/(float)ms_to_samples(m_xfade, SAMPLERATE));
    m_attack_inc    = static_cast<float>(ENV_RESOLUTION/(float)ms_to_samples(m_attack, SAMPLERATE));
    m_release_inc   = static_cast<float>(ENV_RESOLUTION/(float)ms_to_samples(m_release, SAMPLERATE));

    m_release_end   = ENV_RESOLUTION;
    m_attack_end    = ms_to_samples(m_attack, SAMPLERATE);

    m_xfade_length  = ms_to_samples(m_xfade, SAMPLERATE);
    m_xfade_point   = m_buffer_size-m_xfade_length;
}

float** Sampler::process(float**, qint64 le)
{
    auto bufdata        = m_buffer;
    auto bufnsamples    = m_buffer_size;
    auto first          = m_first_play;
    auto spos           = m_phase;
    auto out            = m_out;
    auto nch            = m_num_outputs;
    auto loop           = m_loop;

    auto attack         = m_attack_env;
    auto attack_end     = m_attack_end;
    auto attack_phase   = m_attack_phase;
    auto attack_inc     = m_attack_inc;
    auto release_phase  = m_release_phase;

    auto release        = m_release_env;
    auto release_inc    = m_release_inc;
    auto release_end    = m_release_end;

    auto xfade_point    = m_xfade_point;
    auto xfade_phase    = m_xfade_phase;
    auto xfade_inc      = m_xfade_inc;
    auto xfade_len      = m_xfade_length;

    bufdata+= spos*nch;

    if ( loop && spos > attack_end )
    {
        first        = false;
        m_first_play = first;
    }

    for ( qint64 s = 0; s < le; ++s )
    {
        if ( !m_playing )
        {
            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s] = 0.f;
            return out;
        }

        else if ( first && spos < attack_end )
        {
            //          if first play && phase is in the 'attack zone'
            //          get interpolated data from envelope
            int y       = floor(attack_phase);
            float x     = (float) attack_phase-y;
            float e     = lininterp(x, attack[y], attack[y+1]);

            for ( quint16 ch = 0; ch < nch; ++ch )
            {
                out[ch][s] = *bufdata*e;
                bufdata++;
            }

            spos++;
            attack_phase += attack_inc;
        }
        else if ( loop && spos >= xfade_point && spos < bufnsamples )
        {
            //          if loop mode and if phase is in the 'crossfade zone'
            //          get interpolated data from envelope
            int y       = floor(xfade_phase);
            float x     = (float) xfade_phase-y;
            float xfu   = lininterp(x, attack[y], attack[y+1]);
            float xfd   = 1.f - xfu;

            for ( quint16 ch = 0; ch < nch; ++ch )
            {
                float* rphs = bufdata-xfade_point*nch;
                out[ch][s]  = *bufdata*xfd + *rphs*xfu;

                bufdata++;
            }
            spos++;
            xfade_phase += xfade_inc;
        }

        else if ( spos == bufnsamples )
        {
            if ( loop )
            {
                // if phase reaches end of 'crossfade zone'
                // main phase continues from end of 'up' crossfade
                // reset envelope phase
                bufdata         = &m_buffer[xfade_len*nch];
                xfade_phase     = 0;

                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = *bufdata++;

                spos = xfade_len+1;
            }
            else
            {
                // if not looping, stop and set inactive
                // zero out rest of the buffer
                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = 0.f;

                m_phase             = 0;
                m_attack_phase      = 0;
                m_xfade_phase       = 0;
                m_release_phase     = 0;
                m_playing           = false;
                m_first_play        = true;
                setActive          ( false );
                m_releasing         = false;
            }
        }
        else
        {
            // normal behaviour
            for ( quint16 ch = 0; ch < nch; ++ch )
                out[ch][s] = *bufdata++;

            spos++;
        }

        if ( m_releasing )
        {
            // if reaching end of release envelope
            if ( release_phase >= release_end )
            {
                m_phase             = 0;
                m_attack_phase      = 0;
                m_xfade_phase       = 0;
                m_release_phase     = 0;
                m_playing           = false;
                m_first_play        = true;
                setActive          ( false );
                m_releasing         = false;

                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] = 0.f;
            }

            else
            {
                int y       = floor ( release_phase );
                float x     = (float) release_phase-y;
                float rel   = lininterp(x, release[y], release[y+1]);

                for ( quint16 ch = 0; ch < nch; ++ch )
                    out[ch][s] *= rel;

                release_phase += release_inc;
            }
        }
    }

    // update member values
    m_phase             = spos;
    m_attack_phase      = attack_phase;
    m_xfade_phase       = xfade_phase;
    m_release_phase     = release_phase;

    return out;
}
