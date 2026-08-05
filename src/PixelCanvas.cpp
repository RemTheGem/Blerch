#include "PixelCanvas.h"
#include "tools/mediancut.h"
#include "dialogs/pictureimportdialog.h"
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

// widgets
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
ColorPreviewWidget::ColorPreviewWidget(QWidget *parent){
}
// paint events
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
    /*
     * draw grid
    for(int y = 0; y < height(); y += pixelSize)
    {
        painter.drawLine(0, y, width(), y);
    }
    for(int x = 0; x < width(); x += pixelSize)
    {
        painter.drawLine(x, 0, x, height());
    }
    */
    if(currentTool == Tool::Move || currentTool == Tool::Select){
    QPen pen;
    pen.setWidth(3);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    QPoint topLeft(std::min(selection.dragStart.x(), selection.previewEnd.x()),
                   std::min(selection.dragStart.y(), selection.previewEnd.y()));
    QPoint bottomRight(std::max(selection.dragStart.x(), selection.previewEnd.x()),
                       std::max(selection.dragStart.y(), selection.previewEnd.y()));
    QRect selectionRect(topLeft * pixelSize, (bottomRight + QPoint(1,1)) * pixelSize);
    painter.drawRect(selectionRect);
    }
    buildPalette();
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
void PixelCanvas::paintColor(int x, int y, const QColor &color, bool recordUndo)
{
    auto draw = [&](int px, int py){
        if (px >= 0 && px < layers[activeLayer].width &&
            py >= 0 && py < layers[activeLayer].height && layers[activeLayer].at(px, py) != color){
            if(recordUndo){
                currentAction.push_back({activeLayer, px, py, layers[activeLayer].at(px, py), color});
                qDebug() << "recorded undo";
            }
            layers[activeLayer].at(px, py) = color;
        }
    };
    int radius = brushSize / 2;
    for (int offsetY = -radius; offsetY <= radius; offsetY++){
        for (int offsetX = -radius; offsetX <= radius; offsetX++){

            // for a circular brush
            // if(offsetX*offsetX + offsetY*offsetY > radius*radius)
            //     continue;
            int pixelX = x + offsetX;
            int pixelY = y + offsetY;
            int mirrorX = layers[activeLayer].width  - 1 - x;
            int mirrorY = layers[activeLayer].height - 1 - y;
            draw(pixelX, pixelY);
            if (horizontalSymmetry)
                draw(mirrorX, pixelY);

            if (verticalSymmetry)
                draw(pixelX, mirrorY);

            if (horizontalSymmetry && verticalSymmetry)
                draw(mirrorX, mirrorY);
        }
    }
}
// canvas methods
void PixelCanvas::updateCanvasSize()
{
    setFixedSize(layers[activeLayer].width *pixelSize, layers[activeLayer].height *pixelSize);
    update();
}
void PixelCanvas::resizeCanvas(int width, int height)
{
    Layer &layer = layers[activeLayer];
    // old width and height to redraw the previous canvas
    QVector<QColor> oldPixels = layer.pixels;
    int oldWidth = layer.width;
    int oldHeight = layer.height;
    // these ones so that when we add new layers they knew what size to be
    canvasHeight = height;
    canvasWidth = width;
    layer.width = width;
    layer.height = height;
    layer.pixels.assign(width * height, Qt::transparent);
    for(int y = 0; y < std::min(oldHeight, height); y++){
        for(int x = 0; x < std::min(oldWidth, width); x++){
            layer.pixels[y * width + x] = oldPixels[y * oldWidth + x];
        }
    }
    updateCanvasSize();
    update();
}
// mouse events
void PixelCanvas::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    if(layers[activeLayer].type == LayerType::Reference){
        if(event->button() == Qt::LeftButton){
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
            paintColor(x, y, currentColor);
            break;
        case Tool::Eraser:
            paintColor(x, y, Qt::transparent);
            break;
        case Tool::EyeDropper:
            currentColor = layers[activeLayer].at(x, y);
            emit colorChanged(currentColor);
            break;
        case Tool::Fill:
            floodFill(x, y);
            break;
        case Tool::Select:
        {
            selection = Selection();
            selection.dragStart = QPoint(x, y);
            break;
        }
        case Tool::Move:
            if(selection.isEmpty(selection)) return;
            selection.dragOffset = event->pos();
            selection.dragging = true;
            if(!selection.moveFloating){
                makeTempLayer();
                selection.moveFloating = true;
            }
            break;
        case Tool::Shape:{
            shape = Shape();
            shape.start = QPoint(x, y);
            makeTempLayer();
            break;
        }
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
                paintColor(x, y, Qt::transparent);
                shape = Shape();
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
                paintColor(x, y, currentColor);
                changed = true;
            }
            break;
        case Tool::Eraser:
            if(layers[activeLayer].at(x, y) != Qt::transparent)
            {
                paintColor(x, y, Qt::transparent);
                changed = true;
            }
            break;
        case Tool::Select:
        {
            int eventX = event->position().x() / pixelSize;
            int eventY = event->position().y() / pixelSize;
            eventX = std::clamp(eventX, 0, layers[activeLayer].width - 1);
            eventY = std::clamp(eventY, 0, layers[activeLayer].height - 1);
            selection.previewEnd = QPoint(eventX, eventY);
            update();
            break;
        }
        case Tool::Move:
        {
            if(selection.isEmpty(selection)) return;
            clear();
            selection.selectionOffset = ((event->pos() - selection.dragOffset)/pixelSize) + QPoint(selection.dragStart.x(), selection.dragStart.y());
            for (int my = 0; my < selection.height+1; my++){
                for (int mx = 0; mx < selection.width+1; mx++){
                    int canvasX = selection.selectionOffset.x() + mx;
                    int canvasY = selection.selectionOffset.y() + my;
                    int index = (selection.width+1) * my + mx;
                    if (index >= selection.colors.size()) continue;
                    if (canvasX < 0 || canvasX >= layers[activeLayer].width) continue;
                    if (canvasY < 0 || canvasY >= layers[activeLayer].height) continue;
                    layers[activeLayer].at(canvasX, canvasY) = selection.colors.at(index);
                }
            }
            update();
            break;
        }
        case Tool::Shape:{
            clear();
            shape.end = event->pos()/pixelSize;
            QPoint topLeft(std::min(shape.start.x(), shape.end.x()), std::min(shape.start.y(), shape.end.y()));
            QPoint bottomRight(std::max(shape.start.x(), shape.end.x()), std::max(shape.start.y(),shape.end.y()));
            shape.width = shape.end.x() - shape.start.x();
            shape.height = shape.end.y() - shape.start.y();
            if(currentShape == ShapeType::Rectangle){
                drawRectangle(topLeft, bottomRight, false);
            }
            if(currentShape == ShapeType::Circle){
                drawCircle(topLeft, bottomRight, false);
            }
            if(currentShape == ShapeType::Line){
                drawLine(shape.start, shape.end, false);
            }
            update();
            break;
        }
        }
    }
    }
    else{
        if(x >= 0 && x < layers[activeLayer].width && y >= 0 && y < layers[activeLayer].height){
            if (layers[activeLayer].at(x, y) != Qt::transparent){
                paintColor(x, y, Qt::transparent);
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
        layers[activeLayer].position = (event->pos() - moveOffset) / pixelSize;
        update();
    }
    switch(currentTool){
    case Tool::Select:
    {
        int eventX = event->position().x()/ pixelSize;
        int eventY = event->position().y()/pixelSize;
        eventX = std::clamp(eventX, 0, layers[activeLayer].width -1);
        eventY = std::clamp(eventY, 0, layers[activeLayer].height -1);
        selection.dragEnd = QPoint(eventX, eventY);
        QPoint topLeft(std::min(selection.dragStart.x(), selection.dragEnd.x()), std::min(selection.dragStart.y(),selection.dragEnd.y()));
        QPoint bottomRight(std::max(selection.dragStart.x(), selection.dragEnd.x()), std::max(selection.dragStart.y(),selection.dragEnd.y()));
        selection.dragStart = topLeft;
        selection.dragEnd = bottomRight;
        selection.width = selection.dragEnd.x() - selection.dragStart.x();
        selection.height = selection.dragEnd.y() - selection.dragStart.y();
        for(int sy = selection.dragStart.y(); sy<=selection.height+selection.dragStart.y(); sy++){
            for(int sx = selection.dragStart.x(); sx<=selection.width+selection.dragStart.x(); sx++){
                selection.colors.push_back(layers[activeLayer].at(sx,sy));
            }
        }
        update();
        break;
    }
    case Tool::Move:
        selection.dragging = false;
        if(selection.isEmpty(selection)) return;

        break;
    case Tool::Shape:
    {
        removeTempLayer();
        if(shape.end == QPoint(0,0) && shape.start == QPoint(0,0)){
            update();
            break;
        }
        shape.end = event->pos()/pixelSize;
        QPoint topLeft(std::min(shape.start.x(), shape.end.x()), std::min(shape.start.y(), shape.end.y()));
        QPoint bottomRight(std::max(shape.start.x(), shape.end.x()), std::max(shape.start.y(),shape.end.y()));
        shape.width = shape.end.x() - shape.start.x();
        shape.height = shape.end.y() - shape.start.y();
        if(currentShape == ShapeType::Rectangle){
            drawRectangle(topLeft, bottomRight);
        }

        else if(currentShape == ShapeType::Circle){
            drawCircle(topLeft, bottomRight);
        }
        else if(currentShape == ShapeType::Line){
            drawLine(shape.start, shape.end);
        }
        update();
        break;

    }
    }

    if(!currentAction.empty()){
        undoStack.push_back(currentAction);
        redoStack.clear();
    }
}
// key press events
void PixelCanvas::keyPressEvent(QKeyEvent *event){
    if(currentTool == Tool::Move && selection.moveFloating){
        if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter){
            commitMove();
            return;
        }
        if(event->key() == Qt::Key_Escape){
            cancelMove();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}
// helper methods
void PixelCanvas::drawRectangle(QPoint topLeft, QPoint bottomRight, bool recordUndo){
    for(int sx = topLeft.x(); sx <= bottomRight.x(); sx++){
        for(int sy = topLeft.y(); sy <= bottomRight.y(); sy++){
            if(sx < 0 || sx >= layers[activeLayer].width) continue;
            if(sy < 0 || sy >= layers[activeLayer].height) continue;
            if(sx == topLeft.x() || sx == bottomRight.x() || sy == topLeft.y() || sy == bottomRight.y()){
                paintColor(sx,sy, currentColor, recordUndo);
            }
        }
    }
}
void PixelCanvas::drawCircle(QPoint topLeft, QPoint bottomRight, bool recordUndo){
    // midpoint circle algorithm stolen from the internet
    int centerX = (topLeft.x() + bottomRight.x())/2;
    int centerY = (topLeft.y() + bottomRight.y())/2;
    int radiusX = (bottomRight.x()-topLeft.x())/2;
    int radiusY = (bottomRight.y()-topLeft.y())/2;
    for(int cx = -radiusX; cx <=radiusX;cx++){
        for(int cy = -radiusY; cy <= radiusY;cy++){
            if((cx*cx*radiusY*radiusY) + (cy*cy*radiusX*radiusX) <= (radiusX*radiusX*radiusY*radiusY)){
                int px = centerX + cx;
                int py = centerY + cy;
                if(px < 0 || px >= layers[activeLayer].width) continue;
                if(py < 0 || py >= layers[activeLayer].height) continue;
                paintColor(px,py,currentColor, recordUndo);

            }
        }
    }
}
void PixelCanvas::drawLine(QPoint start, QPoint end, bool recordUndo)
{
    // Bresenham's algorithm stolen from the internet
    int x0 = start.x();
    int y0 = start.y();
    int x1 = end.x();
    int y1 = end.y();
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while(true){
        if(x0 >= 0 && x0 < layers[activeLayer].width && y0 >= 0 && y0 < layers[activeLayer].height){
            paintColor(x0, y0, currentColor, recordUndo);
        }
        if(x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if(e2 >= dy){
            err += dy;
            x0 += sx;
        }
        if(e2 <= dx){
            err += dx;
            y0 += sy;
        }
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
    buildPalette();
    update();
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
                paintColor(x, y, currentColor);
                q.push(QPoint(x+1, y));
                q.push(QPoint(x-1, y));
                q.push(QPoint(x, y+1));
                q.push(QPoint(x, y-1));
            }
        }


    }
}
void PixelCanvas::buildPalette(){
    colorFrequency.clear();
    for (const auto &layer : layers){
        if (layer.type != LayerType::Pixel) continue;
        for (const QColor &color : layer.pixels){
            if (color == Qt::transparent) continue;
                colorFrequency[color.rgba()]++;
        }
    }
    emit paletteUpdated(sortColors(colorFrequency));
}
QList<QColor> PixelCanvas::sortColors(QHash<QRgb, int> colorFrequency){
    QList<QPair<QRgb,int>> pairs;
    for(auto it = colorFrequency.begin(); it != colorFrequency.end(); ++it){
        pairs.append({it.key(), it.value()});
    }
    std::sort(pairs.begin(), pairs.end(), [](auto a, auto b){
        return a.second > b.second;});
    QList<QColor> result;
    for(const auto &pair :pairs){
        result.append(pair.first);
    }
    return result;
}
void PixelCanvas::commitMove(){
    removeTempLayer();
    qDebug() << "move committed";
    QRect destRect(selection.selectionOffset.x(), selection.selectionOffset.y(),
                   selection.width+1, selection.height+1);
    for (int my = 0; my < selection.height+1; my++){
        for (int mx = 0; mx < selection.width+1; mx++){
            int canvasX = selection.selectionOffset.x() + mx;
            int canvasY = selection.selectionOffset.y() + my;
            int index = (selection.width+1) * my + mx;
            if (index >= selection.colors.size()) continue;
            if (canvasX < 0 || canvasX >= layers[activeLayer].width) continue;
            if (canvasY < 0 || canvasY >= layers[activeLayer].height) continue;
            if(selection.colors.at(index) == Qt::transparent) continue;
            paintColor(canvasX, canvasY,selection.colors.at(index));
            //layers[activeLayer].at(canvasX, canvasY) = selection.colors.at(index);
            selection.previewEnd = QPoint(canvasX, canvasY);
        }
    }
    for (int my = 0; my < selection.height+1; my++){
        for (int mx = 0; mx < selection.width+1; mx++){
            int oldX = selection.dragStart.x() + mx;
            int oldY = selection.dragStart.y() + my;
            int index = (selection.width+1) * my + mx;
            if (index >= selection.colors.size()) continue;
            if (oldX < 0 || oldX >= layers[activeLayer].width) continue;
            if (oldY < 0 || oldY >= layers[activeLayer].height) continue;
            if(selection.colors.at(index) == Qt::transparent) continue;
            if(destRect.contains(oldX, oldY)) continue;
            paintColor(oldX,oldY,Qt::transparent);
            // layers[activeLayer].at(oldX, oldY) = Qt::transparent;
        }
    }
    selection.dragging = false;
    selection.moveFloating = false;
    selection.setValues(selection);
    /*
    if(!currentAction.empty()){
        undoStack.push_back(currentAction);
        redoStack.clear();
    }
    */
    update();
}
void PixelCanvas::cancelMove(){
    removeTempLayer();
    selection.moveFloating = false;
    selection.dragging = false;
    update();
}
// layer methods
void PixelCanvas::makeTempLayer(){
    addLayer();
    setActiveLayer(layers.size()-1);
    layers[activeLayer].opacity = 0.5f;
    layers[activeLayer].width = canvasWidth;
    layers[activeLayer].height = canvasHeight;
    qDebug() << "current width" << layers[activeLayer].width;
}
void PixelCanvas::removeTempLayer(){
    removeLayer(layers.size()-1);
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
    buildPalette();
    update();
}
void PixelCanvas::removeLayer(int index){
    if(layers.size() <= 1) return;
    layers.erase(layers.begin()+index);
    activeLayer = std::clamp(activeLayer,0,(int)layers.size()-1);
    update();
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
// File manipulation
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
                paintColor(x, y, QColor(colorString));
                index++;
            }
        }
    }
    activeLayer = 0;
    updateCanvasSize();
    resizeCanvas(width, height);
    buildPalette();
    update();
}
void PixelCanvas::pictureToPixel(){
    QString file = QFileDialog::getOpenFileName(this, "Import Picture", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if(file.isEmpty()) return;
    Layer layer;
    layer.type = LayerType::Pixel;
    layer.name = QFileInfo(file).baseName();
    MedianCut medianCut;
    PictureImportDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    int targetWidth = dialog.width();
    int targetHeight = dialog.height();
    int paletteSize = dialog.colors();
    QImage image(file);
    if(dialog.keepAspect()){
        image = image.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    else {
        image = image.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    layer.width = image.width();
    layer.height = image.height();
    canvasWidth = image.width();
    canvasHeight = image.height();
    resizeCanvas(canvasWidth, canvasHeight);
    updateCanvasSize();
    auto palette = medianCut.medianCut(image, paletteSize);
    layer.pixels.resize(canvasWidth * canvasHeight);
    for (int y = 0; y < canvasHeight; y++) {
        for (int x = 0; x < canvasWidth; x++) {
            QColor mapped = medianCut.nearestColor(image.pixelColor(x, y), palette);
            paintColor(x, y, mapped);
        }
    }
    update();
}
// undo and redo
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
// getters
QColor PixelCanvas::getColor(){
    return currentColor;
}
int PixelCanvas::getZoom(){
    return pixelSize;
}
QStringList PixelCanvas::getLayerNames(){
    QStringList names;
    for(const auto &layer : layers) names.append(layer.name);
    return names;
}
float PixelCanvas::getLayerOpacity(int index) const{
    return layers[index].opacity;
}
// setters
void ColorPreviewWidget::setPreviewColor(const QColor &color){
    selectedColor = color;
    update();
}
void PixelCanvas::setHorizontalSymmetry(bool enabled){
    horizontalSymmetry = enabled;
    update();
}
void PixelCanvas::setVerticalSymmetry(bool enabled){
    verticalSymmetry = enabled;
    update();
}
void PixelCanvas::setZoom(int zoom){
    pixelSize = zoom;
    updateCanvasSize();
    update();
}
void PixelCanvas::setActiveLayer(int index){
    if(index >= 0 && index < layers.size()){
        activeLayer = index;
        update();
    }
}
void PixelCanvas::setLayerOpacity(int index, float opacity){
    if (index < 0 || index >= layers.size()) return;
    layers[index].opacity = opacity;
    update();
}
void PixelCanvas::setTool(Tool tool){
    cancelMove();
    currentTool = tool;
}
void PixelCanvas::setShape(ShapeType shape){
    currentShape = shape;
}
void PixelCanvas::setBrushSize(int newSize){
    brushSize = newSize;
}