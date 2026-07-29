#include "PixelCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QColorDialog>
#include <QImage>
#include <QFileDialog>
#include <QPoint>
#include <queue>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollArea>
#include <QScrollBar>


PixelCanvas::PixelCanvas(QWidget *parent)
    : QWidget(parent)
{
    Layer layer;

    layer.name = "Layer 1";
    layer.width = canvasWidth;
    layer.height = canvasHeight;
    layer.pixels.resize(canvasWidth * canvasHeight);
    for (int y = 0; y < layer.height; y++) {
        for (int x = 0; x < layer.width; x++) {
            layer.at(x,y) = Qt::transparent;
        }
    }
    undoStack.push_back(currentAction);
    layers.push_back(layer);
    updateCanvasSize();
    setMouseTracking(true);
}
void PixelCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    int checkerSize = pixelSize;
    for (int y = 0; y < height(); y += checkerSize) {
        for (int x = 0; x < width(); x += checkerSize) {
            bool dark = ((x / checkerSize) + (y / checkerSize)) % 2;
            if (dark)
                painter.fillRect(x, y, checkerSize, checkerSize, QColor(224, 224, 224));
            else
                painter.fillRect(x, y, checkerSize, checkerSize, QColor(176, 176, 176));
        }
    }
    for (const auto &layer : layers)
    {
        if(!layer.visible) continue;
        painter.save();
        painter.setOpacity(layer.opacity);
        if(layer.type == LayerType::Pixel){
        for (int y = 0; y < layer.height; y++)
        {
            for (int x = 0; x < layer.width; x++)
            {
                QRect rect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
                QColor color = layer.at(x, y);
                if(color != Qt::transparent)
                {
                    painter.fillRect(rect, color);
                }
            }
        }
        }
        else {
            QRect target(
                layer.position.x() * pixelSize,
                layer.position.y() * pixelSize,
                layer.image.width() * layer.scale * pixelSize,
                layer.image.height() * layer.scale * pixelSize
                );

            painter.drawImage(target, layer.image);
        }

        painter.restore();
    }
    for(int y = 0; y < height(); y += pixelSize)
    {
        painter.drawLine(0, y, width(), y);
    }
    for(int x = 0; x < width(); x += pixelSize)
    {
        painter.drawLine(x, 0, x, height());
    }
}
void PixelCanvas::updateCanvasSize()
{
    setFixedSize(layers[activeLayer].width *pixelSize, layers[activeLayer].height *pixelSize);
    update();
}
void PixelCanvas::resizeCanvas(int width, int height)
{
    layers[activeLayer].width = width;
    layers[activeLayer].height = height;
    layers[activeLayer].pixels.resize(width * height);

    for(auto &pixel : layers[activeLayer].pixels) pixel = Qt::transparent;

    updateCanvasSize();
    update();
}
ColorPreviewWidget::ColorPreviewWidget(QWidget *parent){

}
void ColorPreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect rect(
        0,
        0,
        previewSize,
        previewSize
        );
    painter.setPen(Qt::black);
    painter.fillRect(rect, selectedColor);
    painter.drawRect(rect);

}
void ColorPreviewWidget::setColor(const QColor &color){
    selectedColor = color;
    update();
}

