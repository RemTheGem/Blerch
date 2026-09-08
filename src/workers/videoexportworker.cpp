#include "videoexportworker.h"
#include <QUrl>

void VideoExportWorker::run(){
    videoExportFrameIndex = 0;
    repeatIndex = 1;
    videoSession = new QMediaCaptureSession(this);
    videoRecorder = new QMediaRecorder(this);
    videoFrameInput = new QVideoFrameInput(this);
    videoSession->setRecorder(videoRecorder);
    videoSession->setVideoFrameInput(videoFrameInput);
    videoRecorder->setOutputLocation(QUrl::fromLocalFile(path));
    videoRecorder->setQuality(QMediaRecorder::HighQuality);
    int imageWidth = imageWidth;
    int imageHeight = imageHeight;
    connect(videoFrameInput, &QVideoFrameInput::readyToSendVideoFrame, this, [this, imageWidth, imageHeight](){
        if(cancelled.loadRelaxed()) return;
        if(videoExportFrameIndex >= frameSize){
            emit finished(!cancelled.loadRelaxed());
            videoRecorder->stop();
            return;
        }
        const int fps = 30;
        const double frameInterval = 1000.0 / fps;
        emit recieveNextFrame(videoExportFrameIndex);
        QVideoFrame videoFrame(currentFrame.convertToFormat(QImage::Format_RGBA8888));
        videoFrameInput->sendVideoFrame(videoFrame);
        repeatIndex++;
        int repeatCount = qMax(1, qRound(currentDuration/frameInterval));
        if(repeatIndex > repeatCount){
            repeatIndex = 1;
            videoExportFrameIndex++;
        }
        emit progress(videoExportFrameIndex +1, frameSize);
        qDebug() << "Repeat Count for frame: "<< videoExportFrameIndex << " is: " << repeatCount;
    });
    connect(videoRecorder, &QMediaRecorder::recorderStateChanged, this, [this](QMediaRecorder::RecorderState state){
        if(state == QMediaRecorder::StoppedState){
            videoSession->deleteLater();
            videoRecorder->deleteLater();
            videoFrameInput->deleteLater();
            videoSession = nullptr;
            videoRecorder = nullptr;
            videoFrameInput = nullptr;
        }
    });
    videoRecorder->record();

}
void VideoExportWorker::recieveFrame(QImage frame, int duration){
    currentFrame = frame;
    currentDuration = duration;

}
void VideoExportWorker::cancel(){
    cancelled.storeRelaxed(1);
}