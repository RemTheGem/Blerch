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


PixelCanvas::PixelCanvas(QWidget *parent)
    : QWidget(parent)
{
    currentState.width = canvasWidth;
    currentState.height = canvasHeight;
    currentState.pixels.resize(canvasWidth * canvasHeight);
    for (int y = 0; y < currentState.height; y++) {
        for (int x = 0; x < currentState.width; x++) {
            currentState.at(x,y) = Qt::transparent;
        }
    }
    undoStack.push_back(currentAction);
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
    for (int y = 0; y < currentState.height; y++) {
        for (int x = 0; x < currentState.width; x++) {
            QRect rect(
                x * pixelSize,
                y * pixelSize,
                pixelSize,
                pixelSize
                );
            if (currentState.at(x,y) != Qt::transparent) {
                painter.fillRect(rect, currentState.at(x,y));
            }
            painter.drawRect(rect);
        }
    }
}
void PixelCanvas::updateCanvasSize()
{
    setFixedSize(currentState.width *pixelSize, currentState.height *pixelSize);
    update();
}
void PixelCanvas::resizeCanvas(int width, int height)
{
    CanvasState newState;
    newState.width = width;
    newState.height = height;
    newState.pixels.resize(width * height);

    for(auto &pixel : newState.pixels)
        pixel = Qt::transparent;

    currentState = newState;

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

    if (x >= 0 && x < currentState.width && y >= 0 && y < currentState.height) {
        switch(currentTool){
        case Tool::Brush:
            currentAction.push_back({x, y, currentState.at(x,y), currentColor});
            currentState.at(x, y) = currentColor;
            break;
        case Tool::Eraser:
            currentAction.push_back({x, y, currentState.at(x,y), Qt::transparent});
            currentState.at(x, y) = Qt::transparent;
            break;
        case Tool::EyeDropper:
            currentColor = currentState.at(x, y);
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

            if (x >= 0 && x < currentState.width && y >= 0 && y < currentState.height) {
                currentAction.push_back({x, y, currentState.at(x,y), Qt::transparent});
                currentState.at(x, y) = Qt::transparent;
            }

            update();
        }

}


void PixelCanvas::mouseMoveEvent(QMouseEvent *event)
{

    if (!isDrawing) return;

    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;
    bool changed = false;
    if(!isErasing){
    if (x >= 0 && x < currentState.width && y >= 0 && y < currentState.height) {
        switch(currentTool){
        case Tool::Brush:
            if(currentState.at(x, y) != currentColor)
            {
                currentAction.push_back({x, y, currentState.at(x,y), currentColor});
                currentState.at(x, y) = currentColor;
                changed = true;
            }
            break;
        case Tool::Eraser:
            if(currentState.at(x, y) != Qt::transparent)
            {
                currentAction.push_back({x, y, currentState.at(x,y), Qt::transparent});
                currentState.at(x, y) = Qt::transparent;
                changed = true;
            }
            break;
        }  
    }
    }
    else{
        if(x >= 0 && x < currentState.width && y >= 0 && y < currentState.height){
            if (currentState.at(x, y) != Qt::transparent){
                currentAction.push_back({x, y, currentState.at(x,y), Qt::transparent});
                currentState.at(x, y) = Qt::transparent;
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
    if(!currentAction.empty()){
        undoStack.push_back(currentAction);
        redoStack.clear();
    }
}
void PixelCanvas::clear()
{
    for (int y = 0; y < currentState.height; y++) {
        for (int x = 0; x < currentState.width; x++) {
            currentState.at(x, y) = Qt::transparent;
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
void PixelCanvas::saveImage()
{
    QImage image(currentState.width * pixelSize, currentState.height * pixelSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);

    for (int y = 0; y<currentState.height; y++){
        for (int x = 0; x<currentState.width; x++){
            QRect rect(
                x * pixelSize,
                y * pixelSize,
                pixelSize,
                pixelSize);
            painter.fillRect(rect, currentState.at(x, y));
        }
    }
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Image",
        "",
        "PNG Files (*.png)");
    if(!fileName.isEmpty()){
        if(!fileName.endsWith(".png")){
            fileName += ".png";
        }
        image.save(fileName);
    }
}
void PixelCanvas::saveProject(){
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Project",
        "",
        "Pixel Project (*.json)");
    if(!fileName.isEmpty()){
        if(!fileName.endsWith(".json")){
            fileName += ".json";
        }
    }
    else return;

    QJsonObject root;
    QJsonArray pixelMap;

    for(int y =0; y <currentState.height; y++){
        for (int x =0; x <currentState.width; x++){
            QColor color = currentState.at(x, y);

            pixelMap.append(color.name(QColor::HexArgb));

        }
    }
    root["Height"] = currentState.height;
    root["Width"] = currentState.width;
    root["pixels"] = pixelMap;

    QJsonDocument doc(root);
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly)){
        file.write(doc.toJson());
        file.close();
    }
}
void PixelCanvas::loadProject(){
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load Project",
        "",
        "Pixel Project (*.json)");
    if(fileName.isEmpty()) return;

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    int fileWidth;
    int fileHeight;
    if (root.contains("gridSize")){
        fileWidth = root["gridSize"].toInt();
        fileHeight = fileWidth;
    }
    else {
        fileWidth = root["Width"].toInt();
        fileHeight = root["Height"].toInt();
    }
    if(fileWidth > currentState.width || fileHeight > currentState.height){
    currentState.width = fileWidth;
    currentState.height = fileHeight;
    currentState.pixels.resize(currentState.width * currentState.height);
    updateCanvasSize();
    }
    // this can be done better i know it lol
    QJsonArray pixelMap = root["pixels"].toArray();

    int index = 0;
    for(int y =0; y<fileHeight; y++){
        for(int x=0; x<fileWidth; x++){
            QString colorString = pixelMap[index].toString();
            currentState.at(x, y) = QColor(colorString);
            index++;
        }
    }
    updateCanvasSize();
    update();
}
void PixelCanvas::setTool(Tool tool){
    currentTool = tool;
}
void PixelCanvas::floodFill(int startX, int startY){
    QColor target = currentState.at(startX, startY);
    QColor fill = currentColor;
    if(target == fill) return;
    std::queue<QPoint> q;
    q.push(QPoint(startX, startY));
    while (!q.empty()){
        QPoint p = q.front();
        q.pop();

        int x = p.x();
        int y = p.y();
        if (x >= 0 && x < currentState.width && y >= 0 && y < currentState.height){
            if(currentState.at(x, y) == target){
                currentState.at(x, y) = currentColor;
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
        currentState.at(change.x, change.y) = change.oldColor;
    }
    redoStack.push_back(action);
    update();
}
void PixelCanvas::redo(){
    if(redoStack.empty()) return;

    auto action = redoStack.back();
    redoStack.pop_back();

    for (auto &change : action){
        currentState.at(change.x, change.y) = change.newColor;
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