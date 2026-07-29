#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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



private:
    Ui::MainWindow *ui;
    QMessageBox::StandardButton reply;
    QListWidget *layerList;
    QPushButton *addLayerButton;
    QPushButton *removeLayerButton;
};
#endif // MAINWINDOW_H
