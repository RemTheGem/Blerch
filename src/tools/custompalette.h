#ifndef CUSTOMPALETTE_H
#define CUSTOMPALETTE_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

class CustomPalette : public QWidget
{
    Q_OBJECT
public:
    explicit CustomPalette(QWidget *parent = nullptr);
    void setColors(const QList<QColor> &colors);
    void loadGPL(const QString &fileName);
    void clearPalette();

signals:
    void colorSelected(QColor color);
    void paletteUpdatedCustom(QVector<QColor> colors); // signal to change the palette for imported palettes

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
private:
    QList<QColor> colors;
    int cellSize = 20;
    int spacing = 1;
    int columns = 9;
};

#endif // CUSTOMPALETTE_H
