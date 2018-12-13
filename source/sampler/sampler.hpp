#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include <source/audio.hpp>
#include <source/soundfile.hpp>
#include <QQmlParserStatus>
#include <QThread>

#define ENV_RESOLUTION 32768
#define BUFSTREAM_MAX_XFADELEN 5
// in seconds
#define BUFSTREAM_NSAMPLES_DEFAULT 131072
// approx. 3 seconds buffer

class StreamSampler : public StreamNode
{
    Q_OBJECT

    Q_PROPERTY  ( QString path READ path WRITE setPath )
    Q_PROPERTY  ( bool loop READ loop WRITE setLoop )
    Q_PROPERTY  ( int xfade READ xfade WRITE setXfade )
    Q_PROPERTY  ( int attack READ attack WRITE setAttack )
    Q_PROPERTY  ( int release READ release WRITE setRelease )
    Q_PROPERTY  ( qreal start READ start WRITE setStart )
    Q_PROPERTY  ( qreal end READ end WRITE setEnd )
    Q_PROPERTY  ( qreal length READ length WRITE setLength )

    public:
    StreamSampler();
    ~StreamSampler() override;

    virtual void componentComplete() override;    
    virtual float** process ( float**, qint64 le ) override;
    virtual void initialize ( qint64 ) override;

    QString path        ( ) const { return m_path; }
    bool loop           ( ) const { return m_loop; }
    quint32 xfade       ( ) const { return m_xfade; }
    quint32 attack      ( ) const { return m_attack; }
    quint32 release     ( ) const { return m_release; }
    qreal start         ( ) const { return m_start; }
    qreal end           ( ) const { return m_end; }
    qreal length        ( ) const { return m_length; }
    qreal rate          ( ) const { return m_rate; }

    void setPath        ( QString path );
    void setLoop        ( bool loop );
    void setXfade       ( quint32 xfade );
    void setAttack      ( quint32 attack );
    void setRelease     ( quint32 release );
    void setStart       ( qreal start );
    void setEnd         ( qreal end );
    void setLength      ( qreal length );
    void setRate        ( qreal rate );

    public slots:
    void onNextBufferReady();
    Q_INVOKABLE void play   ( );
    Q_INVOKABLE void stop   ( );

    signals:
    void reset  (float*);
    void next   (float*);
    void fileLengthChanged ();

    private:
    void reset();

    Soundfile* m_soundfile          = nullptr;
    SoundfileStreamer* m_streamer   = nullptr;
    QThread m_streamer_thread;

    bool m_next_buffer_ready    = false;
    bool m_first_play           = true;
    bool m_releasing            = false;
    bool m_playing              = false;

    quint64 m_buffer_size       = 0;
    quint64 m_play_size         = 0;
    float* m_xfade_buffer       = nullptr;
    float* m_current_buffer     = nullptr;
    float* m_next_buffer        = nullptr;

    quint64 m_phase             = 0;
    quint64 m_stream_phase      = 0;
    quint64 m_xfade_buf_phase   = 0;
    float m_attack_phase        = 0.f;
    float m_release_phase       = 0.f;
    float m_xfade_phase         = 0.f;
    float m_attack_inc          = 0.f;
    float m_release_inc         = 0.f;
    float m_xfade_inc           = 0.f;

    quint64 m_release_end       = 0;
    quint64 m_attack_end        = 0;
    quint64 m_xfade_point       = 0;
    quint64 m_xfade_length      = 0;

    float m_attack_env  [ ENV_RESOLUTION ];
    float m_release_env [ ENV_RESOLUTION ];

    // properties
    QString m_path;
    bool m_loop         = false;
    quint32 m_xfade     = 0;
    quint32 m_attack    = 0;
    quint32 m_release   = 0;
    qreal m_start       = 0;
    qreal m_end         = 0;
    qreal m_length      = 0;
    qreal m_rate        = 1;
};

class Sampler : public StreamNode
{
    Q_OBJECT

    Q_PROPERTY  ( QString path READ path WRITE setPath )
    Q_PROPERTY  ( bool loop READ loop WRITE setLoop )
    Q_PROPERTY  ( int xfade READ xfade WRITE setXfade )
    Q_PROPERTY  ( int attack READ attack WRITE setAttack )
    Q_PROPERTY  ( int release READ release WRITE setRelease )
    Q_PROPERTY  ( qreal start READ start WRITE setStart )
    Q_PROPERTY  ( qreal end READ end WRITE setEnd )
    Q_PROPERTY  ( qreal length READ length WRITE setLength )
    Q_PROPERTY  ( qreal rate READ rate WRITE setRate )

    public:
    Sampler();
    ~Sampler() override;

    virtual void componentComplete() override;

    virtual float** process ( float**, qint64 le ) override;
    virtual void initialize ( qint64 ) override;

    QString path        ( ) const { return m_path; }
    bool loop           ( ) const { return m_loop; }
    quint32 xfade       ( ) const { return m_xfade; }
    quint32 attack      ( ) const { return m_attack; }
    quint32 release     ( ) const { return m_release; }
    qreal start         ( ) const { return m_start; }
    qreal end           ( ) const { return m_end; }
    qreal length        ( ) const { return m_length; }
    qreal rate          ( ) const { return m_rate; }

    void setPath        ( QString path );
    void setLoop        ( bool loop );
    void setXfade       ( quint32 xfade );
    void setAttack      ( quint32 attack );
    void setRelease     ( quint32 release );
    void setStart       ( qreal start );
    void setEnd         ( qreal end );
    void setLength      ( qreal length );
    void setRate        ( qreal rate );

    public slots:
    Q_INVOKABLE void play   ( );
    Q_INVOKABLE void stop   ( );

    private:        


    Soundfile* m_soundfile  = nullptr;
    float* m_buffer         = nullptr;
    quint64 m_buffer_size   = 0;

    bool m_first_play   = true;
    bool m_releasing    = false;
    bool m_playing      = false;

    quint64 m_phase         = 0;
    float m_attack_phase    = 0.f;
    float m_release_phase   = 0.f;
    float m_xfade_phase     = 0.f;
    quint64 m_attack_end    = 0;
    quint64 m_release_end   = 0;
    quint64 m_xfade_point   = 0;
    quint64 m_xfade_length  = 0;

    float m_attack_inc      = 0.f;
    float m_release_inc     = 0.f;
    float m_xfade_inc       = 0.f;

    float m_attack_env  [ ENV_RESOLUTION ];
    float m_release_env [ ENV_RESOLUTION ];

    // properties
    QString m_path;
    bool m_loop         = false;
    quint32 m_xfade     = 0;
    quint32 m_attack    = 0;
    quint32 m_release   = 0;
    qreal m_start       = 0.f;
    qreal m_end         = 0.f;
    qreal m_length      = 0.f;
    qreal m_rate        = 1.f;
};

#endif // SAMPLER_HPP
