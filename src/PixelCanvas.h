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
    void setColor(const QColor &c) {currentColor = c;}
    void clear();
    void saveImage();
    void saveProject();
    void loadProject();
    void updateCanvasSize();
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
    int getLayerCount();
    void setActiveLayer(int index);

    // helper methods
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
    QColor currentColor = Qt::black;
    bool isDrawing = false;
    bool isUndoing = false;
    bool isErasing = false;
    struct Layer{
        QString name;
        int width;
        int height;
        QVector<QColor> pixels;
        QColor& at(int x, int y){
            return pixels[y * width + x];
        }
        const QColor& at(int x, int y) const{
            return pixels[y * width +x];
        }
        bool visible = true;
    };
    struct PixelChange{
        int x;
        int y;
        QColor oldColor;
        QColor newColor;
    };

    std::vector<Layer> layers;
    int activeLayer = 0;
    int layerCount = 1;
    Tool currentTool = Tool::Brush;
    Layer currentState;
    Layer undoState;
    std::deque<std::vector<PixelChange>> undoStack;
    std::deque<std::vector<PixelChange>> redoStack;
    std::vector<PixelChange> currentAction;
    int maxUndo = 5;
signals:
    void colorChanged(QColor color);
};



class ColorPreviewWidget : public QWidget{
  Q_OBJECT


  int previewSize = 20;
  protected:

  void paintEvent(QPaintEvent *event) override;
  public:
    explicit ColorPreviewWidget(QWidget *parent = nullptr);
    QColor selectedColor = Qt::black;
    void setColor(const QColor &color);


};

#endif // PIXELCANVAS_H