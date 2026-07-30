#ifndef PALETTEWIDGET_H
#define PALETTEWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

class paletteWidget : public QWidget
{
    Q_OBJECT
public:
    explicit paletteWidget(QWidget *parent = nullptr);
    void setColors(const QList<QColor> &colors);
signals:
    void colorSelected(QColor color);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
private:
    QList<QColor> colors;
    int cellSize = 20;
    int spacing = 1;
    int columns = 9;
};

#endif // PALETTEWIDGET_H
