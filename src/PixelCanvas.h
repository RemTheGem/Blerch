#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H
#include "model/CanvasTypes.h"
#include "model/canvasdocument.h"
#include <QWidget>
#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
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
        // Transform variables
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        QPointF pivot;
        QImage sourceImage;
        enum class Handle{ None, Move, TopLeft, TopRight, BottomLeft, BottomRight};
        Handle activeHandle = Handle::None;

        bool isEmpty(const Selection& s){
            return s.colors.empty();
        }
        void setValues(Selection s){
            dragStart = s.selectionOffset;
        }

    };


    // main functions
    void setColor(const QColor &c) {currentColor = c; emit colorChanged(c);} // set the current color
    void setDarkMode(bool mode) {darkMode = mode; update();}
    bool getDarkMode(){return darkMode;}
    void clear(); // clear the canvas on the current layer
    void resetCanvas(); // reset the whole canvas

    void updateCanvasSize(); // update the current canvas size
    void floodFill(int x, int y); // method for flood fill tool

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
    enum class BrushMode{
        Normal,
        Shade,
        Lighten,
        Blend
    };
    enum class BrushApplication{
        Continuous,
        OnePassPerStroke
    };

    struct Shape{
        QPoint start;
        QPoint end;
        int width;
        int height;
    };

    // helper methods
    void paintColor(int x, int y, const QColor &color, bool recordUndo = true); // paint color into the corresponding square
    void setPixel(int x, int y, const QColor &color, bool recordUndo = true); // same as paint color but for individual pixels
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
    void setBrushSize(int newSize); // change brush size
    void commitMove(); // confirm move
    void cancelMove(); // cancel move
    void commitPaste(); // confirm paste
    void cancelPaste(); // cancel paste
    void drawChecker(QPainter &painter); // draw the checkerboard in the background
    void drawSelectionPreview(QPainter &painter); // draws the dotted line that shows selection
    void paintLine(int x0, int y0, int x1, int y1, const std::function<QColor(int, int)> &colorAt, bool recordUndo = true);
    Selection::Handle hitTransformHandle(QPointF pos);
    void rebuildTransformPreview();
    QImage makeTransformedImage();
    void beginFloatingSelection();
    void placeStrokePixel(int x, int y, const QColor &color, bool recordUndo = false);
    QVector<QPoint> mirroredPointsPixelPerfect(const QPoint &point);
    void undoStrokePixel(const QPoint &point);
    void setPixelPerfect(bool enabled) { pixelPerfect = enabled;}
    bool isPixelPerfect() const {return pixelPerfect;}
    // onion methods
    void drawOnionFrame(QPainter &painter, int frameIndex, float onionOpacity);
    QImage tintOnionFrame(QImage imageBefore, QImage imageAfter, QColor tint); // tint onion frame to different color
    void changeOnionSettings();
    void setOnionOn(bool value);
    void setOnionOpacity(float value);
    void setPreviousFrames(int value);
    void setNextFrames(int value);
    // shade methods
    void setBrushMode(BrushMode mode);
    void setBrushAmount(float amount);
    QColor getBrushColor(const QColor &pixel);
    QColor shadePixel(const QColor &color);
    QColor lightenPixel(const QColor &color);
    QColor blendPixel(const QColor &color, const QColor &blendColor);
    void setBrushApplication(BrushApplication type);
    // frame methods
    void switchFrame(int index);

    // autosave
    bool autosaveDirty = false;
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
    bool movingPicture = false; // boolean to check if an imported picture is being moved
    bool horizontalSymmetry = false; // bool for horizontal symmetry
    bool verticalSymmetry = false; // bool for vertical symmetry
    QPoint moveOffset; // point on the canvas for moving picture
    QColor currentColor = Qt::black; // selected color
    bool isDrawing = false; // bool to check if user is drawing
    bool isErasing = false; // bool to check if user is erasing
    bool isPasting = false;
    bool darkMode = true;
    int previousFrames = 1;
    int nextFrames = 1;
    float onionOpacity = 0.15;
    bool onionOn = true;
    QColor previousFramesColor = Qt::red;
    QColor nextFramesColor = Qt::green;
    BrushMode brushMode = BrushMode::Normal;
    float brushAmount = 0.1f;
    BrushApplication brushApplication = BrushApplication::OnePassPerStroke;
    QSet<QPair<int, int>> affectedPixels;
    QPoint lastPaintPos;
    static constexpr double handleHalfSize = 4.0;
    bool pixelPerfect = false;
    QVector<QPoint> strokeHistory;
    // others


    Tool currentTool = Tool::Brush; // default tool
    ShapeType currentShape = ShapeType::Rectangle; // default shape
    Selection selection;
    Shape shape;
    QVector<PixelChange> currentAction; // vector to store current action
signals:
    void colorChanged(QColor color); // signal to change the selected color
    void mousePositionChanged(int x, int y); // signal to change current mouse position in status bar
    void brushModeChanged(BrushMode mode);
    void switchBackToSelect();

};


// class for previewing selected color


#endif // PIXELCANVAS_H