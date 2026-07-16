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
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            currentState.pixels[y][x] = Qt::white;
        }
    }
    undoStack.push_back(currentState);
    setMouseTracking(true);
}
void PixelCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {

            QRect rect(
                x * pixelSize,
                y * pixelSize,
                pixelSize,
                pixelSize
                );

            painter.fillRect(rect, currentState.pixels[y][x]);
            painter.drawRect(rect);
        }
    }
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
    if(event->button() == Qt::LeftButton){
    isDrawing = true;
    // make sure you don't overwrite on the old canvas. if you do, it leads to both actions being on the same canvas and any subsequent undos undoes both.
    if(isUndoing){
        undoStack.push_back(currentState);
        isUndoing = false;
    }


    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;

    if (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
        switch(currentTool){
        case Tool::Brush:
            currentState.pixels[y][x] = currentColor;
            break;
        case Tool::Eraser:
            currentState.pixels[y][x] = Qt::white;
            break;
        case Tool::EyeDropper:
            currentColor = currentState.pixels[y][x];
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
                undoStack.push_back(currentState);
                isUndoing = false;
            }


            int x = event->position().x() / pixelSize;
            int y = event->position().y() / pixelSize;

            if (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
                currentState.pixels[y][x] = Qt::white;
            }

            update();
        }

}


void PixelCanvas::mouseMoveEvent(QMouseEvent *event)
{

    if (!isDrawing) return;

    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;
    if(!isErasing){
    if (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
        switch(currentTool){
        case Tool::Brush:
            currentState.pixels[y][x] = currentColor;
            break;
        case Tool::Eraser:
            currentState.pixels[y][x] = Qt::white;
            break;
        }

        update();
    }
    }
    else{
        if(x >= 0 && x < gridSize && y >= 0 && y < gridSize){
            currentState.pixels[y][x] = Qt::white;
        }
        update();
    }

}
void PixelCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    isDrawing = false;
    isErasing = false;
    undoActions();
}
void PixelCanvas::clear()
{
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            currentState.pixels[y][x] = Qt::white;
        }
    }
    update();
}
void PixelCanvas::saveImage()
{
    QImage image(gridSize * pixelSize, gridSize * pixelSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);

    for (int y = 0; y<gridSize; y++){
        for (int x = 0; x<gridSize; x++){
            QRect rect(
                x * pixelSize,
                y * pixelSize,
                pixelSize,
                pixelSize);
            painter.fillRect(rect, currentState.pixels[y][x]);
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

    for(int y =0; y <gridSize; y++){
        for (int x =0; x <gridSize; x++){
            QColor color = currentState.pixels[y][x];

            pixelMap.append(color.name());

        }
    }
    root["gridSize"] = gridSize;
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
    QJsonArray pixelMap = root["pixels"].toArray();

    int index = 0;
    for(int y =0; y<gridSize; y++){
        for(int x=0; x<gridSize; x++){
            QString colorString = pixelMap[index].toString();
            currentState.pixels[y][x] = QColor(colorString);
            index++;
        }
    }
    update();
}
void PixelCanvas::setTool(Tool tool){
    currentTool = tool;
}
void PixelCanvas::floodFill(int startX, int startY){
    QColor target = currentState.pixels[startY][startX];
    QColor fill = currentColor;
    if(target == fill) return;
    std::queue<QPoint> q;
    q.push(QPoint(startX, startY));
    while (!q.empty()){
        QPoint p = q.front();
        q.pop();

        int x = p.x();
        int y = p.y();
        if (x >= 0 && x < gridSize && y >= 0 && y < gridSize){
            if(currentState.pixels[y][x] == target){
                currentState.pixels[y][x] = currentColor;
                q.push(QPoint(x+1, y));
                q.push(QPoint(x-1, y));
                q.push(QPoint(x, y+1));
                q.push(QPoint(x, y-1));
            }
        }


    }
}
void PixelCanvas::undo(){
    if(undoStack.size() > 1){
        isUndoing = true;
    redoStack.push_back(currentState);
    undoStack.pop_back();
    currentState = undoStack.back();
    }
    update();
}
void PixelCanvas::redo(){
    if(!redoStack.empty()){
        currentState = redoStack.back();
        undoStack.push_back(currentState);
        redoStack.pop_back();
    }
    update();
}
void PixelCanvas::undoActions(){

    redoStack.clear();
    if(undoStack.size() >= maxUndo){
        undoStack.pop_front();
        undoStack.push_back(currentState);
    }
    else{
        undoStack.push_back(currentState);
    }
}
QColor PixelCanvas::getColor(){
    return currentColor;
}