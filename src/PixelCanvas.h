#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H
#include "tools/mediancut.h"
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
    struct Selection{
        int width;
        int height;
        bool dragging;
        QPoint dragStart;
        QPoint dragEnd;
        QPoint dragOffset;
        QPoint selectionOffset;
        QPoint position;
        std::vector<QColor> colors;
        bool isEmpty(const Selection& s){
            return s.colors.empty();
        }
        void setValues(Selection s){
            dragStart = s.selectionOffset;
        }

    };

    // main functions
    void setColor(const QColor &c) {currentColor = c; emit colorChanged(c);} // set the current color
    void clear(); // clear the canvas on the current layer
    void saveImage(); // save image as png
    void saveProject(); // save the project as a json file
    void loadProject(); // load project from a json file
    void loadPicture(); // load a picture on a separate layer for reference
    void updateCanvasSize(); // update the current canvas size
    void buildPalette(); // get colors and their corresponding frequencies from the canvas and add them to a hash table
    void resizeCanvas(int width, int height); // set the canvas size
    void floodFill(int x, int y); // method for flood fill tool
    void undo(); // undo method
    void redo(); // redo method
    void pictureToPixel(); // turn imported picture into pixel art
    // enum class for available tools
    enum class Tool {
        Brush,
        Eraser,
        EyeDropper,
        Fill,
        Select,
        Move,
        Shape
    };
    enum class ShapeType{
        Rectangle,
        Circle,
        Line
    };
    struct Shape{
        QPoint start;
        QPoint end;
        int width;
        int height;
    };

    // layer methods
    void addLayer(); // add a new layer
    void removeLayer(int index); // remove selected layer
    void setActiveLayer(int index); // change selected layer
    void moveLayerUp(int index); // move selected layer up
    void moveLayerDown(int index); // move selected layer down
    void renameLayer(int index, const QString &name); // rename selected layer
    void setLayerOpacity(int index, float opacity); // change selected layer's opacity
    float getLayerOpacity(int index)const; // get selected layer's opacity
    QStringList getLayerNames(); // return the list of layer names

    // helper methods
    void paintColor(int x, int y, const QColor &color); // paint color into the corresponding square
    QList<QColor> sortColors(QHash<QRgb, int> colorFrequency); // sort a list of colors based on frequency
    void undoActions(); // helper method for undo
    QColor getColor(); // return the current color
    int getZoom(); // get the current zoom
    void setZoom(int zoom); // set the zoom amount
    void setHorizontalSymmetry(bool enabled); // set horizontal symmetry drawing
    void setVerticalSymmetry(bool enabled); // set vertical symmetry drawing
    void setTool(Tool tool); // set the current tool
    void setShape(ShapeType shape); // set shape
    void drawRectangle(QPoint topLeft, QPoint bottomRight);
    void drawCircle(QPoint topLeft, QPoint bottomRight);
    void makeTempLayer();
    void removeTempLayer();
    void setBrushSize(int newSize);
    // Events
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    int pixelSize = 20; // pixel/zoom size
    int brushSize = 1;
    int canvasWidth = 32;
    int canvasHeight = 32;
    bool movingPicture; // boolean to check if an imported picture is being moved
    bool horizontalSymmetry = false; // bool for horizontal symmetry
    bool verticalSymmetry = false; // bool for vertical symmetry
    QPoint moveOffset; // point on the canvas for moving picture
    QColor currentColor = Qt::black; // selected color
    QColor moveColor;
    bool isDrawing = false; // bool to check if user is drawing
    bool isUndoing = false; // bool to check if user is undoing
    bool isErasing = false; // bool to check if user is erasing
    // enum class for different layer types
    // Pixel is for drawing, Reference is for pictures that have been imported
    enum class LayerType{
        Pixel, Reference
    };
    // struct for layers and their attributes
    struct Layer{
        int width; // layer width
        int height; // layer height
        float opacity = 1.0f; // layer opacity
        float scale = 1.0f; // layer scale (for zoom)
        bool visible = true; // layer visibility (not really used)
        bool locked = false; // layer lock (not used)
        LayerType type = LayerType::Pixel; // default layer type is pixel (for drawing)
        QString name; // layer name
        QVector<QColor> pixels; // colors in current layer
        QImage image; // image imported for current layer
        QPoint position = {0,0}; // position of the current layer
        // methods to get color at a specific pixel
        QColor& at(int x, int y){
            return pixels[y * width + x];
        }
        const QColor& at(int x, int y) const{
            return pixels[y * width +x];
        }
    };
    // struct for storing any changes in pixel color
    struct PixelChange{
        int layer;
        int x;
        int y;
        QColor oldColor;
        QColor newColor;
    };
    // others
    std::vector<Layer> layers; // vector storing layers
    int activeLayer = 0; // selected layer
    QHash<QRgb, int> colorFrequency; // number of times color has appeared on current canvas
    Tool currentTool = Tool::Brush; // default tool
    ShapeType currentShape = ShapeType::Rectangle;
    Selection selection;
    Shape shape;
    Layer currentState; // store current state for undo and redo
    Layer undoState; // last state for undo and redo
    std::deque<std::vector<PixelChange>> undoStack; // stack to store undos
    std::deque<std::vector<PixelChange>> redoStack; // stack to store redos
    std::vector<PixelChange> currentAction; // vector to store current action
    int maxUndo = 5; // max number of undos (probably redundant atp. cbf to check)
signals:
    void colorChanged(QColor color); // signal to change the selected color
    void paletteUpdated(QList<QColor> colors); // signal to change the palette
};


// class for previewing selected color
class ColorPreviewWidget : public QWidget{
  Q_OBJECT
    int previewSize = 20; // size of the preview pixel
protected:
    void paintEvent(QPaintEvent *event) override;
public:
    explicit ColorPreviewWidget(QWidget *parent = nullptr);
    QColor selectedColor = Qt::black; // default selected color
    void setPreviewColor(const QColor &color); // method for changing the selected color
};

#endif // PIXELCANVAS_H