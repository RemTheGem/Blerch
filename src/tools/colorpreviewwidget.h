#ifndef COLORPREVIEWWIDGET_H
#define COLORPREVIEWWIDGET_H
#include <QWidget>
#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QPainter>

class ColorPreviewWidget : public QWidget{
    Q_OBJECT
    int previewSize = 20; // size of the preview pixel
protected:
    void paintEvent(QPaintEvent *event) override;
public:
    explicit ColorPreviewWidget(QWidget *parent = nullptr);
    QColor selectedColor = Qt::black; // default selected color
    void setPreviewColor(const QColor &color); // method for changing the selected color
};
#endif // COLORPREVIEWWIDGET_H
