#pragma once
#include "../model/canvastypes.h"
#include <QObject>
#include <QImageReader>

class GifImportWorker : public QObject
{
    Q_OBJECT
public:
    GifImportWorker(const QString &path, int totalFrames, QVector<Frame> postFrames, int targetWidth, int targetHeight, int paletteSize, bool keepAspect)
        : path(path), totalFrames(totalFrames), postFrames(std::move(postFrames)), targetWidth(targetWidth), targetHeight(targetHeight), paletteSize(paletteSize), keepAspect(keepAspect){}
    const QVector<Frame>& getFrames() const {return postFrames;}

public slots:
    void run();
    void cancel();

signals:
    void progress(int current, int total);
    void finished(bool success);

private:
    QString path;
    int totalFrames;
    QVector<Frame> postFrames;
    int targetWidth;
    int targetHeight;
    int paletteSize;
    bool keepAspect;
    QAtomicInt cancelled{0};
};