void PixelCanvas::mousePressEvent(QMouseEvent *event)
{
    if(layers[activeLayer].type == LayerType::Reference){
        if(event->button() == Qt::LeftButton){
            QPoint mousePosCanvas(event->position().x() / pixelSize, event-> position().y() / pixelSize);
            movingPicture = true;
            moveOffset = event->pos() - layers[activeLayer].position;
        }
    }
    else{
    currentAction.clear();
    if(event->button() == Qt::LeftButton){
    isDrawing = true;
    // make sure you don't overwrite on the old canvas. if you do, it leads to both actions being on the same canvas and any subsequent undos undoes both.
    if(isUndoing){
        undoStack.push_back(currentAction);
        isUndoing = false;
    }


    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;

    if (x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height) {
        switch(currentTool){
        case Tool::Brush:
            currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), currentColor});
            layers[activeLayer].at(x, y) = currentColor;
            break;
        case Tool::Eraser:
            currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), Qt::transparent});
            layers[activeLayer].at(x, y) = Qt::transparent;
            break;
        case Tool::EyeDropper:
            currentColor = layers[activeLayer].at(x, y);
            emit colorChanged(currentColor);
            break;
        case Tool::Fill:
            floodFill(x, y);
            break;
        }

        update();
    }
    }
    else if(event->button() == Qt::RightButton){
            isDrawing = true;
            isErasing = true;
            // make sure you don't overwrite on the old canvas. if you do, it leads to both actions being on the same canvas and any subsequent undos undoes both.
            if(isUndoing){
                undoStack.push_back(currentAction);
                isUndoing = false;
            }


            int x = event->position().x() / pixelSize;
            int y = event->position().y() / pixelSize;

            if (x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height) {
                currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), Qt::transparent});
                layers[activeLayer].at(x, y) = Qt::transparent;
            }

            update();
        }
           else if(event->button() == Qt::MiddleButton){
            int x = event->position().x() / pixelSize;
            int y = event->position().y() / pixelSize;
            if (x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height) {
                currentColor = layers[activeLayer].at(x, y);
                emit colorChanged(currentColor);
            }
            }
    }
}

