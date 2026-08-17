#ifndef CANVASDOCUMENT_H
#define CANVASDOCUMENT_H

#include "CanvasTypes.h"
#include <QObject>
#include <QHash>
#include <QRgb>
#include <QStringList>



class CanvasDocument : public QObject
{
    Q_OBJECT
public:
    explicit CanvasDocument(QObject *parent = nullptr);

    void resizeCanvas(int width, int height);
    void resetCanvas();
    void clear();
    void buildPalette();
    QList<QColor> sortColors(const QHash<QRgb, int> &colorFrequency) const;
    int getCanvasWidth() const {return canvasWidth;}
    int getCanvasHeight() const {return canvasHeight;}

    void addLayer();
    void removeLayer(int index);
    void setActiveLayer(int index);
    void moveLayerUp(int index);
    void moveLayerDown(int index);
    void renameLayer(int index, const QString &name);
    void setLayerOpacity(int index, float opacity);
    float getLayerOpacity(int index) const;
    QStringList getLayerNames() const;
    int getActiveLayer() const;
    void makeTempLayer();
    void removeTempLayer();

    Layer &activeLayer_();
    const Layer &activeLayer_() const;
    Frame &currentFrame_();
    const Frame &currentFrame_() const;

    void duplicateFrame();
    void copyFrame();
    void pasteFrame();
    void addFrame();
    void deleteFrame(int index);
    void switchFrame(int index);
    int getCurrentFrame() const;
    int getFrameSize() const;
    int getFrameDuration() const;
    int getThisFrameDuration(int index) const;
    void setFrameDuration(int value);
    void setAllFrameDurations(int value);
    QImage renderFrame(int frameIndex) const;
    const QList<Frame> &allFrames() const {return frames;}
    void loadFrames(const QList<Frame> &newFrames);

    int getOnionPreviousFrames() const;
    int getOnionNextFrames() const;

    void undo();
    void redo();
    void pushUndoAction(const UndoAction& action);
signals:
    void layerChanged();
    void frameChanged();
    void paletteUpdated(QList<QColor> colors);
    void clearLayerList();
    void reInitLayers();
    void canvasSizeChanged(int width, int height);
    void documentMutated();

private:
    int canvasWidth = 32;
    int canvasHeight = 32;
    int currentFrameIndex = 0;
    int activeLayerIndex = 0;
    QList<Frame> frames;
    Frame copiedFrame;
    QHash<QRgb, int> colorFrequency;

};

#endif // CANVASDOCUMENT_H
