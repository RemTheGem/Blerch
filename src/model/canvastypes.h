#ifndef CANVASTYPES_H
#define CANVASTYPES_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QString>
#include <QVector>
#include <QList>
#include <QHash>

enum class LayerType{
    Pixel,
    Reference
};
struct Layer {
    int id = -1;
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
    int layerId;
    qint64 seq = 0;
    UndoType type;
    QVector<PixelChange> changes;
    QVector<Layer> before;
    QVector<Layer> after;
};
struct Frame{
    QList<Layer> layers;
    int duration = 100;
    QHash<int, QVector<UndoAction>> undoStack;
    QHash<int, QVector<UndoAction>> redoStack;
    QVector<UndoAction> frameUndoStack;
    QVector<UndoAction> frameRedoStack;
    bool isEmpty() const { return layers.isEmpty();}
};



#endif // CANVASTYPES_H