void PixelCanvas::wheelEvent(QWheelEvent *event)
{
    if(event->modifiers() & Qt::ShiftModifier){
        if(layers[activeLayer].type == LayerType::Reference){
            if(event->angleDelta().y() > 0){
                layers[activeLayer].scale *= 1.1f;
            }
            else layers[activeLayer].scale *= 0.9f;
            layers[activeLayer].scale = std::clamp(layers[activeLayer].scale, 0.001f, 10.0f);
            update();
        }
        return;
    }
    if(event->modifiers() & Qt::ControlModifier){
        if(event->angleDelta().y() > 0)
            setZoom(pixelSize + 2);
        else
            setZoom(std::max(2, pixelSize - 2));
        return;
    }
    QWidget::wheelEvent(event);
}
void PixelCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if(movingPicture){
        QPoint mousePosCanvas(event->position().x() / pixelSize, event-> position().y() / pixelSize);
        layers[activeLayer].position = (event->pos() - moveOffset) / pixelSize;
        update();
    }
    if (!isDrawing) return;

    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;
    bool changed = false;
    if(!isErasing){
    if (x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height) {
        switch(currentTool){
        case Tool::Brush:
            if(layers[activeLayer].at(x, y) != currentColor)
            {
                currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), currentColor});
                layers[activeLayer].at(x, y) = currentColor;
                changed = true;
            }
            break;
        case Tool::Eraser:
            if(layers[activeLayer].at(x, y) != Qt::transparent)
            {
                currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), Qt::transparent});
                layers[activeLayer].at(x, y) = Qt::transparent;
                changed = true;
            }
            break;
        }
    }
    }
    else{
        if(x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height){
            if (layers[activeLayer].at(x, y) != Qt::transparent){
                currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), Qt::transparent});
                layers[activeLayer].at(x, y) = Qt::transparent;
                changed = true;
            }
        }
    }
    if (changed) update();
}
void PixelCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    isDrawing = false;
    isErasing = false;
    movingPicture = false;
    if(movingPicture){
        QPoint mousePosCanvas(event->position().x() / pixelSize, event-> position().y() / pixelSize);
        layers[activeLayer].position = (event->pos() - moveOffset) / pixelSize;
        update();
    }
    if(!currentAction.empty()){
        undoStack.push_back(currentAction);
        redoStack.clear();
    }
}
void PixelCanvas::clear()
{
    if (layers[activeLayer].type == LayerType::Reference) return;
    for (int y = 0; y < layers[activeLayer].height; y++) {
        for (int x = 0; x < layers[activeLayer].width; x++) {
            layers[activeLayer].at(x, y) = Qt::transparent;
        }
    }
    update();
}
void PixelCanvas::setZoom(int zoom){
    pixelSize = zoom;
    updateCanvasSize();
    update();
}
int PixelCanvas::getZoom(){
    return pixelSize;
}
void PixelCanvas::addLayer(){
    Layer layer;
    layer.name = "Layer " + QString::number(layers.size()+1);
    layer.width = canvasWidth;
    layer.height = canvasHeight;
    layer.visible = true;
    layer.pixels.resize(layer.width * layer.height);
    for(auto &pixel : layer.pixels) pixel = Qt::transparent;
    layers.push_back(layer);
    activeLayer = layers.size()-1;
    update();
}
void PixelCanvas::removeLayer(int index){
    if(layers.size() <= 1) return;
    layers.erase(layers.begin()+index);
    activeLayer = std::clamp(activeLayer,0,(int)layers.size()-1);
    update();
}
void PixelCanvas::setActiveLayer(int index){
    if(index >= 0 && index < layers.size()){
        activeLayer = index;
        update();
    }
}
QStringList PixelCanvas::getLayerNames(){
    QStringList names;
    for(const auto &layer : layers) names.append(layer.name);
    return names;
}
void PixelCanvas::renameLayer(int index, const QString &name){
    if(index <0 || index >= layers.size()) return;
    layers[index].name = name;
}
void PixelCanvas::moveLayerUp(int index)
{
    if(index < 0 || index >= layers.size()-1)
        return;
    std::swap(layers[index], layers[index+1]);
    if(activeLayer == index)
        activeLayer++;
    else if(activeLayer == index+1)
        activeLayer--;
    update();
}
void PixelCanvas::moveLayerDown(int index)
{
    if(index <= 0 || index >= layers.size()) return;
    std::swap(layers[index], layers[index-1]);
    if(activeLayer == index)
        activeLayer--;
    else if(activeLayer == index-1)
        activeLayer++;
    update();
}
void PixelCanvas::setLayerOpacity(int index, float opacity){
    if (index < 0 || index >= layers.size()) return;
    layers[index].opacity = opacity;
    update();
}
float PixelCanvas::getLayerOpacity(int index) const{
    return layers[index].opacity;
}
void PixelCanvas::saveImage()
{
    QImage image(layers[0].width * pixelSize, layers[0].height * pixelSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    for(const auto &layer : layers)
    {
        if(!layer.visible) continue;
        for(int y = 0; y < layer.height; y++)
        {
            for(int x = 0; x < layer.width; x++)
            {
                QColor color = layer.at(x,y);
                if(color != Qt::transparent)
                {
                    QRect rect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
                    painter.fillRect(rect, color);
                }
            }
        }
    }

    QString fileName = QFileDialog::getSaveFileName(this,"Save Image","","PNG Files (*.png)");
    if(!fileName.isEmpty())
    {
        if(!fileName.endsWith(".png"))
            fileName += ".png";
        image.save(fileName);
    }
}
void PixelCanvas::saveProject()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Project", "", "Pixel Project (*.json)");
    if(fileName.isEmpty()) return;
    if(!fileName.endsWith(".json")) fileName += ".json";
    QJsonObject root;
    root["Width"] = layers[0].width;
    root["Height"] = layers[0].height;
    QJsonArray layerArray;

    for(const auto &layer : layers){
        QJsonObject layerObject;
        layerObject["name"] = layer.name;
        layerObject["visible"] = layer.visible;
        layerObject["opacity"] = layer.opacity;
        QJsonArray pixelMap;

        for(int y = 0; y < layer.height; y++){
            for(int x = 0; x < layer.width; x++){
                pixelMap.append(layer.at(x,y).name(QColor::HexArgb));
            }
        }
        layerObject["pixels"] = pixelMap;
        layerArray.append(layerObject);
    }
    root["layers"] = layerArray;
    QJsonDocument doc(root);
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}
void PixelCanvas::loadPicture()
{
    QString file = QFileDialog::getOpenFileName(this, "Import Reference", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if(file.isEmpty()) return;
    Layer layer;
    layer.type = LayerType::Reference;
    layer.name = QFileInfo(file).baseName();
    layer.image.load(file);
    layer.width = layers[0].width;
    layer.height = layers[0].height;
    layers.push_back(layer);
    activeLayer = layers.size()-1;
    update();
}
void PixelCanvas::loadProject()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Project", "", "Pixel Project (*.json)");
    if(fileName.isEmpty()) return;
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    int width = root["Width"].toInt();
    int height = root["Height"].toInt();

    if (root.contains("layers")){
        layers.clear();
        QJsonArray layerArray = root["layers"].toArray();

        for(auto layerValue : layerArray){
            QJsonObject layerObject = layerValue.toObject();
            Layer layer;
            layer.name = layerObject["name"].toString();
            layer.visible = layerObject["visible"].toBool();
            layer.opacity = layerObject["opacity"].toDouble(1.0);
            layer.width = width;
            layer.height = height;
            layer.pixels.resize(width * height);

            QJsonArray pixels = layerObject["pixels"].toArray();
            int index = 0;
            for(int y = 0; y < height; y++){
                for(int x = 0; x < width; x++){
                    layer.at(x,y) = QColor(pixels[index].toString());
                    index++;
                }
            }
            layers.push_back(layer);
        }
    }
    else{
        layers.clear();
        if (root.contains("gridSize")){
            width = root["gridSize"].toInt();
            height = width;
        }
        else {
            width = root["Width"].toInt();
            height = root["Height"].toInt();
        }
        if(layers.empty()){
            addLayer();
        }
        if(width > layers[activeLayer].width || height > layers[activeLayer].height){
            layers[activeLayer].width = width;
            layers[activeLayer].height = height;
            layers[activeLayer].pixels.resize(layers[activeLayer].width * layers[activeLayer].height);
            updateCanvasSize();
        }
        // this can be done better i know it lol
        QJsonArray pixelMap = root["pixels"].toArray();
        int index = 0;
        for(int y =0; y<height; y++){
            for(int x=0; x<width; x++){
                QString colorString = pixelMap[index].toString();
                layers[activeLayer].at(x, y) = QColor(colorString);
                index++;
            }
        }
    }
    activeLayer = 0;
    updateCanvasSize();
    update();
}
void PixelCanvas::setTool(Tool tool){
    currentTool = tool;
}
void PixelCanvas::floodFill(int startX, int startY){
    QColor target = layers[activeLayer].at(startX, startY);
    QColor fill = currentColor;
    if(target == fill) return;
    std::queue<QPoint> q;
    q.push(QPoint(startX, startY));
    while (!q.empty()){
        QPoint p = q.front();
        q.pop();
        int x = p.x();
        int y = p.y();
        if (x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height){
            if(layers[activeLayer].at(x, y) == target){
                currentAction.push_back({activeLayer, x, y, layers[activeLayer].at(x,y), currentColor});
                layers[activeLayer].at(x, y) = currentColor;
                q.push(QPoint(x+1, y));
                q.push(QPoint(x-1, y));
                q.push(QPoint(x, y+1));
                q.push(QPoint(x, y-1));
            }
        }


    }
}
void PixelCanvas::undo(){
    if(undoStack.empty()) return;

    auto action = undoStack.back();
    undoStack.pop_back();

    for(auto &change : action){
        layers[change.layer].at(change.x, change.y) = change.oldColor;
    }
    redoStack.push_back(action);
    update();
}
void PixelCanvas::redo(){
    if(redoStack.empty()) return;

    auto action = redoStack.back();
    redoStack.pop_back();

    for (auto &change : action){
        layers[change.layer].at(change.x, change.y) = change.newColor;
    }
    undoStack.push_back(action);
    update();
}
void PixelCanvas::undoActions(){

    redoStack.clear();
    if(undoStack.size() >= maxUndo){
        undoStack.pop_front();
        undoStack.push_back(currentAction);
    }
    else{
        undoStack.push_back(currentAction);
    }
}
QColor PixelCanvas::getColor(){
    return currentColor;
}