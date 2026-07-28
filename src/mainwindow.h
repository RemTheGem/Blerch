#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QColor>
#include <QPaintEvent>
#include <QMessageBox>


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
    static const int gridSize = 20;
    QColor pixels[32][32];
    QMessageBox::StandardButton reply;
};
#endif // MAINWINDOW_H
