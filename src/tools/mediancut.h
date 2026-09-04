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
    int rMin, rMax, gMin, gMax, bMin, bMax;
    void computeRanges(){
        rMin = gMin = bMin = 255;
        rMax = gMax = bMax = 0;
        for(const auto& c : colors){
            rMin = std::min(rMin, (int)c.r);
            rMax = std::max(rMax, (int)c.r);
            gMin = std::min(gMin, (int)c.g);
            gMax = std::max(gMax, (int)c.g);
            bMin = std::min(bMin, (int)c.b);
            bMax = std::max(bMax, (int)c.b);
        }
    }
    int rRange() const {return rMax - rMin;}
    int gRange() const {return gMax - gMin;}
    int bRange() const {return bMax - bMin;}
    int maxRange() const {return std::max({rRange(), gRange(), bRange()});}
};
struct TupleHash{
    size_t operator()(const std::tuple<int, int, int>& t)const{
        auto [r,g,b] = t;
        return (r << 16) ^ (g << 8) ^b;
    }
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
