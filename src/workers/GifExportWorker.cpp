#include "GifExportWorker.h"
#include "../../thirdParty/gif-h/gif.h"

void GifExportWorker::run(){
    QByteArray filePath = path.toUtf8();
    GifWriter writer = {};
    int width = frames[0].width() * scale;
    int height = frames[0].height() * scale;
    if(!GifBegin(&writer, filePath.constData(), width, height, durations[0]/10)){
        emit finished(false);
        return;
    }
    for(int i = 0; i< frames.size(); i++){
        if(cancelled.loadRelaxed()) break;
        QImage img = frames[i].convertToFormat(QImage::Format_RGBA8888);
        GifWriteFrameScaled(&writer, img.constBits(), frames[i].width(), frames[i].height(), scale, durations[i]/10);
        emit progress(i +1, frames.size());
    }
    GifEnd(&writer);
    emit finished(!cancelled.loadRelaxed());
}
void GifExportWorker::cancel(){
    cancelled.storeRelaxed(1);
}