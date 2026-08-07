#include "custompalette.h"
#include <QRegularExpression>
#include <QFile>
CustomPalette::CustomPalette(QWidget *parent)
    : QWidget{parent}
{

}
void CustomPalette::paintEvent(QPaintEvent *){
    QPainter painter(this);
    for (int i = 0; i<colors.size(); i++){
        int row = i / columns;
        int col = i % columns;

        QRect rect(col * (cellSize *spacing), row * (cellSize *spacing), cellSize, cellSize);
        painter.fillRect(rect, colors[i]);
        painter.drawRect(rect);
    }
}
void CustomPalette::setColors(const QList<QColor> &newColors){
    colors = newColors;
    int rows = (colors.size() + columns -1)/ columns;
    // may cause problems. keep an eye
    setFixedSize(columns *(cellSize + spacing), rows * (cellSize +spacing));
    update();
}
void CustomPalette::mousePressEvent(QMouseEvent *event){
    int col = event->position().x() / (cellSize + spacing);
    int row = event->position().y() / (cellSize + spacing);
    int index = row * columns + col;
    if(index >= 0 && index < colors.size()){
        emit colorSelected(colors[index]);
    }
}
void CustomPalette::loadGPL(const QString &fileName){
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QVector<QColor> colors;
    while(!in.atEnd()){
        QString line = in.readLine().trimmed();
        if(line.isEmpty()) continue;
        if(line.startsWith("#")) continue;
        if(line.startsWith("GIMP Palette") || line.startsWith("Name:") || line.startsWith("Columns:")) continue;

        QStringList parts = line.split(QRegularExpression("\\s"));
        if(parts.size() >= 3){
            int r = parts[0].toInt();
            int g = parts[1].toInt();
            int b = parts[2].toInt();
            colors.append(QColor(r,g,b));
        }
    }
    emit paletteUpdatedCustom(colors);
}
void CustomPalette::clearPalette(){
    colors.clear();
    emit paletteUpdatedCustom(colors);
}