#ifndef MULTISAMPLER_HPP
#define MULTISAMPLER_HPP

#include <source/audio.hpp>
#include <source/sampler/sampler.hpp>
#include <QDir>

class Urn
{
    public:
    Urn() {}
    Urn(quint16 size);
    quint16 draw();

    private:
    quint16 get();
    quint16 m_size;
    QVector<quint16> m_draws;
};

class MultiSampler : public StreamNode
{
    Q_OBJECT

    Q_PROPERTY  ( QString path READ path WRITE setPath )
    Q_PROPERTY  ( QStringList files READ files NOTIFY filesChanged )

    public:
    MultiSampler();   
    ~MultiSampler() override;

    virtual void initialize ( qint64 ) override;
    virtual float** process ( float** buf, qint64 nsamples ) override;

    Q_INVOKABLE void play(QVariant var);
    Q_INVOKABLE void playRandom();

    Q_INVOKABLE void stop(QVariant var);

    QString path    ( ) const { return m_path; }
    void setPath    ( QString path );

    QStringList files() const { return m_files; }

    signals:
    void filesChanged();

    private:
    Urn m_urn;
    QDir* m_dir;
    QString m_path;
    QStringList m_files;
    QVector<Sampler*> m_samplers;
};

#endif // MULTISAMPLER_HPP
