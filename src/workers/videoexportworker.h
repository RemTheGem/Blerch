#pragma once

#include <QWidget>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoFrameInput>
#include <QVideoFrame>

class VideoExportWorker : public QWidget
{
    Q_OBJECT
public:

    VideoExportWorker(QString path, int imageWidth, int imageHeight, int frameSize)
        : path(std::move(path)), imageWidth(imageWidth), imageHeight(imageHeight), frameSize(frameSize){}

public slots:
    void run();
    void cancel();
    void recieveFrame(QImage frame, int duration);

signals:
    void progress(int current, int total);
    void finished(bool success);
    void recieveNextFrame(int index);

private:
    QString path;
    int imageWidth;
    int imageHeight;
    int frameSize;
    QMediaCaptureSession *videoSession = nullptr;
    QMediaRecorder *videoRecorder = nullptr;
    QVideoFrameInput *videoFrameInput = nullptr;
    QImage currentFrame;
    int currentDuration;
    int videoExportFrameIndex = 0;
    int repeatIndex = 0;
    QAtomicInt cancelled{0};

};

