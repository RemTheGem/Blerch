#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H
#include "tools/mediancut.h"
#include "model/CanvasTypes.h"
#include "model/canvasdocument.h"
#include <QWidget>
#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <deque>
#include <vector>
#include <QWheelEvent>
#include <algorithm>

class PixelCanvas : public QWidget
{
    Q_OBJECT


public:
    explicit PixelCanvas(QWidget *parent = nullptr);
    struct Selection{
        int width;
        int height;
        bool dragging;
        bool moveFloating = false;
        bool canMove = false;
        QPoint dragStart;
        QPoint dragEnd;
        QPoint previewEnd;
        QPoint dragOffset;
        QPoint selectionOffset;
        QPoint movePosition;
        QPoint topLeft; // used for move only. preview has its own
        QPoint bottomRight; // used for move only. preview has its own
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
    void resetCanvas(); // reset the whole canvas
    void saveImage(); // save image as png
    void saveProject(const QString &path); // save the project as a json file
    void loadFromJson(QJsonObject obj); // load project from a json file
    void loadPicture(); // load a picture on a separate layer for reference
    void saveSpriteSheet(const QString &path, int columns, int scale);
    void saveGIF(const QString &path, int scale);
    void updateCanvasSize(); // update the current canvas size
    void buildPalette(); // get colors and their corresponding frequencies from the canvas and add them to a hash table
    void floodFill(int x, int y); // method for flood fill tool
    void undo(); // undo method
    void redo(); // redo method
    void pictureToPixel(); // turn imported picture into pixel art
    void GIFToPixel();
    void copyPixels(); // copy pixels
    void pastePixels(); // paste copied pixels
    void flipHorizontal();
    void flipVertical();
    CanvasDocument* getDocument() const {return document;}
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
        Ellipse,
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
    void paintColor(int x, int y, const QColor &color, bool recordUndo = true); // paint color into the corresponding square
    QList<QColor> sortColors(QHash<QRgb, int> colorFrequency); // sort a list of colors based on frequency
    QColor getColor(); // return the current color
    int getZoom(); // get the current zoom
    void setZoom(int zoom); // set the zoom amount
    void setHorizontalSymmetry(bool enabled); // set horizontal symmetry drawing
    void setVerticalSymmetry(bool enabled); // set vertical symmetry drawing
    void setTool(Tool tool); // set the current tool
    void setShape(ShapeType shape); // set shape
    void drawRectangle(QPoint topLeft, QPoint bottomRight, bool recordUndo = true); // function to draw a rectangle
    void drawCircle(QPoint topLeft, QPoint bottomRight, bool recordUndo = true); // function to draw a circle
    void drawEllipse(QPoint topLeft, QPoint bottomRight, bool recordUndo = true); // function to draw an ellipse
    void drawLine(QPoint start, QPoint end, bool recordUndo = true); // function to draw a straight line
    void makeTempLayer(); // make temporary layer for preview
    void removeTempLayer(); // remove temporary layer
    int getActiveLayer() const;
    void setBrushSize(int newSize); // change brush size
    void commitMove(); // confirm move
    void cancelMove(); // cancel move
    void commitPaste(); // confirm paste
    void cancelPaste(); // cancel paste
    void drawChecker(QPainter &painter); // draw the checkerboard in the background
    void drawSelectionPreview(QPainter &painter); // draws the dotted line that shows selection
    void drawOnionFrame(QPainter &painter, int frameIndex, float onionOpacity);
    QImage tintOnionFrame(QImage imageBefore, QImage imageAfter, QColor tint); // tint onion frame to different color
    void changeOnionSettings();
    // frame methods
    void duplicateFrame();
    void addFrame();
    void deleteFrame(int index);
    void copyFrame();
    void pasteFrame();
    void switchFrame(int index);
    int getCurrentFrame();
    int getFrameSize();
    int getFrameDuration();
    void setFrameDuration(int value);
    QImage renderFrame(int frameIndex);
    // Events
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    CanvasDocument *document;
    int pixelSize = 20; // pixel/zoom size
    int brushSize = 1;
    bool movingPicture; // boolean to check if an imported picture is being moved
    bool horizontalSymmetry = false; // bool for horizontal symmetry
    bool verticalSymmetry = false; // bool for vertical symmetry
    QPoint moveOffset; // point on the canvas for moving picture
    QColor currentColor = Qt::black; // selected color
    bool isDrawing = false; // bool to check if user is drawing
    bool isUndoing = false; // bool to check if user is undoing
    bool isErasing = false; // bool to check if user is erasing
    bool isPasting = false;
    int previousFrames = 1;
    int nextFrames = 1;
    float onionOpacity = 0.25;
    bool onionOn = true;
    // others


    QHash<QRgb, int> colorFrequency; // number of times color has appeared on current canvas
    Tool currentTool = Tool::Brush; // default tool
    ShapeType currentShape = ShapeType::Rectangle; // default shape
    Selection selection;
    Shape shape;
    QVector<PixelChange> currentAction; // vector to store current action
    int maxUndo = 5; // max number of undos (probably redundant atp. cbf to check)
signals:
    void colorChanged(QColor color); // signal to change the selected color
    void mousePositionChanged(int x, int y); // signal to change current mouse position in status bar

};


// class for previewing selected color


#endif // PIXELCANVAS_H