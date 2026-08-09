#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PixelCanvas.h"
#include "tools/palettewidget.h"
#include "tools/custompalette.h"
#include "../include/settingsmanager.h"
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
#include <QFileDialog>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QFormLayout>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // sorry for the mess here. moving stuff around changes how they look in the UI. idk any better way lol
    // main setup
    canvas = new PixelCanvas(this);
    auto colorPreview = new ColorPreviewWidget(this);
    QToolButton *shapeButton = new QToolButton(this);
    setFocusPolicy(Qt::StrongFocus);
    shapeButton->setText("Shape");
    QMenu *shapeMenu = new QMenu(shapeButton);
    QApplication::setApplicationName("Blerch");
    setWindowTitle("Blerch");
    setWindowIcon(QIcon(":/Blerch icon v2.png"));
    QWidget *container = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0,0,0,0);
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidget(canvas);
    scroll->setWidgetResizable(false);
    scroll->setAlignment(Qt::AlignCenter);
    QWidget *frameContainer = new QWidget;
    frameLayout = new QVBoxLayout(frameContainer);
    QScrollArea *frameScroll = new QScrollArea;
    frameScroll->setWidgetResizable(true);
    frameScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    frameScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *frameButtonsContainer = new QWidget;
    frameButtonsLayout = new QHBoxLayout(frameButtonsContainer);
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    QPushButton *addFrameButton = new QPushButton("Add Frame");
    QPushButton *deleteFrameButton = new QPushButton("Remove Frame");
    QPushButton *playAnimationButton = new QPushButton("▶");
    QPushButton *pauseAnimationButton = new QPushButton("⏸");
    durationSpinBox = new QSpinBox(this);
    durationSpinBox->setRange(1, 60000);
    durationSpinBox->setValue(100);
    durationSpinBox->setSuffix(" ms");
    controlsLayout->addWidget(addFrameButton);
    controlsLayout->addWidget(deleteFrameButton);
    controlsLayout->addWidget(playAnimationButton);
    controlsLayout->addWidget(pauseAnimationButton);
    controlsLayout->addWidget(durationSpinBox);
    frameScroll->setWidget(frameButtonsContainer);
    frameLayout->addWidget(frameScroll);
    frameLayout->addLayout(controlsLayout);
    animationTimer = new QTimer(this);
    // settings
    QString lastProject = SettingsManager::instance().getLastProject();
    QString previousPalette = SettingsManager::instance().getCustomPalette();

    // shape menu setup
    shapeMenu->addAction("Rectangle", [=]{
        canvas->setTool(PixelCanvas::Tool::Shape);
        canvas->setShape(PixelCanvas::ShapeType::Rectangle);
    });
    shapeMenu->addAction("Circle", [=]{
        canvas->setTool(PixelCanvas::Tool::Shape);
        canvas->setShape(PixelCanvas::ShapeType::Circle);
    });
    shapeMenu->addAction("Ellipse", [=]{
        canvas->setTool(PixelCanvas::Tool::Shape);
        canvas->setShape(PixelCanvas::ShapeType::Ellipse);
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
    paletteWidget *paletteW = new paletteWidget;
    CustomPalette *customPalette = new CustomPalette;
    QSlider *brushSizeSlider = new QSlider(Qt::Horizontal);
    QLabel *brushSizeLabel = new QLabel("Brush Size");
    QLabel *paletteLabel = new QLabel("Frequent Colors");
    QLabel *customPaletteLabel = new QLabel("Palette");
    QComboBox *paletteSelector = new QComboBox(this);
    paletteSelector->addItem("None Selected...", "");
    paletteSelector->addItem("Pico-8", ":/palettes/pico-8.gpl");
    paletteSelector->addItem("Sweetie 16", ":/palettes/sweetie-16.gpl");
    paletteSelector->addItem("DawnBringer 16", ":/palettes/dawnbringer-16.gpl");
    paletteSelector->addItem("DawnBringer 32", ":/palettes/dawnbringer-32.gpl");
    paletteSelector->addItem("Endesga 32", ":/palettes/endesga-32.gpl");
    paletteSelector->addItem("Resurrect 64", ":/palettes/resurrect-64.gpl");
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
    paletteLayout->addWidget(customPaletteLabel);
    paletteLayout->addWidget(paletteSelector);
    paletteLayout->addWidget(customPalette);
    paletteLayout->addWidget(paletteLabel);
    paletteLayout->addWidget(paletteW);
    paletteLayout->addStretch();
    // size policies
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    paletteW->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layerPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layerPanel->setMinimumWidth(100);
    paletteContainer->setFixedWidth(190);
    // menus
    QMenu *fileMenu = menuBar()->addMenu("File");
    QMenu *picToPixMenu = menuBar()->addMenu("Picture to Pixel");
    QMenu *canvasMenu = menuBar()->addMenu("Canvas");
    QMenu *exportMenu = menuBar()->addMenu("Export");
    QMenu *helpMenu = menuBar()->addMenu("Help");
    // organization
    QSplitter *canvasSplitter = new QSplitter(Qt::Vertical);
    canvasSplitter->addWidget(scroll);
    canvasSplitter->addWidget(frameContainer);
    canvasSplitter->setStretchFactor(0,1);
    canvasSplitter->setStretchFactor(1,0);
    canvasSplitter->setSizes({600, 100});
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(paletteContainer);
    splitter->addWidget(canvasSplitter);
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
    toolbar->addSeparator();
    QAction *loadPicture = fileMenu->addAction("Open Reference Picture");
    QAction *loadProjectAction = fileMenu->addAction("Open Project");
    QAction *loadLastProject = fileMenu->addAction("Open Last Project");
    recentFilesMenu = fileMenu->addMenu("Recent Files");
    QAction *loadPalette = fileMenu->addAction("Import Palette");
    QAction *shortcutsAction = helpMenu->addAction("Keyboard Shortcuts");
    QAction *openPicture = picToPixMenu->addAction("Open Picture");
    QAction *flipHorizontal = canvasMenu->addAction("Flip Horizontal");
    QAction *flipVertical = canvasMenu->addAction("Flip Vertical");
    QAction *resizeCanvas = canvasMenu->addAction("Resize Canvas");
    QAction *eraseBoard = canvasMenu->addAction("Clear Canvas");
    QAction *saveDrawing = exportMenu->addAction("Export PNG");
    QAction *saveProjectAction = exportMenu->addAction("Save Project");
    QAction *saveSpriteSheetAction = exportMenu->addAction("Export as Sprite Sheet");
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
    // status Bar
    QLabel *positionLabel = new QLabel("X: 0 Y: 0", this);
    statusBar()->addPermanentWidget(positionLabel);
    QLabel *canvasSizeLabel = new QLabel("32x32", this);
    statusBar()->addPermanentWidget(canvasSizeLabel);
    // laod settings
    if(!previousPalette.isEmpty()){
        customPalette->loadGPL(previousPalette);
    }
    updateRecentFiles();
    updateTimeline();

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
    saveProjectAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    loadProjectAction->setShortcut(QKeySequence("Ctrl+O"));
    loadPicture->setShortcut(QKeySequence("Ctrl+P"));
    copyPixels->setShortcut(QKeySequence("Ctrl+C"));
    pastePixels->setShortcut(QKeySequence("Ctrl+V"));

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
        statusBar()->showMessage("Undo", 2000);
    });
    connect(redo, &QAction::triggered, [=](){
        canvas->redo();
        statusBar()->showMessage("Redo", 2000);
    });
    connect(copyPixels, &QAction::triggered, [=](){
        canvas->copyPixels();
        statusBar()->showMessage("Copied!", 2000);
    });
    connect(pastePixels, &QAction::triggered, [=](){
        canvas->pastePixels();
        statusBar()->showMessage("Left Click or press enter to confirm. Esc to Cancel", 5000);
    });
    connect(canvas, &PixelCanvas::colorChanged, colorPreview, &ColorPreviewWidget::setPreviewColor);
    connect(saveProjectAction, &QAction::triggered, [=](){
        saveProject();
    });
    connect(saveSpriteSheetAction, &QAction::triggered, [=](){
        QDialog dialog(this);
        dialog.setWindowTitle("Export Sprite Sheet");
        QFormLayout *layout = new QFormLayout(&dialog);
        QSpinBox *columnSpin = new QSpinBox(&dialog);
        columnSpin->setRange(1,100);
        columnSpin->setValue(5);

        QSpinBox *scaleSpin = new QSpinBox(&dialog);
        scaleSpin->setRange(1,16);
        scaleSpin->setValue(1);
        layout->addRow("Columns:", columnSpin);
        layout->addRow("Scale:", scaleSpin);
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if(dialog.exec() == QDialog::Accepted){
            int cols = columnSpin->value();
            int scale = scaleSpin->value();
            saveSpriteSheet("", cols, scale);
        }
    });
    connect(loadProjectAction, &QAction::triggered, [=](){
        loadProject();
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(0);
        updateTimeline();
        statusBar()->showMessage("Project Loaded!", 4000);
    });
    connect(loadLastProject, &QAction::triggered, [=](){

        loadProject(lastProject);
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(0);
        statusBar()->showMessage("Project Loaded!", 4000);
    });
    connect(canvas, &PixelCanvas::frameChanged, [=](){
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
        layerList->setCurrentRow(0);
        updateTimeline();
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
    connect(loadPalette, &QAction::triggered, this, [=]{
        QString fileName = QFileDialog::getOpenFileName(this, "Open Palette", "", "GPL File (*.gpl)");
        if (!fileName.isEmpty()){
            customPalette->loadGPL(fileName);
            QString paletteName = QFileInfo(fileName).baseName();
            int customIndex = paletteSelector->findData("custom");
            if(customIndex == -1){
                QVariantMap customData;
                customData["type"] = "custom";
                customData["path"] = fileName;
                paletteSelector->addItem(paletteName, customData);
            }
            else{
                paletteSelector->setItemText(customIndex, paletteName);
            }
            paletteSelector->setCurrentIndex(paletteSelector->findText(paletteName));
            SettingsManager::instance().setCustomPalette(fileName);
        }
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
    connect(moveUpButton, &QPushButton::clicked, [=](){
        int index = layerList->currentRow();
        if(index <0 || index >= layerList->count()-1) return;
        canvas->moveLayerUp(index);
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());
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
    connect(canvas, &PixelCanvas::layerChanged, this, [=]() {
        layerList->clear();
        layerList->addItems(canvas->getLayerNames());

        if (layerList->count() > 0)
            layerList->setCurrentRow(canvas->getActiveLayer());
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
                                 "Ctrl + C  - Copy Selection\n"
                                 "Ctrl + V  - Paste Selection\n"
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
    connect(paletteW, &paletteWidget::colorSelected, canvas, &PixelCanvas::setColor);
    connect(canvas, &PixelCanvas::paletteUpdated, paletteW, &paletteWidget::setColors);
    connect(customPalette, &CustomPalette::colorSelected, canvas, &PixelCanvas::setColor);
    connect(customPalette, &CustomPalette::paletteUpdatedCustom, customPalette, &CustomPalette::setColors);
    connect(paletteSelector, &QComboBox::currentIndexChanged, this, [=](int index){
        QVariant data = paletteSelector->itemData(index);
        if(data.typeId() == QMetaType::QString){
            QString path = data.toString();
            if(path.isEmpty()){
                customPalette->clearPalette();
                return;
            }
            customPalette->loadGPL(path);
        }
        else if(data.typeId() == QMetaType::QVariantMap){
            QVariantMap map = data.toMap();
            customPalette->loadGPL(map["path"].toString());
        }
    });
    connect(flipHorizontal, &QAction::triggered, [=](){
        canvas->flipHorizontal();
    });
    connect(flipVertical, &QAction::triggered, [=](){
        canvas->flipVertical();
    });
    connect(horizontalSymmetryButton, &QPushButton::toggled, canvas, &PixelCanvas::setHorizontalSymmetry);
    connect(verticalSymmetryButton, &QPushButton::toggled, canvas, &PixelCanvas::setVerticalSymmetry);
    connect(brushSizeSlider, &QSlider::valueChanged, [=](int value){
        canvas->setBrushSize(value);
    });
    connect(canvas, &PixelCanvas::mousePositionChanged, this, [=](int x, int y){
        positionLabel->setText(QString("X: %1 Y: %2 ").arg(x).arg(y));
    });
    connect(canvas, &PixelCanvas::canvasSizeChanged, this, [=](int x, int y){
        canvasSizeLabel->setText(QString(" %1x%2 ").arg(x).arg(y));
    });
    connect(addFrameButton, &QPushButton::clicked, this, [this](){
        canvas->addFrame();
        updateTimeline();
    });
    connect(deleteFrameButton, &QPushButton::clicked, this, [this](){
        canvas->deleteFrame(canvas->getCurrentFrame());
        updateTimeline();
    });
    connect(playAnimationButton, &QPushButton::clicked, this, [this](){
        playAnimation();
        updateTimeline();
    });
    connect(pauseAnimationButton, &QPushButton::clicked, this, [this](){
        pauseAnimation();
        updateTimeline();
    });
    connect(animationTimer, &QTimer::timeout, this, [this](){
        int next = canvas->getCurrentFrame() + 1;
        if (next >= canvas->getFrameSize()) next = 0;
        canvas->switchFrame(next);
        durationSpinBox->blockSignals(true);
        durationSpinBox->setValue(canvas->getFrameDuration());
        durationSpinBox->blockSignals(false);
        animationTimer->start(canvas->getFrameDuration());
        updateTimeline();
    });
    connect(durationSpinBox, &QSpinBox::valueChanged, this, [=](int value){
                canvas->setFrameDuration(value);
    });
}
void MainWindow::playAnimation(){
    animationTimer->start(canvas->getFrameDuration());
}
void MainWindow::pauseAnimation(){
    animationTimer->stop();
}
void MainWindow::updateTimeline(){
    while (QLayoutItem *item = frameButtonsLayout->takeAt(0)){
        delete item->widget();
        delete item;
    }
    for (int i = 0; i < canvas->getFrameSize(); i++) {
        QPushButton *button = new QPushButton(QString::number(i + 1));
        button->setCheckable(true);
        button->setChecked(i == canvas->getCurrentFrame());
        connect(button, &QPushButton::clicked, this, [this, i]() {
            canvas->switchFrame(i);
            durationSpinBox->setValue(canvas->getFrameDuration());
            updateTimeline();
        });
        frameButtonsLayout->addWidget(button);
    }
}
void MainWindow::loadProject(const QString &filePath){
    QString path = filePath;
    if(path.isEmpty()){
        path = QFileDialog::getOpenFileName(this, "Open Project", "", "JSON file (*.json)");
    }
    if(path.isEmpty()) return;
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());

    canvas->loadFromJson(doc.object());

    SettingsManager::instance().setLastProject(path);
    SettingsManager::instance().addRecentFile(path);
}
void MainWindow::saveProject(const QString &filePath){
    QString path = filePath;
    if(path.isEmpty()){
        path = QFileDialog::getSaveFileName(this, "Save Project", "", "Pixel Project (*.json)");
        if(path.isEmpty())return;
    }
    if(!path.endsWith(".json")) path += ".json";
    canvas->saveProject(path);
    SettingsManager::instance().setLastProject(path);
    SettingsManager::instance().addRecentFile(path);
}
void MainWindow::saveSpriteSheet(const QString &filePath, int columns, int scale){
    QString path = filePath;
    if(path.isEmpty()){
        path = QFileDialog::getSaveFileName(this, "Save Sprite Sheet", "", "PNG Image (*.png)");
    }
    if(!path.isEmpty()){
        canvas->saveSpriteSheet(path, columns, scale);
    }
}
void MainWindow::updateRecentFiles(){
    recentFilesMenu->clear();
    QStringList files = SettingsManager::instance().getRecentFiles();
    if(files.empty()){
        QAction *empty = recentFilesMenu->addAction("No Recent Files");
        empty->setEnabled(false);
        return;
    }
    for(QString file :files){
        if(!QFile::exists(file)) continue;
        QAction *action = recentFilesMenu->addAction(QFileInfo(file).fileName());
        action->setData(file);
        connect(action, &QAction::triggered,this, [=]{
            loadProject(action->data().toString());
            layerList->clear();
            layerList->addItems(canvas->getLayerNames());
            layerList->setCurrentRow(0);
            statusBar()->showMessage("Project Loaded!", 4000);
        });
    }
}
MainWindow::~MainWindow()
{
    delete ui;
}