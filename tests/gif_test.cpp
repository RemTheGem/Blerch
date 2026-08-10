#include <QCoreApplication>
#include <QImage>
#include <QDebug>

#include "../thirdParty/gif-h/gif.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const int width = 32;
    const int height = 32;

    QImage red(32, 32, QImage::Format_RGBA8888);
    red.fill(Qt::red);

    QImage blue(32, 32, QImage::Format_RGBA8888);
    blue.fill(Qt::blue);

    GifWriter writer = {};

    GifBegin(&writer, "test1.gif", 32, 32, 10);

    GifWriteFrame(&writer,
                  red.constBits(),
                  32, 32,
                  10);

    GifWriteFrame(&writer,
                  blue.constBits(),
                  32, 32,
                  10);

    GifEnd(&writer);

    qDebug() << "GIF created!";

    return 0;
}