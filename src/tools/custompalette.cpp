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

int CustomPalette::colorDistance(const QColor &a, const QColor &b){
    int distanceH = std::abs(a.hsvHue() - b.hsvHue());
    if(distanceH  > 180) distanceH = 360 - distanceH;
    int distanceS = std::abs(a.hsvSaturation() - b.hsvSaturation());
    int distanceV = std::abs(a.value() - b.value());
    return distanceH +  distanceS + distanceV;
}

bool CustomPalette::tooSimilar(const QColor &candidate, const QList<QColor> &existing, int minDistance){
    for(const QColor &c : existing){
        if(colorDistance(candidate, c) < minDistance) return true;
    }
    return false;
}

void CustomPalette::randomizePalette(){
    constexpr int hJitter = 18;
    constexpr int sMin = 180;
    constexpr int sMax = 255;
    constexpr int sJitter = 25;
    constexpr int vFloor = 55;
    constexpr int vJitter = 12;
    constexpr int minColorDistance = 45;
    constexpr int maxAttempts = 20;

    QList<QColor> randomColors;
    int sizes[] = {8, 16, 32};
    int paletteSize = sizes[QRandomGenerator::global()->bounded(3)];
    int hueCount = QRandomGenerator::global()->bounded(1, 4);
    QList<int> hues;
    for(int i = 0; i< hueCount; i++){
        hues.append(QRandomGenerator::global()->bounded(360));
    }
    int baseSaturation = QRandomGenerator::global()->bounded(sMin, sMax + 1);
    for(int i  = 0; i < paletteSize; i++){
        QColor candidate;
        int attempts = 0;
        do {
            int baseHue = hues[QRandomGenerator::global()->bounded(hues.size())];
            int h = (baseHue + QRandomGenerator::global()->bounded(-hJitter, hJitter + 1) + 360) % 360;
            int s = qBound(0, baseSaturation + QRandomGenerator::global()->bounded(-sJitter, sJitter +1), 255);
            int v = vFloor + (i * (255 - vFloor) / (paletteSize - 1));
            v = qBound(0, v + QRandomGenerator::global()->bounded(-vJitter, vJitter+1), 255);
            candidate = QColor::fromHsv(h, s, v);
            attempts++;
        } while(tooSimilar(candidate, randomColors, minColorDistance) && attempts < maxAttempts);
        randomColors.append(candidate);
    }
    sortColors(randomColors);
    setColors(randomColors);
}

void CustomPalette::sortColors(QList<QColor> &colors){
    std::sort(colors.begin(), colors.end(), [](const QColor &a, const QColor &b){
        if(a.value() != b.value()) return a.hsvHue() < b.hsvHue();
        return  a.value() < b.value();
    });
}