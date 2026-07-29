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
#include <QIcon>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto canvas = new PixelCanvas(this);
    auto colorPreview = new ColorPreviewWidget(this);
    QApplication::setApplicationName("Blerch");
    setWindowTitle("Blerch");
    setWindowIcon(QIcon(":/Blerch icon v2.png"));
    QWidget *container = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(0,0,0,0);
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidget(canvas);
    scroll->setWidgetResizable(false);
    scroll->setAlignment(Qt::AlignCenter);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Layer
    QWidget *layerPanel = new QWidget(this);
    layerPanel->setFixedWidth(200);
    QVBoxLayout *layerLayout = new QVBoxLayout(layerPanel);
    layerList = new QListWidget(this);
    layerList->addItems(canvas->getLayerNames());
    layerList->setCurrentRow(0);
    addLayerButton = new QPushButton("+", this);
    removeLayerButton = new QPushButton("-", this);
    QPushButton *moveUpButton = new QPushButton("↑", this);
    QPushButton *moveDownButton = new QPushButton("↓", this);
    QPushButton *renameLayerButton = new QPushButton("Rename", this);
    layerLayout->addWidget(renameLayerButton);
    layerLayout->addWidget(moveUpButton);
    layerLayout->addWidget(moveDownButton);
    layerLayout->addWidget(layerList);
    layerLayout->addWidget(addLayerButton);
    layerLayout->addWidget(removeLayerButton);
    mainLayout->addWidget(scroll, 1);
    mainLayout->addWidget(layerPanel, 0);
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
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(0);
    });
    connect(zoomIn, &QAction::triggered, [=](){
        canvas->setZoom(canvas->getZoom() + 2);
    });
    connect(zoomOut, &QAction::triggered, [=](){
        canvas->setZoom(canvas->getZoom() - 2);
    });
    connect(addLayerButton,&QPushButton::clicked,[=](){
        canvas->addLayer();
        layerList->addItem("Layer " + QString::number(layerList->count()+1));
        layerList->setCurrentRow(layerList->count()-1);
    });
    connect(removeLayerButton,&QPushButton::clicked,[=](){
        int row = layerList->currentRow();
        if(row >= 0){
            canvas->removeLayer(row);
            delete layerList->takeItem(row);
        }
    });
    connect(layerList,&QListWidget::currentRowChanged, canvas, &PixelCanvas::setActiveLayer);
    connect(moveUpButton, &QPushButton::clicked, [=](){
        int index = layerList->currentRow();
        if(index <0 || index >= layerList->count()-1) return;
        canvas->moveLayerUp(index);
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        qDebug() << canvas-> getLayerNames() << " layers";
        layerList->setCurrentRow(index + 1);
    });
    connect(moveDownButton, &QPushButton::clicked, [=](){
        int index = layerList->currentRow();
        if(index <=0 || index > layerList->count()-1) return;
        canvas->moveLayerDown(index);
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(index - 1);
    });
    connect(renameLayerButton, &QPushButton::clicked, [=](){
        int index = layerList->currentRow();
        if(index < 0) return;
        bool ok;
        QString name = QInputDialog::getText(this, "Rename Layer", "Layer name:",
                                            QLineEdit::Normal, layerList->currentItem()->text(),&ok);
        if(ok && !name.isEmpty())
        {
            canvas->renameLayer(index, name);
            layerList->item(index)->setText(name);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}