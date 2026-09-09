#include "gifimportworker.h"
#include "../tools/mediancut.h"
#include <QFileInfo>

void GifImportWorker::run(){
    MedianCut medianCut;
    QImageReader reader(path);
    for(int x=0; x < totalFrames; x++){
        // if(!reader.jumpToImage(x)) break;
        if(cancelled.loadRelaxed()) break;
        Frame frame;
        Layer layer;
        layer.type = LayerType::Pixel;
        QImage image = reader.read();
        if(image.isNull()) continue;
        image = image.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        layer.width = image.width();
        layer.height = image.height();
        layer.name = QFileInfo(path).baseName();
        auto palette = medianCut.medianCut(image, paletteSize);
        QImage argb = image.convertToFormat(QImage::Format_ARGB32);
        std::unordered_map<QRgb, QColor> nearestCache;
        nearestCache.reserve(4096);
        layer.pixels.resize(argb.width() * argb.height());
        for (int y = 0; y < argb.height(); y++) {
            const QRgb* line = reinterpret_cast<const QRgb*> (argb.constScanLine(y));
            for (int x = 0; x < argb.width(); x++) {
                QRgb rgb = line[x];
                if (qAlpha(rgb) == 0){
                    layer.at(x, y) = Qt::transparent;
                    continue;
                }
                auto it = nearestCache.find(rgb);
                if(it != nearestCache.end()){
                    layer.at(x,y) = it->second;
                }
                else{
                    QColor nearest = medianCut.nearestColor(QColor(rgb), palette);
                    nearestCache.emplace(rgb, nearest);
                    layer.at(x,y) = nearest;
                }
            }
        }
        frame.layers.push_back(layer);
        postFrames.append(frame);
        emit progress(x + 1, totalFrames);
    }
    emit finished(!cancelled.loadRelaxed());
}

void GifImportWorker::cancel(){
    cancelled.storeRelaxed(1);
}