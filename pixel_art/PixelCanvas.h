#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H

#include <QWidget>
#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <deque>

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

    // helper methods
    void undoActions();
    QColor getColor();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    static const int gridSize = 40;
    int pixelSize = 10;
    QColor currentColor = Qt::black;
    bool isDrawing = false;
    bool isUndoing = false;
    bool isErasing = false;
    struct CanvasState{
        QColor pixels[gridSize][gridSize];
    };
    Tool currentTool = Tool::Brush;
    CanvasState currentState;
    CanvasState undoState;
    std::deque<CanvasState> undoStack;
    std::deque<CanvasState> redoStack;
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