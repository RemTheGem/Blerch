#ifndef CANVASTYPES_H
#define CANVASTYPES_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QString>
#include <QVector>
#include <QList>

enum class LayerType{
    Pixel,
    Reference
};
struct Layer {
    int width;
    int height;
    float opacity = 1.0f;
    float scale = 1.0f;
    bool visible = true;
    bool locked = false;
    bool isTempLayer = false;
    LayerType type = LayerType::Pixel;
    QString name;
    QVector<QColor> pixels;
    QImage image;
    QPoint position = {0,0};
    QColor &at(int x, int y){
        return pixels[y*width +x];
    }
    const QColor &at(int x, int y) const {
        return pixels[y*width +x];
    }
};
struct PixelChange {
    int layer;
    int x;
    int y;
    QColor oldColor;
    QColor newColor;
};
enum class UndoType{
    Pixel,
    Snapshot
};
struct UndoAction{
    UndoType type;
    QVector<PixelChange> changes;
    QVector<Layer> before;
    QVector<Layer> after;
};
struct Frame{
    QList<Layer> layers;
    int duration = 100;
    QVector<UndoAction> undoStack;
    QVector<UndoAction> redoStack;
    bool isEmpty() const { return layers.isEmpty();}
};



#endif // CANVASTYPES_H
