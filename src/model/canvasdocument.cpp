#include "CanvasDocument.h"
#include <QPainter>
#include <algorithm>

CanvasDocument::CanvasDocument(QObject *parent) : QObject(parent) {
    Frame initialFrame;
    Layer layer;
    layer.name = "Layer 1";
    layer.width = canvasWidth;
    layer.height = canvasHeight;
    layer.pixels.resize(canvasWidth*canvasHeight);
    for(int y = 0; y < layer.height; y++){
        for(int x = 0; x < layer.width; x++){
            layer.at(x, y) = Qt::transparent;
        }
    }
    initialFrame.layers.push_back(layer);
    frames.append(initialFrame);
}
void CanvasDocument::resizeCanvas(int width, int height){
    Layer &layer = frames[currentFrameIndex].layers[activeLayerIndex];
    QVector<QColor> oldPixels = layer.pixels;
    int oldWidth = layer.width;
    int oldHeight = layer.height;

    canvasWidth = width;
    canvasHeight = height;
    layer.width = width;
    layer.height = height;
    layer.pixels.assign(width *height, Qt::transparent);
    for(int y = 0; y <std::min(oldHeight, height); y++){
        for(int x = 0; x <std::min(oldWidth, width); x++){
            layer.pixels[y*width +x] = oldPixels[y*oldWidth+x];
        }
    }
    emit canvasSizeChanged(canvasWidth, canvasHeight);
}
void CanvasDocument::resetCanvas(){
    for(int i = 0; i <= frames[currentFrameIndex].layers.size()+1; i++){
        emit clearLayerList();
    }
    frames[currentFrameIndex].layers.clear();
    emit reInitLayers();
    activeLayerIndex = 0;
    resizeCanvas(32,32);
    buildPalette();
    emit documentMutated();

}
void CanvasDocument::clear(){
    Layer &layer = frames[currentFrameIndex].layers[activeLayerIndex];
    if(layer.type == LayerType::Reference) return;
    for(int y = 0; y < layer.height; y++){
        for(int x = 0; x < layer.width; x++){
            layer.at(x, y) = Qt::transparent;
        }
    }
    buildPalette();
}
void CanvasDocument::buildPalette(){
    colorFrequency.clear();
    for(const auto &layer : frames[currentFrameIndex].layers){
        if(layer.type != LayerType::Pixel) continue;
        for(const QColor &color : layer.pixels){
            if(color == Qt::transparent) continue;
            colorFrequency[color.rgba()]++;
        }
    }
    emit paletteChanged(sortColors(colorFrequency));
}

QList<QColor> CanvasDocument::sortColors(const QHash<QRgb, int> &colorFrequency)const {
    QList<QPair<QRgb, int>> pairs;
    for(auto it = colorFrequency.begin(); it != colorFrequency.end(); ++it){
        pairs.append({it.key(), it.value()});
    }
    std::sort(pairs.begin(), pairs.end(), [](auto a, auto b){ return a.second > b.second;});
    QList<QColor> result;
    for(const auto &pair :pairs) result.append(pair.first);
    return result;
}

void CanvasDocument::addLayer(){
    Layer layer;
    layer.name = "Layer " + QString::number(frames[currentFrameIndex].layers.size() + 1);
    layer.width = canvasWidth;
    layer.height = canvasHeight;
    layer.visible = true;
    layer.pixels.resize(layer.width * layer.height);
    for(auto &pixel : layer.pixels) pixel = Qt::transparent;
    frames[currentFrameIndex].layers.push_back(layer);
    activeLayerIndex = frames[currentFrameIndex].layers.size()-1;
    buildPalette();
    emit layerChanged();
}


