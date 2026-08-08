#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "PixelCanvas.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QMessageBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void loadProject(const QString &filePath = QString());
    void saveProject(const QString &filePath = "");
    void playAnimation();
    void pauseAnimation();
    void updateTimeline();



private:
    Ui::MainWindow *ui;
    QMessageBox::StandardButton reply;
    QListWidget *layerList;
    QPushButton *addLayerButton;
    QPushButton *removeLayerButton;
    PixelCanvas *canvas;
    QMenu *recentFilesMenu;
    QTimer *animationTimer;
    QVBoxLayout *frameLayout;
    QHBoxLayout *frameButtonsLayout;

    void updateRecentFiles();
};
#endif // MAINWINDOW_H
