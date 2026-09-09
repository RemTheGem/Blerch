#include "videoexportworker.h"
#include <QUrl>

void VideoExportWorker::run(){
    videoExportFrameIndex = 0;
    repeatIndex = 1;
    codecAttempts = 0;
    startRecording(codecFallbacks[codecAttempts]);
}

void VideoExportWorker::startRecording(QMediaFormat::VideoCodec codec){
    const int thisGeneration = ++recorderGeneration;
    currentTimestamp = 0;
    videoSession = new QMediaCaptureSession(this);
    videoRecorder = new QMediaRecorder(this);
    videoFrameInput = new QVideoFrameInput(this);
    videoSession->setRecorder(videoRecorder);
    videoSession->setVideoFrameInput(videoFrameInput);
    videoRecorder->setOutputLocation(QUrl::fromLocalFile(path));
    videoRecorder->setQuality(QMediaRecorder::HighQuality);
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(codec);
    videoRecorder->setMediaFormat(format);
    connect(videoRecorder, &QMediaRecorder::errorOccurred, this, [this, thisGeneration](QMediaRecorder::Error error, const QString &errorString){
        if(thisGeneration != recorderGeneration) return;
        if(error != QMediaRecorder::ResourceError) return;
        qDebug() << "Codec " << codecFallbacks[codecAttempts] << " failed to open: " << errorString;
        tearDownRecorder();
        codecAttempts++;
        if(codecAttempts >= codecFallbacks.size()){
            qDebug() << "No codecs left";
            emit finished(ExportResult::Failed);
            return;
        }
        videoExportFrameIndex = 0;
        repeatIndex = 1;
        startRecording(codecFallbacks[codecAttempts]);
    });
    connect(videoFrameInput, &QVideoFrameInput::readyToSendVideoFrame, this, [this, thisGeneration](){
        if(thisGeneration != recorderGeneration) return;
        if(cancelled.loadRelaxed()) return;
        if(videoExportFrameIndex >= frameSize){
            emit finished(cancelled.loadRelaxed() ? ExportResult::Cancelled : ExportResult::Success);
            videoRecorder->stop();
            return;
        }
        const int fps = 30;
        const double frameInterval = 1000.0 / fps;
        const qint64 frameDuration = qint64(1000000.0/fps);
        qDebug() << "frame Duration: " << frameDuration;
        emit recieveNextFrame(videoExportFrameIndex);
        QVideoFrame videoFrame(currentFrame.convertToFormat(QImage::Format_RGBA8888));
        videoFrame.setStartTime(currentTimestamp);
        videoFrame.setEndTime(currentTimestamp+frameDuration);
        currentTimestamp += frameDuration;
        videoFrameInput->sendVideoFrame(videoFrame);
        repeatIndex++;
        int repeatCount = qMax(1, qRound(currentDuration/frameInterval));
        if(repeatIndex > repeatCount){
            repeatIndex = 1;
            videoExportFrameIndex++;
        }
        emit progress(videoExportFrameIndex + 1, frameSize);
    });
    connect(videoRecorder, &QMediaRecorder::recorderStateChanged, this, [this, thisGeneration](QMediaRecorder::RecorderState state){
        if(thisGeneration != recorderGeneration) return;
        if(state == QMediaRecorder::StoppedState){
            tearDownRecorder();
        }
    });
    videoRecorder->record();
}

void VideoExportWorker::tearDownRecorder(){
    if(videoSession)videoSession->deleteLater();
    if(videoRecorder)videoRecorder->deleteLater();
    if(videoFrameInput)videoFrameInput->deleteLater();
    videoSession = nullptr;
    videoRecorder = nullptr;
    videoFrameInput = nullptr;
}
void VideoExportWorker::recieveFrame(QImage frame, int duration){
    currentFrame = frame;
    currentDuration = duration;

}
void VideoExportWorker::cancel(){
    cancelled.storeRelaxed(1);
}