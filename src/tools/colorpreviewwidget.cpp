#include "colorpreviewwidget.h"

ColorPreviewWidget::ColorPreviewWidget(QWidget *parent){
}
void ColorPreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect rect(
        0,
        0,
        previewSize,
        previewSize
        );
    painter.setPen(Qt::black);
    painter.fillRect(rect, selectedColor);
    painter.drawRect(rect);

}
void ColorPreviewWidget::setPreviewColor(const QColor &color){
    selectedColor = color;
    update();
}
