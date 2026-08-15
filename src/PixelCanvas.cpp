#include "PixelCanvas.h"
#include "tools/mediancut.h"
#include "dialogs/pictureimportdialog.h"
#include "../thirdParty/gif-h/gif.h"
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
    document = new CanvasDocument(this);
    connect(document, &CanvasDocument::documentMutated, this, [this]{
        selection = Selection();
        update();
    });
    connect(document, &CanvasDocument::layerChanged, this,[this]{update();});
    connect(document, &CanvasDocument::frameChanged, this, [this]{update();});
    connect(document, &CanvasDocument::canvasSizeChanged, this, [this]{updateCanvasSize();});
    updateCanvasSize();
    setMouseTracking(true);
}

// paint events
void PixelCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // draw the checkered background
    for (int y = 0; y < height(); y += pixelSize) {
        for (int x = 0; x < width(); x += pixelSize) {
            bool dark = ((x / pixelSize) + (y / pixelSize)) % 2;
            if (dark)
                painter.fillRect(x, y, pixelSize, pixelSize, QColor(224, 224, 224));
            else
                painter.fillRect(x, y, pixelSize, pixelSize, QColor(176, 176, 176));
        }
    }
    // draw all layers
    for (const auto &layer : document->currentFrame_().layers)
    {
        if(!layer.visible) continue;
        painter.save();
        painter.setOpacity(layer.opacity);
        // if its a pixel layer draw pixels
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
        // else if its a reference image then draw image
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
    // if we are using move or select then draw the outline for selection
    if((currentTool == Tool::Move || currentTool == Tool::Select) && selection.canMove){
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
    qDebug() << selection.previewEnd << selection.dragStart;
    }
    // build color palette according to the colors used
    document->buildPalette();
}
// draw stuff with coordinates and color as parameters
void PixelCanvas::paintColor(int x, int y, const QColor &color, bool recordUndo)
{
    // if we want to undo the drawing later (skip when drawing previews)
    auto draw = [&](int px, int py){
        if (px >= 0 && px < document->activeLayer_().width &&
            py >= 0 && py < document->activeLayer_().height && document->activeLayer_().at(px, py) != color){
            if(recordUndo){
                currentAction.push_back({document->getActiveLayer(), px, py, document->activeLayer_().at(px, py), color});
            }
            document->activeLayer_().at(px, py) = color;
        }
    };
    int radius = brushSize / 2;
    for (int offsetY = -radius; offsetY <= radius; offsetY++){
        for (int offsetX = -radius; offsetX <= radius; offsetX++){
            int pixelX = x + offsetX;
            int pixelY = y + offsetY;
            int mirrorX = document->activeLayer_().width  - 1 - pixelX;
            int mirrorY = document->activeLayer_().height - 1 - pixelY;
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
    setFixedSize(document->getCanvasWidth()*pixelSize, document->getCanvasHeight()*pixelSize);
    update();
}
void PixelCanvas::resizeCanvas(int width, int height)
{
    document->resizeCanvas(width, height);
    updateCanvasSize();
}
// mouse events
void PixelCanvas::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    // if the current layer is
    if(document->activeLayer_().type == LayerType::Reference){
        if(event->button() == Qt::LeftButton){
            movingPicture = true;
            moveOffset = event->pos() - document->activeLayer_().position;
        }
        return;
    }
    currentAction.clear();
    if(isPasting) return;
    // dont think we need an else statement here ###### check and fix
    else{
    if(event->button() == Qt::LeftButton){
    isDrawing = true;
    // make sure you don't overwrite on the old canvas. if you do, it leads to both actions being on the same canvas and any subsequent undos undoes both.
    //if(isUndoing){
    //    undoStack.push_back(currentAction);
    //    isUndoing = false;
    //}

    // coordinates of the cursor with respect to our canvas
    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;
    // quick boundary check
    if (x >= 0 && x < document->activeLayer_().width && y >= 0 && y < document->activeLayer_().height) {
        switch(currentTool){
        case Tool::Brush:
            paintColor(x, y, currentColor);
            break;
        case Tool::Eraser:
            paintColor(x, y, Qt::transparent);
            break;
        case Tool::EyeDropper:
            currentColor = document->activeLayer_().at(x, y);
            emit colorChanged(currentColor);
            break;
        case Tool::Fill:
            floodFill(x, y);
            break;
        case Tool::Select:
        {
            // clear previous selection and start a new one
            selection = Selection();
            selection.canMove = true;
            selection.dragStart = QPoint(x, y);
            break;
        }
        case Tool::Move:
        {
            // make sure we have a selection
            if(selection.isEmpty(selection) || !selection.canMove) return;
            selection.dragOffset = event->pos();
            selection.dragging = true;
            if(!selection.moveFloating){
                makeTempLayer();
                selection.moveFloating = true;
            }
            break;
        }
        case Tool::Shape:{
            // clear previous shapes details and start a new one
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
            //if(isUndoing){
            //    undoStack.push_back(currentAction);
            //    isUndoing = false;
            //}


            int x = event->position().x() / pixelSize;
            int y = event->position().y() / pixelSize;

            if (x >= 0 && x < document->activeLayer_().width && y >= 0 && y < document->activeLayer_().height) {
                paintColor(x, y, Qt::transparent);
                shape = Shape();
            }

            update();
        }
           else if(event->button() == Qt::MiddleButton){
            int x = event->position().x() / pixelSize;
            int y = event->position().y() / pixelSize;
            if (x >= 0 && x < document->activeLayer_().width && y >= 0 && y < document->activeLayer_().height) {
                currentColor = document->activeLayer_().at(x, y);
                // send signal to update the selected color preview
                emit colorChanged(currentColor);
            }
            }
    }
}
void PixelCanvas::wheelEvent(QWheelEvent *event)
{
    // for reference picture make it smaller or bigger
    if(event->modifiers() & Qt::ShiftModifier){
        if(document->activeLayer_().type == LayerType::Reference){
            if(event->angleDelta().y() > 0){
                document->activeLayer_().scale *= 1.1f;
            }
            else document->activeLayer_().scale *= 0.9f;
            document->activeLayer_().scale = std::clamp(document->activeLayer_().scale, 0.001f, 10.0f);
            update();
        }
        return;
    }
    // for normal layers zoom in and out
    if(event->modifiers() & Qt::ControlModifier){
        if(event->angleDelta().y() > 0)
            setZoom(pixelSize + 2);
        else
            setZoom(std::max(2, pixelSize -2));
        return;
    }
    QWidget::wheelEvent(event);
}
void PixelCanvas::mouseMoveEvent(QMouseEvent *event)
{
    // send signal to update x and y on status bar
    emit mousePositionChanged((event->position().x()/pixelSize)+1, (event->position().y()/pixelSize)+1);
    // for moving reference picture
    if(movingPicture){
        document->activeLayer_().position = (event->pos() - moveOffset) / pixelSize;
        update();
        return;
    }
    // draw a preview of copied selection when pasting
    if(isPasting){
        if(selection.isEmpty(selection)) return;
        clear();
        selection.movePosition = QPoint(event->position().x()/pixelSize, event->position().y()/pixelSize);
        for (int py = 0; py < selection.height+1; py++){
            for (int px = 0; px < selection.width+1; px++){
                int canvasX = selection.movePosition.x() - (selection.width/2) + px;
                int canvasY = selection.movePosition.y() - (selection.height/2) + py;
                int index = (selection.width+1) * py + px;
                if (index >= selection.colors.size()) continue;
                if (canvasX < 0 || canvasX >= document->activeLayer_().width) continue;
                if (canvasY < 0 || canvasY >= document->activeLayer_().height) continue;
                document->activeLayer_().at(canvasX, canvasY) = selection.colors.at(index);
            }
        }
        update();
        return;
    }
    if (!isDrawing) return;
    int x = event->position().x() / pixelSize;
    int y = event->position().y() / pixelSize;
    bool changed = false;
    if(!isErasing){
    if (x >= 0 && x < document->activeLayer_().width && y >= 0 && y < document->activeLayer_().height) {
        switch(currentTool){
        case Tool::Brush:
            if(document->activeLayer_().at(x, y) != currentColor)
            {
                paintColor(x, y, currentColor);
                changed = true;
            }
            break;
        case Tool::Eraser:
            if(document->activeLayer_().at(x, y) != Qt::transparent)
            {
                paintColor(x, y, Qt::transparent);
                changed = true;
            }
            break;
        case Tool::Select:
        {
            int eventX = event->position().x() / pixelSize;
            int eventY = event->position().y() / pixelSize;
            eventX = std::clamp(eventX, 0, document->activeLayer_().width - 1);
            eventY = std::clamp(eventY, 0, document->activeLayer_().height - 1);
            selection.previewEnd = QPoint(eventX, eventY);
            update();
            break;
        }
        case Tool::Move:
        {
            // draw a preview of where the moved selection is
            if(selection.isEmpty(selection) || !selection.canMove) return;

            clear();
            selection.selectionOffset = ((event->pos() - selection.dragOffset)/pixelSize) + QPoint(selection.topLeft.x(), selection.topLeft.y());
            for (int my = 0; my < selection.height+1; my++){
                for (int mx = 0; mx < selection.width+1; mx++){
                    int canvasX = selection.selectionOffset.x() + mx;
                    int canvasY = selection.selectionOffset.y() + my;
                    int index = (selection.width+1) * my + mx;
                    if (index >= selection.colors.size()) continue;
                    if (canvasX < 0 || canvasX >= document->activeLayer_().width) continue;
                    if (canvasY < 0 || canvasY >= document->activeLayer_().height) continue;
                    document->activeLayer_().at(canvasX, canvasY) = selection.colors.at(index);
                }
            }
            update();
            break;
        }
        case Tool::Shape:{
            // draw a preview of how the shape will look
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
            if(currentShape == ShapeType::Ellipse){
                drawEllipse(topLeft, bottomRight, false);
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
        // if youre erasing with right click
        if(x >= 0 && x < document->activeLayer_().width && y >= 0 && y < document->activeLayer_().height){
            if (document->activeLayer_().at(x, y) != Qt::transparent){
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
    // if moving reference picture
    if(movingPicture){
        document->activeLayer_().position = (event->pos() - moveOffset) / pixelSize;
        movingPicture = false;
        update();
        return;
    }
    // to confirm paste
    if(isPasting){
        commitPaste();
        return;
    }
    switch(currentTool){
    case Tool::Select:
    {
        // end of selection. calculate the selections corners and save the colors in the selection
        int eventX = event->position().x()/ pixelSize;
        int eventY = event->position().y()/pixelSize;
        eventX = std::clamp(eventX, 0, document->activeLayer_().width -1);
        eventY = std::clamp(eventY, 0, document->activeLayer_().height -1);
        selection.dragEnd = QPoint(eventX, eventY);
        selection.topLeft = QPoint(std::min(selection.dragStart.x(), selection.dragEnd.x()), std::min(selection.dragStart.y(),selection.dragEnd.y()));
        selection.bottomRight = QPoint(std::max(selection.dragStart.x(), selection.dragEnd.x()), std::max(selection.dragStart.y(),selection.dragEnd.y()));
        selection.width = selection.bottomRight.x() - selection.topLeft.x();
        selection.height = selection.bottomRight.y() - selection.topLeft.y();
        for(int sy = selection.topLeft.y(); sy<=selection.bottomRight.y(); sy++){
            for(int sx = selection.topLeft.x(); sx<=selection.bottomRight.x(); sx++){
                selection.colors.push_back(document->activeLayer_().at(sx,sy));
            }
        }
        update();
        break;
    }
    case Tool::Move:
        selection.dragging = false;
        if(selection.isEmpty(selection) || !selection.canMove) return;
        break;
    case Tool::Shape:
    {
        // remove the temp layer for previews
        removeTempLayer();
        // make sure shape isnt just a point (creates other problems)
        if(shape.end == QPoint(0,0) && shape.start == QPoint(0,0)){
            update();
            break;
        }
        // calculate the shapes corners and draw according to the chosen shape
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
        else if(currentShape == ShapeType::Ellipse){
            drawEllipse(topLeft, bottomRight);
        }
        else if(currentShape == ShapeType::Line){
            drawLine(shape.start, shape.end);
        }
        update();
        break;

    }
    }

    // push what you did to the undo stack
    if(!currentAction.empty()){
        UndoAction action;
        action.type = UndoType::Pixel;
        action.changes = currentAction;


        document->pushUndoAction(action);
    }
}
// key press events
void PixelCanvas::keyPressEvent(QKeyEvent *event){
    // enter confirms paste. escape cancels it
    if(isPasting){
        if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter){
            commitPaste();
            return;
        }
        if(event->key() == Qt::Key_Escape){
            cancelPaste();
            return;
        }
    }
    // enter confirms move. escape cancels it
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

// function to draw rectangle with just boundaries
void PixelCanvas::drawRectangle(QPoint topLeft, QPoint bottomRight, bool recordUndo){
    for(int sx = topLeft.x(); sx <= bottomRight.x(); sx++){
        for(int sy = topLeft.y(); sy <= bottomRight.y(); sy++){
            if(sx < 0 || sx >= document->activeLayer_().width) continue;
            if(sy < 0 || sy >= document->activeLayer_().height) continue;
            if(sx == topLeft.x() || sx == bottomRight.x() || sy == topLeft.y() || sy == bottomRight.y()){
                paintColor(sx,sy, currentColor, recordUndo);
            }
        }
    }
}
// function to draw ellipse
void PixelCanvas::drawEllipse(QPoint topLeft, QPoint bottomRight, bool recordUndo){
    // algorithm stolen from the internet
    int centerX = (topLeft.x() + bottomRight.x())/2;
    int centerY = (topLeft.y() + bottomRight.y())/2;
    int radiusX = (bottomRight.x()-topLeft.x())/2;
    int radiusY = (bottomRight.y()-topLeft.y())/2;
    for(int cx = -radiusX; cx <=radiusX;cx++){
        for(int cy = -radiusY; cy <= radiusY;cy++){
            if((cx*cx*radiusY*radiusY) + (cy*cy*radiusX*radiusX) <= (radiusX*radiusX*radiusY*radiusY)){
                int px = centerX + cx;
                int py = centerY + cy;
                if(px < 0 || px >= document->activeLayer_().width) continue;
                if(py < 0 || py >= document->activeLayer_().height) continue;
                paintColor(px,py,currentColor, recordUndo);

            }
        }
    }
}
// function to draw circle
void PixelCanvas::drawCircle(QPoint topLeft, QPoint bottomRight, bool recordUndo)
{
    // another stolen algorithm. im not good with maffs
    int centerX = (topLeft.x() + bottomRight.x()) / 2;
    int centerY = (topLeft.y() + bottomRight.y()) / 2;
    int radius = std::min(bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()) / 2;
    int x = radius;
    int y = 0;
    int decision = 1 - radius;
    while (x >= y) {
        paintColor(centerX + x, centerY + y, currentColor, recordUndo);
        paintColor(centerX + y, centerY + x, currentColor, recordUndo);
        paintColor(centerX - y, centerY + x, currentColor, recordUndo);
        paintColor(centerX - x, centerY + y, currentColor, recordUndo);
        paintColor(centerX - x, centerY - y, currentColor, recordUndo);
        paintColor(centerX - y, centerY - x, currentColor, recordUndo);
        paintColor(centerX + y, centerY - x, currentColor, recordUndo);
        paintColor(centerX + x, centerY - y, currentColor, recordUndo);
        y++;
        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            x--;
            decision += 2 * (y - x) + 1;
        }
    }
}
// function to draw a line
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
        if(x0 >= 0 && x0 < document->activeLayer_().width && y0 >= 0 && y0 < document->activeLayer_().height){
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
// reset the canvas to its initial state
void PixelCanvas::resetCanvas()
{
    document->resetCanvas();
}
// clear the current layer
void PixelCanvas::clear()
{
    // only clear if its a pixel layer
    document->clear();
}
void PixelCanvas::floodFill(int startX, int startY){
    QColor target = document->activeLayer_().at(startX, startY);
    QColor fill = currentColor;
    // if selected color and where you click are the same color then stop
    if(target == fill) return;
    std::queue<QPoint> q;
    // populate queue with clicked pixel
    q.push(QPoint(startX, startY));
    // looop through until all neighbors are the same color as selected color
    while (!q.empty()){
        // start with clicked pixel
        QPoint p = q.front();
        // remove it from queue
        q.pop();
        int x = p.x();
        int y = p.y();
        //make sure youre within the canvas boundary
        if (x >= 0 && x < document->activeLayer_().width && y >= 0 && y < document->activeLayer_().height){
            // if the current pixel is the same as what we clicked then go on
            if(document->activeLayer_().at(x, y) == target){
                paintColor(x, y, currentColor);
                // add the current layer's neighbors and continue
                q.push(QPoint(x+1, y));
                q.push(QPoint(x-1, y));
                q.push(QPoint(x, y+1));
                q.push(QPoint(x, y-1));
            }
        }


    }
}
void PixelCanvas::commitMove(){
    removeTempLayer();
    // make a rectangle of where you will put the selection
    // this will be checked later to ensure we dont erase what we just put down
    QRect destRect(selection.selectionOffset.x(), selection.selectionOffset.y(),
                   selection.width+1, selection.height+1);
    // draw the pixels in new location
    for (int my = 0; my < selection.height+1; my++){
        for (int mx = 0; mx < selection.width+1; mx++){
            int canvasX = selection.selectionOffset.x() + mx;
            int canvasY = selection.selectionOffset.y() + my;
            int index = (selection.width+1) * my + mx;
            if (index >= selection.colors.size()) continue;
            if (canvasX < 0 || canvasX >= document->activeLayer_().width) continue;
            if (canvasY < 0 || canvasY >= document->activeLayer_().height) continue;
            if(selection.colors.at(index) == Qt::transparent) continue;
            paintColor(canvasX, canvasY,selection.colors.at(index));
            // change where the selection highlight square is
            selection.previewEnd = QPoint(canvasX, canvasY);
        }
    }
    // erase the pixels from old location
    for (int my = 0; my < selection.height+1; my++){
        for (int mx = 0; mx < selection.width+1; mx++){
            int oldX = selection.topLeft.x() + mx;
            int oldY = selection.topLeft.y() + my;
            int index = (selection.width+1) * my + mx;
            if (index >= selection.colors.size()) continue;
            if (oldX < 0 || oldX >= document->activeLayer_().width) continue;
            if (oldY < 0 || oldY >= document->activeLayer_().height) continue;
            if(selection.colors.at(index) == Qt::transparent) continue;
            // make sure we dont overwrite what we just put down
            if(destRect.contains(oldX, oldY)) continue;
            paintColor(oldX,oldY,Qt::transparent);
        }
    }
    selection.dragging = false;
    selection.moveFloating = false;
    selection.setValues(selection);
    // add what you did to undo stack
    if(!currentAction.empty()){
        UndoAction action;
        action.type = UndoType::Pixel;
        action.changes = currentAction;
        document->pushUndoAction(action);
    }

    update();
}
void PixelCanvas::cancelMove(){
    if (!selection.moveFloating) return;
    removeTempLayer();
    selection.moveFloating = false;
    selection.dragging = false;
    update();
}
void PixelCanvas::copyPixels(){
    // lowkey no reason for this to exist lol
    // just thought users are used to pressing Ctrl+C to copy
    qDebug() << "copied!";
}
void PixelCanvas::pastePixels(){
    isPasting = true;
    makeTempLayer();
}
// confirm paste
void PixelCanvas::commitPaste(){
    // almost the exact same process as confirm move. except we dont remove the pixels from old location
    removeTempLayer();
    currentAction.clear();
    for (int py = 0; py < selection.height+1; py++){
        for (int px = 0; px < selection.width+1; px++){
            int canvasX = selection.movePosition.x() - (selection.width/2) + px;
            int canvasY = selection.movePosition.y() - (selection.height/2) + py;
            int index = (selection.width+1) * py + px;
            if (index >= selection.colors.size()) continue;
            if (canvasX < 0 || canvasX >= document->activeLayer_().width) continue;
            if (canvasY < 0 || canvasY >= document->activeLayer_().height) continue;
            if(selection.colors.at(index) == Qt::transparent) continue;
            paintColor(canvasX, canvasY,selection.colors.at(index));

        }
    }
    if(!currentAction.empty()){
        UndoAction action;
        action.type = UndoType::Pixel;
        action.changes = currentAction;
        document->pushUndoAction(action);
    }
    isPasting = false;
    update();
}
void PixelCanvas::cancelPaste(){
    if(!isPasting) return;
    removeTempLayer();
    isPasting = false;
    update();
}
// layer methods
void PixelCanvas::makeTempLayer(){
    // this is the same as a normal layer just lower opacity, doesnt show in layer panel and is deleted immediately after use
    document->makeTempLayer();
}
void PixelCanvas::removeTempLayer(){
    document->removeTempLayer();
}
// ########## add comments for the rest
void PixelCanvas::flipHorizontal()
{
    UndoAction action;
    action.type = UndoType::Snapshot;
    action.before = document->currentFrame_().layers;
    for(auto &layer : document->currentFrame_().layers){
    Layer tempLayer = layer;

    int width = layer.width;
    int height = layer.height;
    
        for(int x = 0; x < width; x++)
        {
            for(int y = 0; y < height; y++)
            {
                layer.at(x, y) = tempLayer.at(width - 1 - x, y);
            }
        }
    }
    action.after = document->currentFrame_().layers;
    document->pushUndoAction(action);
    update();
}

void PixelCanvas::flipVertical()
{
    UndoAction action;
    action.type = UndoType::Snapshot;
    action.before = document->currentFrame_().layers;
    for(auto &layer : document->currentFrame_().layers){
    Layer tempLayer = layer;
    int width = layer.width;
    int height = layer.height;
        for(int x = 0; x < width; x++)
        {
            for(int y = 0; y < height; y++)
            {
                
                layer.at(x, y) = tempLayer.at(x, height - 1 - y);
            }
        }
    }
    action.after = document->currentFrame_().layers;
    document->pushUndoAction(action);
    update();
}

void PixelCanvas::addLayer(){
    document->addLayer();
}
void PixelCanvas::removeLayer(int index){
    document->removeLayer(index);
}
void PixelCanvas::renameLayer(int index, const QString &name){
    document->renameLayer(index, name);
}
void PixelCanvas::moveLayerUp(int index)
{
    document->moveLayerUp(index);
}
void PixelCanvas::moveLayerDown(int index)
{
    document->moveLayerDown(index);
}
int PixelCanvas::getActiveLayer() const{
    return document->getActiveLayer();
}
// File manipulation
void PixelCanvas::saveImage()
{
    QImage image(document->currentFrame_().layers[0].width * pixelSize, document->currentFrame_().layers[0].height * pixelSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    for(const auto &layer : document->currentFrame_().layers)
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
void PixelCanvas::saveProject(const QString &path)
{
    QJsonObject root;
    root["Width"] = document->currentFrame_().layers[0].width;
    root["Height"] = document->currentFrame_().layers[0].height;
    QJsonArray frameArray;
    for(const auto &frame:document->allFrames()){
        QJsonObject frameObject;
        frameObject["duration"] = frame.duration;
        QJsonArray layerArray;

        for(const auto &layer : frame.layers){
            QJsonObject layerObject;
            layerObject["name"] = layer.name;
            layerObject["visible"] = layer.visible;
            layerObject["opacity"] = layer.opacity;
            QByteArray pixelData;
            pixelData.resize(layer.width * layer.height * 4);
            int index = 0;
            for(int y = 0; y < layer.height; y++){
                for(int x = 0; x < layer.width; x++){
                    QColor color = layer.at(x,y);
                    pixelData[index++] = static_cast<char>(color.red());
                    pixelData[index++] = static_cast<char>(color.green());
                    pixelData[index++] = static_cast<char>(color.blue());
                    pixelData[index++] = static_cast<char>(color.alpha());
                }
            }
            QByteArray compressed = qCompress(pixelData, 9);
            layerObject["pixels"] = QString::fromLatin1(compressed.toBase64());
            layerArray.append(layerObject);
        }
        frameObject["layers"] = layerArray;
        frameArray.append(frameObject);
    }
    root["frames"] = frameArray;
    QJsonDocument doc(root);
    QFile file(path);
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
    layer.width = document->currentFrame_().layers[0].width;
    layer.height = document->currentFrame_().layers[0].height;
    document->currentFrame_().layers.push_back(layer);
    document->setActiveLayer(document->currentFrame_().layers.size()-1);
    update();
}
void PixelCanvas::loadFromJson(QJsonObject root)
{

    int width = root["Width"].toInt();
    int height = root["Height"].toInt();

    QJsonArray frameArray = root["frames"].toArray();
    if (frameArray.isEmpty()) return;
    QList<Frame> loadedFrames;
    for(const auto &frameValue : frameArray){
        QJsonObject frameObject = frameValue.toObject();
        Frame frame;
        frame.duration = frameObject["duration"].toInt(100);
        QJsonArray layerArray = frameObject["layers"].toArray();
        for(const auto &layerValue : layerArray){
            QJsonObject layerObject = layerValue.toObject();
            Layer layer;
            layer.name =  layerObject["name"].toString();
            layer.visible = layerObject["visible"].toBool();
            layer.opacity = layerObject["opacity"].toDouble();
            layer.width = width;
            layer.height = height;
            layer.pixels.resize(width*height);
            QString encoded = layerObject["pixels"].toString();
            QByteArray compressed = QByteArray::fromBase64(encoded.toLatin1());
            QByteArray pixelData = qUncompress(compressed);
            int index = 0;
            for(int y = 0; y < height; y++){
                for(int x = 0; x < width; x++){
                    if(index+4 <= pixelData.size()){
                        int r = static_cast<unsigned char>(pixelData[index++]);
                        int g = static_cast<unsigned char>(pixelData[index++]);
                        int b = static_cast<unsigned char>(pixelData[index++]);
                        int a = static_cast<unsigned char>(pixelData[index++]);
                        layer.at(x, y) = QColor(r, g, b, a);
                    }
                }
            }
            frame.layers.push_back(layer);
        }
        loadedFrames.push_back(frame);
    }
    document->loadFrames(loadedFrames);
    updateCanvasSize();
    resizeCanvas(width, height);
    document->buildPalette();
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
    resizeCanvas(document->getCanvasWidth(), document->getCanvasHeight());
    updateCanvasSize();
    auto palette = medianCut.medianCut(image, paletteSize);
    layer.pixels.resize(document->getCanvasWidth() * document->getCanvasHeight());
    for (int y = 0; y < document->getCanvasHeight(); y++) {
        for (int x = 0; x < document->getCanvasWidth(); x++) {
            QColor mapped = medianCut.nearestColor(image.pixelColor(x, y), palette);
            document->activeLayer_().at(x, y) = mapped;
        }
    }
    update();
}
void PixelCanvas::saveSpriteSheet(const QString &path, int cols, int scale){
    int rows = (document->getFrameSize() + cols - 1) / cols;
    int frameWidth = document->getCanvasWidth() * scale;
    int frameHeight = document->getCanvasHeight()*scale;
    QImage spriteSheet(cols * frameWidth, rows *frameHeight, QImage::Format_ARGB32);
    spriteSheet.fill(Qt::transparent);
    QPainter painter(&spriteSheet);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for(int i = 0; i<document->getFrameSize(); i++){
        QImage frame = renderFrame(i);
        if(scale > 1){
            frame = frame.scaled(frameWidth, frameHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        }
        int x = (i % cols) * frameWidth;
        int y = (i / cols) * frameHeight;
        painter.drawImage(x, y, frame);
    }
    painter.end();
    spriteSheet.save(path);
}
void PixelCanvas::saveGIF(const QString &path, int scale){
    QByteArray filePath = path.toUtf8();
    GifWriter writer = {};
    int imageWidth = document->getCanvasWidth() *scale;
    int imageHeight = document->getCanvasHeight() *scale;
    int maxScale = qMin(64, 8192/qMax(document->getCanvasWidth(), document->getCanvasHeight()));
    scale = qBound(1,scale,maxScale);
    QImage firstImage = renderFrame(0).convertToFormat(QImage::Format_RGBA8888);
    if(scale >1) firstImage = firstImage.scaled(imageWidth, imageHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    if(!GifBegin(&writer, filePath.constData(), firstImage.width(), firstImage.height(), document->getThisFrameDuration(0)/10)){
        qDebug()<< "Gif Begin failed";
        return;
    }
    for(int i = 0; i < document->getFrameSize(); i++){
    QImage image = renderFrame(i).convertToFormat(QImage::Format_RGBA8888);
    if(scale >1) image = image.scaled(imageWidth, imageHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        if(!GifWriteFrame(&writer, image.constBits(), image.width(), image.height(), document->getThisFrameDuration(i)/10)){
            qDebug() << "Gif write faile on frame" << i;
            GifEnd(&writer);
            return;
        }
    qDebug()<<"Gif write successful";
    }
    GifEnd(&writer);
}
// undo and redo
void PixelCanvas::undo()
{
    document->undo();
}
void PixelCanvas::redo()
{
    document->redo();
}
// getters
QColor PixelCanvas::getColor(){
    return currentColor;
}
int PixelCanvas::getZoom(){
    return pixelSize;
}
QStringList PixelCanvas::getLayerNames(){
    return document->getLayerNames();
}
float PixelCanvas::getLayerOpacity(int index) const{
    return document->getLayerOpacity(index);
}
// setters

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
    document->setActiveLayer(index);
}
void PixelCanvas::setLayerOpacity(int index, float opacity){
    document->setLayerOpacity(index, opacity);
}
// frame methods
void PixelCanvas::duplicateFrame(){
    document->duplicateFrame();
}
void PixelCanvas::copyFrame(){
    document->copyFrame();
}
void PixelCanvas::pasteFrame(){
    document->pasteFrame();
}
void PixelCanvas::addFrame(){
    document->addFrame();
}
void PixelCanvas::deleteFrame(int index){
    document->deleteFrame(index);
}
void PixelCanvas::switchFrame(int index){
    if (index < 0 || index >= document->getFrameSize()) return;
    cancelPaste();
    cancelMove();
    selection.canMove = false;
    if(!currentAction.empty()){
        UndoAction action;
        action.type = UndoType::Pixel;
        action.changes = currentAction;
        document->pushUndoAction(action);
    }
    document->switchFrame(index);
}
int PixelCanvas::getCurrentFrame(){
    return document->getCurrentFrame();
}
int PixelCanvas::getFrameSize(){
    return document->getFrameSize();
}
int PixelCanvas::getFrameDuration(){
    return document->getFrameDuration();
}
void PixelCanvas::setFrameDuration(int value){
    document->setFrameDuration(value);
}
QImage PixelCanvas::renderFrame(int frameIndex){
    return document->renderFrame(frameIndex);
}
void PixelCanvas::setTool(Tool tool){
    if(selection.moveFloating){
    cancelMove();
    }
    currentTool = tool;
}
void PixelCanvas::setShape(ShapeType shape){
    currentShape = shape;
}
void PixelCanvas::setBrushSize(int newSize){
    brushSize = newSize;
}