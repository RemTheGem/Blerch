#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PixelCanvas.h"
#include "tools/palettewidget.h"
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
#include <QMenuBar>
#include <QLabel>
#include <QSplitter>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // sorry for the mess here. moving stuff around changes how they look in the UI. idk any better way lol
    // main setup
    auto canvas = new PixelCanvas(this);
    auto colorPreview = new ColorPreviewWidget(this);
    QToolButton *shapeButton = new QToolButton(this);
    setFocusPolicy(Qt::StrongFocus);
    shapeButton->setText("Shape");
    QMenu *shapeMenu = new QMenu(shapeButton);
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
    // shape menu setup
    shapeMenu->addAction("Rectangle", [=]{
        canvas->setTool(PixelCanvas::Tool::Shape);
        canvas->setShape(PixelCanvas::ShapeType::Rectangle);
    });
    shapeMenu->addAction("Circle", [=]{
        canvas->setTool(PixelCanvas::Tool::Shape);
        canvas->setShape(PixelCanvas::ShapeType::Circle);
    });
    shapeMenu->addAction("Line", [=]{
        canvas->setTool(PixelCanvas::Tool::Shape);
        canvas->setShape(PixelCanvas::ShapeType::Line);
    });
    // Layer setup
    QWidget *layerPanel = new QWidget(this);
    QWidget *spacer = new QWidget();
    QVBoxLayout *layerLayout = new QVBoxLayout(layerPanel);
    QHBoxLayout *layerButtons = new QHBoxLayout();
    layerList = new QListWidget(this);
    layerList->addItems(canvas->getLayerNames());
    layerList->setCurrentRow(0);
    addLayerButton = new QPushButton("+", this);
    removeLayerButton = new QPushButton("-", this);
    QPushButton *moveUpButton = new QPushButton("↑", this);
    QPushButton *moveDownButton = new QPushButton("↓", this);
    QPushButton *renameLayerButton = new QPushButton("Rename", this);
    QSlider *opacitySlider = new QSlider(Qt::Horizontal);
    QLabel * opacityLabel = new QLabel("Opacity");
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(100);
    layerLayout->addWidget(opacityLabel);
    layerLayout->addWidget(opacitySlider);
    layerLayout->addWidget(renameLayerButton);
    layerLayout->addWidget(layerList);
    layerButtons->addWidget(moveUpButton);
    layerButtons->addWidget(moveDownButton);
    layerButtons->addWidget(addLayerButton);
    layerButtons->addWidget(removeLayerButton);
    layerLayout->addLayout(layerButtons);
    // palette and symmetry buttons setup
    QWidget *paletteContainer = new QWidget(this);
    QVBoxLayout *paletteLayout = new QVBoxLayout(paletteContainer);
    paletteWidget *palette = new paletteWidget;
    QSlider *brushSizeSlider = new QSlider(Qt::Horizontal);
    QLabel *brushSizeLabel = new QLabel("Brush Size");
    brushSizeSlider->setRange(1,16);
    brushSizeSlider->setValue(1);
    QPushButton *horizontalSymmetryButton = new QPushButton("Horizontal Symmetry");
    QPushButton *verticalSymmetryButton = new QPushButton ("Vertical Symmetry");
    paletteLayout->addWidget(brushSizeLabel);
    paletteLayout->addWidget(brushSizeSlider);
    paletteLayout->addWidget(horizontalSymmetryButton);
    paletteLayout->addWidget(verticalSymmetryButton);
    horizontalSymmetryButton->setCheckable(true);
    verticalSymmetryButton->setCheckable(true);
    paletteLayout->addWidget(palette);
    paletteLayout->addStretch();
    // size policies
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    palette->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layerPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layerPanel->setMinimumWidth(100);
    paletteContainer->setFixedWidth(190);
    // menus
    QMenu *fileMenu = menuBar()->addMenu("File");
    QMenu *picToPixMenu = menuBar()->addMenu("Picture to Pixel");
    QMenu *helpMenu = menuBar()->addMenu("Help");
    // organization
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(paletteContainer);
    splitter->addWidget(scroll);
    splitter->addWidget(layerPanel);
    splitter->setSizes({190, 1100, 200});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    mainLayout->addWidget(splitter);
    setCentralWidget(container);
    // Toolbar
    QToolBar *toolbar = addToolBar("Palette");
    QAction *brushAction = toolbar->addAction("Brush");
    QAction *eraserAction = toolbar->addAction("Eraser");
    QAction *eyeDropperAction = toolbar->addAction("Eye Dropper");
    QAction *fillAction = toolbar->addAction("Fill");
    QAction *shapeAction = new QAction("Shape", this);
    QAction *pastePixels = new QAction("Paste", this);
    QAction *copyPixels = new QAction("Copy", this);
    addAction(copyPixels);
    addAction(pastePixels);
    shapeButton->setDefaultAction(shapeAction);
    shapeButton->setMenu(shapeMenu);
    shapeButton->setPopupMode(QToolButton::MenuButtonPopup);
    toolbar->addWidget(shapeButton);
    QAction *selectAction = toolbar->addAction("Select");
    QAction *moveAction = toolbar->addAction("Move");
    QAction *pickColor = toolbar->addAction("Pick Color");
    colorPreview->setFixedSize(20,20);
    toolbar->addWidget(colorPreview);
    toolbar->addSeparator();
    QAction *undo = toolbar->addAction("Undo");
    QAction *redo = toolbar->addAction("Redo");
    toolbar->addWidget(spacer);
    QAction *eraseBoard = toolbar->addAction("Clear Canvas");
    QAction *resizeCanvas = toolbar->addAction("Resize Canvas");
    toolbar->addSeparator();
    QAction *saveDrawing = fileMenu->addAction("Export PNG");
    QAction *saveProject = fileMenu->addAction("Save Project");
    QAction *loadProject = fileMenu->addAction("Open Project");
    QAction *loadPicture = fileMenu->addAction("Open Picture");
    QAction *shortcutsAction = helpMenu->addAction("Keyboard Shortcuts");
    QAction *openPicture = picToPixMenu->addAction("Open Picture");
    QAction *zoomIn = toolbar->addAction("+");
    QAction *zoomOut = toolbar->addAction("-");
    brushAction->setChecked(true); // default tool as brush
    brushAction->setCheckable(true);
    eraserAction->setCheckable(true);
    eyeDropperAction->setCheckable(true);
    fillAction->setCheckable(true);
    moveAction->setCheckable(true);
    selectAction->setCheckable(true);
    shapeAction->setCheckable(true);
    canvas->setTool(PixelCanvas::Tool::Brush);
    QActionGroup *toolGroup = new QActionGroup(this); // group tools together
    toolGroup->setExclusive(true); // make sure one can be selected at a time
    toolGroup->addAction(brushAction);
    toolGroup->addAction(eraserAction);
    toolGroup->addAction(eyeDropperAction);
    toolGroup->addAction(fillAction);
    toolGroup->addAction(moveAction);
    toolGroup->addAction(selectAction);
    toolGroup->addAction(shapeAction);
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
    moveAction->setShortcut(QKeySequence("M"));
    selectAction->setShortcut(QKeySequence("S"));
    moveUpButton->setShortcut(QKeySequence("Ctrl+U"));
    moveDownButton->setShortcut(QKeySequence("Ctrl+D"));
    addLayerButton->setShortcut(QKeySequence("Ctrl+L"));
    saveProject->setShortcut(QKeySequence("Ctrl+Shift+S"));
    loadProject->setShortcut(QKeySequence("Ctrl+O"));
    loadPicture->setShortcut(QKeySequence("Ctrl+P"));
    copyPixels->setShortcut(QKeySequence("Ctrl+C"));
    pastePixels->setShortcut(QKeySequence("Ctrl+V"));

    qDebug() << palette->width();

    // Connections (needs organizing -_-#)
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
        canvas->resetCanvas();
        }
    });
    connect(resizeCanvas, &QAction::triggered, [=](){

        bool okWidth;
        bool okHeight;
        int width = QInputDialog::getInt(
            this, "Canvas Width", "Width:", 32, 1, 512, 1, &okWidth);
        if(!okWidth)
            return;
        int height = QInputDialog::getInt(this, "Canvas Height", "Height:", 32, 1, 512, 1, &okHeight);
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
    connect(moveAction, &QAction::triggered, [=](){
        canvas->setTool(PixelCanvas::Tool::Move);
    });
    connect(selectAction, &QAction::triggered, [=](){
        canvas->setTool(PixelCanvas::Tool::Select);
    });
    connect(undo, &QAction::triggered, [=](){
        canvas->undo();
    });
    connect(redo, &QAction::triggered, [=](){
        canvas->redo();
    });
    connect(copyPixels, &QAction::triggered, [=](){
        canvas->copyPixels();
    });
    connect(pastePixels, &QAction::triggered, [=](){
        canvas->pastePixels();
    });
    connect(canvas, &PixelCanvas::colorChanged, colorPreview, &ColorPreviewWidget::setPreviewColor);
    connect(saveProject, &QAction::triggered, [=](){
        canvas->saveProject();
    });
    connect(loadProject, &QAction::triggered, [=](){
        canvas->loadProject();
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(0);
    });
    connect(loadPicture, &QAction::triggered, [=](){
        canvas->loadPicture();
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(layerList->count() - 1);
    });
    connect(openPicture, &QAction::triggered, [=](){
        canvas->pictureToPixel();
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(layerList->count() - 1);
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
    connect(canvas, &PixelCanvas::clearLayerList, [=](){
        int row = layerList->currentRow();
        if(row>=0){
            delete layerList->takeItem(row);
        }
    });
    connect(canvas, &PixelCanvas::reInitLayers, [=](){
        canvas->addLayer();
        layerList->addItem("Layer " + QString::number(layerList->count()+1));
        layerList->setCurrentRow(layerList->count()-1);
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
    connect(layerList, &QListWidget::currentRowChanged, [=](int row){
        canvas->setActiveLayer(row);
        opacitySlider->setValue(canvas->getLayerOpacity(row)*100);
    });
    connect(opacitySlider, &QSlider::valueChanged, [=](int value){
        canvas->setLayerOpacity(layerList->currentRow(), value / 100.0f);
    });
    connect(shortcutsAction, &QAction::triggered, [=](){

        QMessageBox::information(this,
                                 "Keyboard Shortcuts",

                                 "Tools:\n"
                                 "B  - Brush\n"
                                 "E  - Eraser\n"
                                 "F  - Fill\n\n"
                                 "I  - switch to Eyedropper\n"
                                 "S  - Select\n"
                                 "M  - Move\n"
                                 "Middle mouse button   - Use Eyedropper on current pixel\n"
                                 "Ctrl + W  - Color Picker\n\n"

                                 "Layers:\n"
                                 "Ctrl + L  - Add Layer\n"
                                 "Ctrl + U  - Move Layer Up\n"
                                 "Ctrl + D  - Move Layer Down\n\n"

                                 "Editing:\n"
                                 "Ctrl + Z  - Undo\n"
                                 "Ctrl + Y  - Redo\n"
                                 "Ctrl + P  - Load Picture\n"
                                 "Ctrl + S  - Save Picture\n"
                                 "Ctrl + O  - Load Project\n"
                                 "Ctrl + Shift + S  - Save Project\n\n"

                                 "View:\n"
                                 "Ctrl + Mouse Wheel  - Canvas Zoom\n"
                                 "Shift + Mouse Wheel   - Reference Image Zoom\n"
                                 "Mouse Wheel  - Scroll vertically\n"
                                 "Alt + Mouse Wheel  - Scroll Horizontally\n\n"
                                 "After moving something with the move tool\n press Enter to confirm or Esc to cancel move"
                                 );

    });
    connect(palette, &paletteWidget::colorSelected, canvas, &PixelCanvas::setColor);
    connect(canvas, &PixelCanvas::paletteUpdated, palette, &paletteWidget::setColors);
    connect(horizontalSymmetryButton, &QPushButton::toggled, canvas, &PixelCanvas::setHorizontalSymmetry);
    connect(verticalSymmetryButton, &QPushButton::toggled, canvas, &PixelCanvas::setVerticalSymmetry);
    connect(brushSizeSlider, &QSlider::valueChanged, [=](int value){
        canvas->setBrushSize(value);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}