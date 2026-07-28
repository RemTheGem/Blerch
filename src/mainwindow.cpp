#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PixelCanvas.h"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QToolBar>
#include <QAction>
#include <QColorDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QActionGroup>
#include <QInputDialog>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto canvas = new PixelCanvas(this);
    auto colorPreview = new ColorPreviewWidget(this);
    QApplication::setApplicationName("Blerch");
    setWindowTitle("Blerch");
    QWidget *container = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(container);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidget(canvas);
    scroll->setWidgetResizable(false);
    scroll->setAlignment(Qt::AlignCenter);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(scroll);

    setCentralWidget(container);
    // Toolbar
    QToolBar *toolbar = addToolBar("Palette");
    QAction *brushAction = toolbar->addAction("Brush");
    QAction *eraserAction = toolbar->addAction("Eraser");
    QAction *eyeDropperAction = toolbar->addAction("Eye Dropper");
    QAction *fillAction = toolbar->addAction("Fill");

    QAction *pickColor = toolbar->addAction("Pick Color");
    colorPreview->setFixedSize(20,20);
    toolbar->addWidget(colorPreview);
    toolbar->addSeparator();
    QAction *undo = toolbar->addAction("Undo");
    QAction *redo = toolbar->addAction("Redo");
    QAction *eraseBoard = toolbar->addAction("Clear Canvas");
    QAction *resizeCanvas = toolbar->addAction("Resize Canvas");
    toolbar->addSeparator();
    QAction *saveDrawing = toolbar->addAction("Save Drawing");
    QAction *saveProject = toolbar->addAction("Save Project");
    QAction *loadProject = toolbar->addAction("Load Project");
    QAction *zoomIn = toolbar->addAction("+");
    QAction *zoomOut = toolbar->addAction("-");
    // UI
    brushAction->setChecked(true);
    brushAction->setCheckable(true);
    eraserAction->setCheckable(true);
    eyeDropperAction->setCheckable(true);
    fillAction->setCheckable(true);
    canvas->setTool(PixelCanvas::Tool::Brush);
    QActionGroup *toolGroup = new QActionGroup(this);
    toolGroup->setExclusive(true);
    toolGroup->addAction(brushAction);
    toolGroup->addAction(eraserAction);
    toolGroup->addAction(eyeDropperAction);
    toolGroup->addAction(fillAction);
    showMaximized();

    // keyboard shortcuts
    pickColor->setShortcut(QKeySequence("Ctrl+W"));
    undo->setShortcut(QKeySequence("Ctrl+Z"));
    redo->setShortcut(QKeySequence("Ctrl+Y"));
    saveDrawing->setShortcut(QKeySequence("Ctrl+S"));
    brushAction->setShortcut(QKeySequence("B"));
    eraserAction->setShortcut(QKeySequence("E"));
    eyeDropperAction->setShortcut(QKeySequence("I"));
    fillAction->setShortcut(QKeySequence("F"));
    //Toolbar actions
    connect(pickColor, &QAction::triggered, [=]() {
        QColor color = QColorDialog::getColor(Qt::white, this);
        if (color.isValid()) {
            canvas->setColor(color);
            colorPreview->selectedColor = canvas->getColor();
            update();
        }
    });
    connect(eraseBoard, &QAction::triggered, [=](){
        reply = QMessageBox::warning(this, "Clear Canvas?", "All progress may be lost. Clear Canvas?", QMessageBox::Yes | QMessageBox::Cancel);
        if(reply == QMessageBox::Yes){
        canvas->clear();
        }
    });
    connect(resizeCanvas, &QAction::triggered, [=](){

        bool okWidth;
        bool okHeight;
        int width = QInputDialog::getInt(
            this, "Canvas Width", "Width:", 32, 1, 256, 1, &okWidth);
        if(!okWidth)
            return;
        int height = QInputDialog::getInt(this, "Canvas Height", "Height:", 32, 1, 256, 1, &okHeight);
        if(!okHeight)
            return;
        canvas->resizeCanvas(width, height);
    });
    connect(saveDrawing, &QAction::triggered, [=]() {
        canvas->saveImage();
    });
    connect(brushAction, &QAction::triggered, [=](){
        canvas->setTool(PixelCanvas::Tool::Brush);
    });
    connect(eraserAction, &QAction::triggered, [=](){
        canvas->setTool(PixelCanvas::Tool::Eraser);
    });
    connect(eyeDropperAction, &QAction::triggered, [=](){
        canvas->setTool(PixelCanvas::Tool::EyeDropper);
    });
    connect(fillAction, &QAction::triggered, [=](){
        canvas->setTool(PixelCanvas::Tool::Fill);
    });
    connect(undo, &QAction::triggered, [=](){
        canvas->undo();
    });
    connect(redo, &QAction::triggered, [=](){
        canvas->redo();
    });
    connect(canvas, &PixelCanvas::colorChanged, colorPreview, &ColorPreviewWidget::setColor);
    connect(saveProject, &QAction::triggered, [=](){
        canvas->saveProject();
    });
    connect(loadProject, &QAction::triggered, [=](){
        canvas->loadProject();
    });
    connect(zoomIn, &QAction::triggered, [=](){
        canvas->setZoom(canvas->getZoom() + 2);
    });
    connect(zoomOut, &QAction::triggered, [=](){
        canvas->setZoom(canvas->getZoom() - 2);
    });


}

MainWindow::~MainWindow()
{
    delete ui;
}