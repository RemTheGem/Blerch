#pragma once

#include <QObject>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoFrameInput>
#include <QVideoFrame>
#include <QMediaFormat>

class VideoExportWorker : public QObject
{
    Q_OBJECT
public:

    VideoExportWorker(QString path, int imageWidth, int imageHeight, int frameSize)
        : path(std::move(path)), imageWidth(imageWidth), imageHeight(imageHeight), frameSize(frameSize){}

    enum class ExportResult{Success, Failed, Cancelled};
public slots:
    void run();
    void cancel();
    void recieveFrame(QImage frame, int duration);

signals:
    void progress(int current, int total);
    void finished(ExportResult result);
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
    QList<QMediaFormat::VideoCodec> codecFallbacks = {
        QMediaFormat::VideoCodec::H264
    };
    int codecAttempts = 0;
    int recorderGeneration = 0;
    qint64 currentTimestamp = 0;

    void startRecording(QMediaFormat::VideoCodec codec);
    void tearDownRecorder();

};

