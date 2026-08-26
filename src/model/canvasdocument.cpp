#include "CanvasDocument.h"
#include <QPainter>
#include <algorithm>
#include <QDebug>
#include <QDialog>

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

    for(auto &frame : frames){
        for(auto &layer : frame.layers){
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
    if(frames[currentFrameIndex].layers.isEmpty()) return;
    for(const auto &layer : std::as_const(frames[currentFrameIndex].layers)){
        if(layer.type != LayerType::Pixel) continue;
        for(const QColor &color : layer.pixels){
            if(color == Qt::transparent) continue;
            colorFrequency[color.rgba()]++;
        }
    }
    emit paletteUpdated(sortColors(colorFrequency));
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
void CanvasDocument::removeLayer(int index){
    if(frames[currentFrameIndex].layers.size() <=1) return;
    frames[currentFrameIndex].layers.erase(frames[currentFrameIndex].layers.begin() + index);
    activeLayerIndex = std::clamp(activeLayerIndex, 0, (int)frames[currentFrameIndex].layers.size() -1);
    emit layerChanged();
}
void CanvasDocument::setActiveLayer(int index){
    if(index< 0|| index>= frames[currentFrameIndex].layers.size()) return;
    if(activeLayerIndex == index) return;
    activeLayerIndex = index;
    emit layerChanged();
}
void CanvasDocument::moveLayerDown(int index){
    if(index<=0|| index>frames[currentFrameIndex].layers.size()) return;
    std::swap(frames[currentFrameIndex].layers[index], frames[currentFrameIndex].layers[index-1]);
    if(activeLayerIndex == index)activeLayerIndex--;
    else if(activeLayerIndex == index-1) activeLayerIndex++;
    emit layerChanged();
}
void CanvasDocument::moveLayerUp(int index){
    if(index < 0 || index >= frames[currentFrameIndex].layers.size()-1) return;
    std::swap(frames[currentFrameIndex].layers[index], frames[currentFrameIndex].layers[index+1]);
    if(activeLayerIndex == index) activeLayerIndex++;
    else if(activeLayerIndex == index +1) activeLayerIndex--;
    emit layerChanged();

}

void CanvasDocument::renameLayer(int index, const QString &name){
    if(index < 0 || index >= frames[currentFrameIndex].layers.size()) return;
    frames[currentFrameIndex].layers[index].name = name;
    emit layerChanged();
}
void CanvasDocument::setLayerOpacity(int index, float opacity){
    if(index < 0 || index >= frames[currentFrameIndex].layers.size()) return;
    frames[currentFrameIndex].layers[index].opacity = opacity;
    emit layerChanged();
}
float CanvasDocument::getLayerOpacity(int index ) const{
    if(index <0 || index >= frames[currentFrameIndex].layers.size()) return 1.0f;
    return frames[currentFrameIndex].layers[index].opacity;
}
QStringList CanvasDocument::getLayerNames() const {
    QStringList names;
    if(frames.isEmpty()) return names;
    if(currentFrameIndex < 0 || currentFrameIndex >= frames.size()) return names;
    for(const auto &layer :frames[currentFrameIndex].layers){
        if(!layer.isTempLayer) names.append(layer.name);
    }
    return names;
}
int CanvasDocument::getActiveLayer() const {return activeLayerIndex;}

void CanvasDocument::makeTempLayer(){
    addLayer();
    setActiveLayer(frames[currentFrameIndex].layers.size()-1);
    frames[currentFrameIndex].layers[activeLayerIndex].isTempLayer = true;
    frames[currentFrameIndex].layers[activeLayerIndex].opacity = 0.5f;
    frames[currentFrameIndex].layers[activeLayerIndex].width = canvasWidth;
    frames[currentFrameIndex].layers[activeLayerIndex].height = canvasHeight;
    frames[currentFrameIndex].layers[activeLayerIndex].name = "Preview";
    emit layerChanged();
}
void CanvasDocument::removeTempLayer(){
    removeLayer(frames[currentFrameIndex].layers.size()-1);
}
Layer &CanvasDocument::activeLayer_() {return frames[currentFrameIndex].layers[activeLayerIndex];}
const Layer &CanvasDocument::activeLayer_() const {return frames[currentFrameIndex].layers[activeLayerIndex];}
Frame &CanvasDocument::currentFrame_() {return frames[currentFrameIndex];}
const Frame &CanvasDocument::currentFrame_() const {return frames[currentFrameIndex];}

void CanvasDocument::duplicateFrame(){
    Frame newFrame = frames[currentFrameIndex];
    frames.insert(currentFrameIndex+1, newFrame);
    currentFrameIndex++;
    activeLayerIndex = 0;
    frames[currentFrameIndex].undoStack.clear();
    frames[currentFrameIndex].redoStack.clear();
    emit layerChanged();
    emit frameChanged();
}

void CanvasDocument::copyFrame() {copiedFrame = frames[currentFrameIndex];}

void CanvasDocument::pasteFrame(){
    if(copiedFrame.isEmpty())return;
    frames.insert(currentFrameIndex+1, copiedFrame);
    currentFrameIndex++;
    activeLayerIndex = 0;
    frames[currentFrameIndex].undoStack.clear();
    frames[currentFrameIndex].redoStack.clear();
    emit layerChanged();
    emit frameChanged();
}
void CanvasDocument::addFrame(){
    Frame newFrame;
    Layer layer;
    layer.name = "Layer 1";
    layer.width = canvasWidth;
    layer.height = canvasHeight;
    layer.pixels.resize(canvasWidth *canvasHeight);
    for(int y= 0; y <layer.height; y++){
        for(int x = 0; x <layer.width; x++){
            layer.at(x, y) = Qt::transparent;
        }
    }
    newFrame.layers.push_back(layer);
    frames.insert(currentFrameIndex+1, newFrame);
    currentFrameIndex++;
    activeLayerIndex = 0;
    emit layerChanged();
    emit frameChanged();

}
void CanvasDocument::deleteFrame(int index){
    if(frames.size() <= 1) return;
    if(index <0 || index >= frames.size()) return;
    frames.removeAt(index);
    if(currentFrameIndex >= frames.size()) currentFrameIndex = frames.size() -1;
    if(activeLayerIndex >= frames[currentFrameIndex].layers.size())
        activeLayerIndex = frames[currentFrameIndex].layers.size()-1;
    emit frameChanged();
}

void CanvasDocument::switchFrame(int index){
    if(index < 0 || index >= frames.size()) return;
    currentFrameIndex = index;
    if(activeLayerIndex >= frames[currentFrameIndex].layers.size())
        activeLayerIndex = frames[currentFrameIndex].layers.size()-1;
    emit frameChanged();
    emit layerChanged();
    buildPalette();
}
int CanvasDocument::getCurrentFrame() const { return currentFrameIndex;}
int CanvasDocument::getFrameSize() const {return frames.size();}
int CanvasDocument::getFrameDuration() const {return frames[currentFrameIndex].duration;}
int CanvasDocument::getThisFrameDuration(int index) const {return frames[index].duration;}
void CanvasDocument::setFrameDuration(int value){frames[currentFrameIndex].duration = value;}

void CanvasDocument::setAllFrameDurations(int value){
    for(auto &frame: frames){
        frame.duration = value;
    }
    emit documentMutated();
}
QImage CanvasDocument::renderFrame(int frameIndex) const {
    QImage image(canvasWidth, canvasHeight, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter (&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for(const auto &layer :frames[frameIndex].layers){
        if(!layer.visible) continue;
        painter.setOpacity(layer.opacity);
        for(int y = 0; y < layer.height; y ++){
            for(int x = 0; x < layer.width; x++){
                QColor color = layer.at(x, y);
                if(color.alpha()>9) painter.fillRect(x,y,1,1,color);
            }
        }
    }
    return image;
}

void CanvasDocument::loadFrames(const QList<Frame> &newFrames){
    frames = newFrames;
    currentFrameIndex = 0;
    activeLayerIndex = 0;
    emit frameChanged();
    emit layerChanged();
}


void CanvasDocument::pushUndoAction(const UndoAction &action){
    frames[currentFrameIndex].undoStack.push_back(action);
    frames[currentFrameIndex].redoStack.clear();
}
void CanvasDocument::undo(){
    Frame &frame = frames[currentFrameIndex];
    if(frame.undoStack.empty()) return;
    UndoAction action = frame.undoStack.back();
    frame.undoStack.pop_back();
    if(action.type == UndoType::Pixel){
        for(auto &change : action.changes ){
            frame.layers[change.layer].at(change.x, change.y) = change.oldColor;
        }
    }
    else if(action.type == UndoType::Snapshot){
        frame.layers = action.before;
    }
    frame.redoStack.push_back(action);
    emit documentMutated();
}
void CanvasDocument::redo(){
    Frame &frame = frames[currentFrameIndex];
    if(frame.redoStack.empty()) return;
    UndoAction action = frame.redoStack.back();
    frame.redoStack.pop_back();
    if(action.type== UndoType::Pixel){
        for(auto &change : action.changes){
            frame.layers[change.layer].at(change.x, change.y) = change.newColor;
        }
    }
    else if(action.type == UndoType::Snapshot){
        frame.layers = action.after;
    }
    frame.undoStack.push_back(action);
    emit documentMutated();
}