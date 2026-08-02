#include "mediancut.h"
#include <map>
// Color quantization for turning pictures into pixel art
MedianCut::MedianCut() {}

// find the color range of a box based on channel
int MedianCut::colorRange(const ColorBox& box, int channel)
{
    int min = 255;
    int max = 0;
    int value;
    // check if we are looking for r, g or b
    for (const auto& c : box.colors){
        if(channel == 0){
            value = c.r;
        }
        else if(channel == 1){
            value = c.g;
        }
        else{
            value = c.b;
        }
        // get the min and max
        min = std::min(min, value);
        max = std::max(max, value);
    }
    // return it
    return max - min;
}
// method to split our boxes into two
std::pair<ColorBox, ColorBox> MedianCut::splitBox(ColorBox box)
{
    if(box.colors.size() <= 1){
        return {box, ColorBox{}};
    }
    int rRange = colorRange(box, 0);
    int gRange = colorRange(box, 1);
    int bRange = colorRange(box, 2);
    // default channel is red
    int channel = 0;
    if (gRange > rRange && gRange >= bRange)
        channel = 1;
    else if (bRange > rRange && bRange >= gRange)
        channel = 2;
    // sort the box, compare red, green or blue based on channel, then return which comes first
    std::sort(box.colors.begin(), box.colors.end(),[channel](const Color& a, const Color& b){
        if (channel == 0) return a.r < b.r;
        if (channel == 1) return a.g < b.g;
            return a.b < b.b;
    });
    int total = 0;
    // total no. of colors
    for(auto& c : box.colors)
        total += c.count;
    int half = total/2;
    int accumulated = 0;
    int splitIndex = 0;
    // find the actual middle based on count of each color
    for(int i= 0; i<box.colors.size(); i++){
        accumulated+= box.colors[i].count;
        if(accumulated >= half){
            // middle found
            splitIndex = i;
            break;
        }
    }
    ColorBox left;
    ColorBox right;
    // split into two
    left.colors.assign(box.colors.begin(), box.colors.begin() + splitIndex);
    right.colors.assign(box.colors.begin() + splitIndex, box.colors.end());
    // return the two sorted boxes
    return {left, right};
}
// get the average color of a box
QColor MedianCut::averageColor(const ColorBox& box)
{
    if(box.colors.empty())
        return QColor(0,0,0);
    long r = 0;
    long g = 0;
    long b = 0;
    long total = 0;
    // go through the colors in a box and add their RGB values
    for (const auto& c : box.colors){
        r += c.r * c.count;
        g += c.g * c.count;
        b += c.b * c.count;
        total += c.count;
    }
    // return the average value
    return QColor(r / total, g / total, b / total);
}
// main median cut method
std::vector<QColor> MedianCut::medianCut(const QImage& image, int paletteSize)
{
    // count the rgb values in the image and add them to a map
    std::map<std::tuple<int,int,int>,int> colorFrequency;
    for (int y = 0; y < image.height(); y++){
        for (int x = 0; x < image.width(); x++){
            QColor c = image.pixelColor(x, y);
            auto key = std::make_tuple(c.red(), c.green(), c.blue());
            colorFrequency[key]++;
        }
    }
    // put all colors into a box
    ColorBox first;
    for(auto& pair : colorFrequency){
        auto [r, g, b] = pair.first;
        first.colors.push_back({r,g,b,pair.second});
    }
    std::vector<ColorBox> boxes;
    boxes.push_back(first);
    while ((int)boxes.size() < paletteSize){
        // lambda to get da biggest box from current boxes
        auto largest = std::max_element(boxes.begin(), boxes.end(), [this](const ColorBox& a, const ColorBox& b){
            int boxA = std::max({colorRange(a,0), colorRange(a,1), colorRange(a,2)});
            int boxB = std::max({colorRange(b,0), colorRange(b,1), colorRange(b,2)});
            // to ensure we dont waste palette slots, we return the box with the bigger color range
            return boxA < boxB;
    });
        // keep splitting the box into smaller boxes until we have our desired palette size (16 rn. will make it user input later)
        auto split = splitBox(*largest);
        boxes.erase(largest);
        boxes.push_back(split.first);
        boxes.push_back(split.second);
    }
    // turn our boxes into colors and return it as a palette
    std::vector<QColor> palette;
    for (const auto& box : boxes)
        palette.push_back(averageColor(box));
    return palette;
}
// get the nearest color to the color needed from our available palette
QColor MedianCut::nearestColor(const QColor& c, const std::vector<QColor>& palette)
{
    int best = INT_MAX;
    QColor nearest;
    // go through the palette and calculate the distance between needed color and colors available
    for (const auto& p : palette){
        int dr = c.red()   - p.red();
        int dg = c.green() - p.green();
        int db = c.blue()  - p.blue();
        int dist = dr*dr + dg*dg + db*db;
        if (dist < best){
            best = dist;
            nearest = p;
        }
    }
    // return the closest color
    return nearest;
}