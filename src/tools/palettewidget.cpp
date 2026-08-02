#include "palettewidget.h"
#include "../mainWindow.h"


paletteWidget::paletteWidget(QWidget *parent)
    : QWidget{parent}
{


}
void paletteWidget::paintEvent(QPaintEvent *){
    QPainter painter(this);
    for (int i = 0; i<colors.size(); i++){
        int row = i / columns;
        int col = i % columns;

        QRect rect(col * (cellSize *spacing), row * (cellSize *spacing), cellSize, cellSize);
        painter.fillRect(rect, colors[i]);
        painter.drawRect(rect);
    }
}
void paletteWidget::setColors(const QList<QColor> &newColors){
    colors = newColors;
    int rows = (colors.size() + columns -1)/ columns;
    // may cause problems. keep an eye
    setFixedSize(columns *(cellSize + spacing), rows * (cellSize +spacing));
    update();
}
void paletteWidget::mousePressEvent(QMouseEvent *event){
    int col = event->position().x() / (cellSize + spacing);
    int row = event->position().y() / (cellSize + spacing);
    int index = row * columns + col;
    if(index >= 0 && index < colors.size()){
        emit colorSelected(colors[index]);
    }

    qDebug() << "x:" << event->position().x()
             << "y:" << event->position().y()
             << "row:" << row
             << "col:" << col
             << "index:" << index
             << "size:" << colors.size();
}