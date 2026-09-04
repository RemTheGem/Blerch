#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "PixelCanvas.h"
#include "File IO/filehandling.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QMessageBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void loadProject(const QString &filePath = QString());
    void saveProject(const QString &filePath = "");
    void autosaveProject();
    void saveSpriteSheet(const QString &filePath = "", int columns = 5, int scale = 1);
    void saveGIF(const QString &filePath = "", int scale = 16);
    void playAnimation();
    void pauseAnimation();
    void updateTimeline();
    int uiToDocumentLayer(int uiIndex);
    int documentToUiLayer(int documentIndex);
    void initializeRecovery();


private:
    Ui::MainWindow *ui;
    QMessageBox::StandardButton reply;
    QListWidget *layerList;
    QPushButton *addLayerButton;
    QPushButton *removeLayerButton;
    CanvasDocument *document;
    PixelCanvas *canvas;
    FileHandling *fileHandling;
    QMenu *recentFilesMenu;
    QTimer *animationTimer;
    QVBoxLayout *frameLayout;
    QHBoxLayout *frameButtonsLayout;
    QSpinBox *durationSpinBox;
    QPushButton *onionSkinActivationButton;
    QVector<QPushButton*> frameButtons;
    QTimer *autosaveTimer;
    QString lockPath;
    void updateRecentFiles();
};
#endif // MAINWINDOW_H
