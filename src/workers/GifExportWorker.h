#pragma once
#include <QObject>
#include <QVector>
#include <QImage>

class GifExportWorker : public QObject {
    Q_OBJECT
public:
    GifExportWorker(QString path, QVector<QImage> frames, QVector<int> durations, int scale)
        : path(std::move(path)), frames(std::move(frames)), durations(std::move(durations)), scale(scale) {}
public slots:
    void run();
    void cancel();

signals:
    void progress(int current, int total);
    void finished(bool success);

private:
    QString path;
    QVector<QImage> frames;
    QVector<int> durations;
    int scale;
    QAtomicInt cancelled{0};
};
