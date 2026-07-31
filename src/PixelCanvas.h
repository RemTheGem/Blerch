#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H

#include <QWidget>
#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <deque>
#include <vector>
#include <QWheelEvent>

class PixelCanvas : public QWidget
{
    Q_OBJECT


public:
    explicit PixelCanvas(QWidget *parent = nullptr);
    void setColor(const QColor &c) {currentColor = c; emit colorChanged(c);}
    void clear();
    void saveImage();
    void saveProject();
    void loadProject();
    void loadPicture();
    void updateCanvasSize();
    void buildPalette();

    QList<QColor> sortColors(QHash<QRgb, int> colorFrequency);
    void setZoom(int zoom);
    void resizeCanvas(int width, int height);
    enum class Tool {
        Brush,
        Eraser,
        EyeDropper,
        Fill
    };
    void setTool(Tool tool);
    void floodFill(int x, int y);
    void undo();
    void redo();
    // layer methods
    void addLayer();
    void removeLayer(int index);
    QStringList getLayerNames();
    void setActiveLayer(int index);
    void moveLayerUp(int index);
    void moveLayerDown(int index);
    void renameLayer(int index, const QString &name);
    void setLayerOpacity(int index, float opacity);
    void setHorizontalSymmetry(bool enabled);
    void setVerticalSymmetry(bool enabled);
    float getLayerOpacity(int index)const;

    // helper methods
    void paintColor(int x, int y, const QColor &color);
    void undoActions();
    QColor getColor();
    int getZoom();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    int pixelSize = 20;
    int canvasWidth = 32;
    int canvasHeight = 32;
    bool movingPicture;
    bool horizontalSymmetry = true;
    bool verticalSymmetry = false;
    QPoint moveOffset;
    QColor currentColor = Qt::black;
    bool isDrawing = false;
    bool isUndoing = false;
    bool isErasing = false;
    enum class LayerType{
        Pixel, Reference
    };
    struct Layer{
        LayerType type = LayerType::Pixel;
        QString name;
        int width;
        int height;
        QVector<QColor> pixels;
        bool visible = true;
        float opacity = 1.0f;
        QImage image;
        QPoint position = {0,0};
        float scale = 1.0f;
        // implement lock!!!
        bool locked = false;
        QColor& at(int x, int y){
            return pixels[y * width + x];
        }
        const QColor& at(int x, int y) const{
            return pixels[y * width +x];
        }
    };
    struct PixelChange{
        int layer;
        int x;
        int y;
        QColor oldColor;
        QColor newColor;
    };

    std::vector<Layer> layers;
    int activeLayer = 0;
    QHash<QRgb, int> colorFrequency;
    Tool currentTool = Tool::Brush;
    Layer currentState;
    Layer undoState;
    std::deque<std::vector<PixelChange>> undoStack;
    std::deque<std::vector<PixelChange>> redoStack;
    std::vector<PixelChange> currentAction;
    int maxUndo = 5;
signals:
    void colorChanged(QColor color);
    void paletteUpdated(QList<QColor> colors);
};



class ColorPreviewWidget : public QWidget{
  Q_OBJECT


  int previewSize = 20;
  protected:

  void paintEvent(QPaintEvent *event) override;
  public:
    explicit ColorPreviewWidget(QWidget *parent = nullptr);
    QColor selectedColor = Qt::black;
    void setPreviewColor(const QColor &color);
};

#endif // PIXELCANVAS_H