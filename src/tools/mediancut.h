#ifndef MEDIANCUT_H
#define MEDIANCUT_H
#include <vector>
#include <QColor>
#include <QImage>




struct Color {
    int r, g, b, count;
};

struct ColorBox {
    std::vector<Color> colors;
};


class MedianCut
{

public:
    int colorRange(const ColorBox& box, int channel);
    std::pair<ColorBox, ColorBox> splitBox(ColorBox box);
    QColor averageColor(const ColorBox& box);
    std::vector<QColor> medianCut(const QImage& image, int palleteSize);
    QColor nearestColor(const QColor& c, const std::vector<QColor>& palette);
    MedianCut();


};

#endif // MEDIANCUT_H
