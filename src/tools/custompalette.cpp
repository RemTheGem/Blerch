#include "custompalette.h"
#include <QRegularExpression>
#include <QFile>
#include <QRandomGenerator>

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

void CustomPalette::randomizePalette(){
    QList<QColor> randomColors;
    int hueCount = QRandomGenerator::global()->bounded(1, 5);
    QList<int> hues;
    int size = 16;
    for(int i = 0; i< hueCount; i++){
        hues.append(QRandomGenerator::global()->bounded(360));
    }
    int baseSaturation = QRandomGenerator::global()->bounded(150, 256);
    for(int i  = 0; i < size; i++){
        int baseHue = hues[QRandomGenerator::global()->bounded(hues.size())];
        int h = baseHue + QRandomGenerator::global()->bounded(-30, 31);
        h = (h + 360) % 360;
        int s = baseSaturation + QRandomGenerator::global()->bounded(-40, 41);
        s = qBound(0, s, 255);
        int v = 40 + (i * 215 / (size -1));
        v += QRandomGenerator::global()->bounded(-15, 16);
        v = qBound ( 0, v, 255);
        QColor color = QColor::fromHsv(h, s, v);
        randomColors.append(color);
    }
    sortColors(randomColors);
    setColors(randomColors);
}

void CustomPalette::sortColors(QList<QColor> &colors){
    std::sort(colors.begin(), colors.end(), [](const QColor &a, const QColor &b){
        if(a.value() != b.value()) return a.value() < b.value();
        return a.hsvHue() < b.hsvHue();
    });
}