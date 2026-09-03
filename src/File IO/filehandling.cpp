#include "filehandling.h"
#include "../../thirdParty/gif-h/gif.h"
#include <QPainter>
#include <QJsonArray>
#include <QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QImageReader>

FileHandling::FileHandling(CanvasDocument *document, PixelCanvas *canvas) : document(document), canvas(canvas) {
    connect(document, &CanvasDocument::paletteUpdated, this, &FileHandling::setPalette);
}
void FileHandling::saveImage(const QString &path)
{
    QImage image(document->currentFrame_().layers[0].width * canvas->getZoom(), document->currentFrame_().layers[0].height * canvas->getZoom(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);

    for(const auto &layer : document->currentFrame_().layers)
    {
        if(!layer.visible) continue;
        for(int y = 0; y < layer.height; y++)
        {
            for(int x = 0; x < layer.width; x++)
            {
                QColor color = layer.at(x,y);
                if(color != Qt::transparent)
                {
                    QRect rect(x * canvas->getZoom(), y * canvas->getZoom(), canvas->getZoom(), canvas->getZoom());
                    painter.fillRect(rect, color);
                }
            }
        }
    }
    image.save(path);

}
void FileHandling::saveProject(const QString &path)
{
    QJsonObject root;
    root["Width"] = document->currentFrame_().layers[0].width;
    root["Height"] = document->currentFrame_().layers[0].height;
    QJsonArray frameArray;
    for(const auto &frame:document->allFrames()){
        QJsonObject frameObject;
        frameObject["duration"] = frame.duration;
        QJsonArray layerArray;

        for(const auto &layer : frame.layers){
            QJsonObject layerObject;
            layerObject["name"] = layer.name;
            layerObject["visible"] = layer.visible;
            layerObject["opacity"] = layer.opacity;
            QByteArray pixelData;
            pixelData.resize(layer.width * layer.height * 4);
            int index = 0;
            for(int y = 0; y < layer.height; y++){
                for(int x = 0; x < layer.width; x++){
                    QColor color = layer.at(x,y);
                    pixelData[index++] = static_cast<char>(color.red());
                    pixelData[index++] = static_cast<char>(color.green());
                    pixelData[index++] = static_cast<char>(color.blue());
                    pixelData[index++] = static_cast<char>(color.alpha());
                }
            }
            QByteArray compressed = qCompress(pixelData, 9);
            layerObject["pixels"] = QString::fromLatin1(compressed.toBase64());
            layerArray.append(layerObject);
        }
        frameObject["layers"] = layerArray;
        frameArray.append(frameObject);
    }
    root["frames"] = frameArray;
    QJsonDocument doc(root);
    QFile file(path);
    if(file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}
void FileHandling::loadPicture(const QString &path)
{
    Layer layer;
    layer.type = LayerType::Reference;
    layer.name = QFileInfo(path).baseName();
    layer.image.load(path);
    layer.width = document->currentFrame_().layers[0].width;
    layer.height = document->currentFrame_().layers[0].height;
    document->currentFrame_().layers.push_back(layer);
    document->setActiveLayer(document->currentFrame_().layers.size()-1);
    emit documentUpdated();
}
void FileHandling::loadFromJson(QJsonObject root)
{

    int width = root["Width"].toInt();
    int height = root["Height"].toInt();

    QJsonArray frameArray = root["frames"].toArray();
    if (frameArray.isEmpty()) return;
    QList<Frame> loadedFrames;
    for(const auto &frameValue : frameArray){
        QJsonObject frameObject = frameValue.toObject();
        Frame frame;
        frame.duration = frameObject["duration"].toInt(100);
        QJsonArray layerArray = frameObject["layers"].toArray();
        for(const auto &layerValue : layerArray){
            QJsonObject layerObject = layerValue.toObject();
            Layer layer;
            layer.name =  layerObject["name"].toString();
            layer.visible = layerObject["visible"].toBool();
            layer.opacity = layerObject["opacity"].toDouble();
            layer.width = width;
            layer.height = height;
            layer.pixels.resize(width*height);
            QString encoded = layerObject["pixels"].toString();
            QByteArray compressed = QByteArray::fromBase64(encoded.toLatin1());
            QByteArray pixelData = qUncompress(compressed);
            int index = 0;
            for(int y = 0; y < height; y++){
                for(int x = 0; x < width; x++){
                    if(index+4 <= pixelData.size()){
                        int r = static_cast<unsigned char>(pixelData[index++]);
                        int g = static_cast<unsigned char>(pixelData[index++]);
                        int b = static_cast<unsigned char>(pixelData[index++]);
                        int a = static_cast<unsigned char>(pixelData[index++]);
                        layer.at(x, y) = QColor(r, g, b, a);
                    }
                }
            }
            frame.layers.push_back(layer);
        }
        loadedFrames.push_back(frame);
    }
    document->loadFrames(loadedFrames);
    document->resizeCanvas(width, height);
    canvas->updateCanvasSize();
    document->buildPalette();
    emit documentUpdated();
}
void FileHandling::pictureToPixel(const QString &path, PictureImportDialog &dialog){

    Layer layer;
    layer.type = LayerType::Pixel;
    layer.name = QFileInfo(path).baseName();
    MedianCut medianCut;
    int targetWidth = dialog.width();
    int targetHeight = dialog.height();
    int paletteSize = dialog.colors();
    QImage image(path);
    if(dialog.keepAspect()){
        image = image.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    else {
        image = image.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    document->resizeCanvas(image.width(), image.height());
    canvas->updateCanvasSize();
    auto palette = medianCut.medianCut(image, paletteSize);
    layer.pixels.resize(document->getCanvasWidth() * document->getCanvasHeight());
    for (int y = 0; y < document->getCanvasHeight(); y++) {
        for (int x = 0; x < document->getCanvasWidth(); x++) {
            QColor original = image.pixelColor(x, y);
            if(original.alpha() == 0) document->activeLayer_().at(x, y) = original;
            else{
                QColor mapped = medianCut.nearestColor(image.pixelColor(x, y), palette);
                document->activeLayer_().at(x, y) = mapped;
            }
        }
    }
    emit documentUpdated();
}
void FileHandling::saveSpriteSheet(const QString &path, int cols, int scale){
    int rows = (document->getFrameSize() + cols - 1) / cols;
    int frameWidth = document->getCanvasWidth() * scale;
    int frameHeight = document->getCanvasHeight()*scale;
    QImage spriteSheet(cols * frameWidth, rows *frameHeight, QImage::Format_ARGB32);
    spriteSheet.fill(Qt::transparent);
    QPainter painter(&spriteSheet);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for(int i = 0; i<document->getFrameSize(); i++){
        QImage frame = document->renderFrame(i);
        if(scale > 1){
            frame = frame.scaled(frameWidth, frameHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        }
        int x = (i % cols) * frameWidth;
        int y = (i / cols) * frameHeight;
        painter.drawImage(x, y, frame);
    }
    painter.end();
    spriteSheet.save(path);
}
void FileHandling::saveGIF(const QString &path, int scale){
    QByteArray filePath = path.toUtf8();
    GifWriter writer = {};
    int imageWidth = document->getCanvasWidth();
    int imageHeight = document->getCanvasHeight();
    int maxScale = qMin(64, 8192/qMax(imageWidth, imageHeight));
    scale = qBound(1,scale,maxScale);
    int outWidth = imageWidth *scale;
    int outHeight = imageHeight *scale;
    if(!GifBegin(&writer, filePath.constData(), outWidth, outHeight, document->getThisFrameDuration(0)/10)){
        qDebug()<< "Gif Begin failed";
        return;
    }
    for(int i = 0; i < document->getFrameSize(); i++){
        qDebug() << "Rendering Frame: " << i;
        QImage image = document->renderFrame(i).convertToFormat(QImage::Format_RGBA8888);
        qDebug() << "Writing Frame: " << i;
        if(!GifWriteFrameScaled(&writer, image.constBits(), imageWidth, imageHeight, scale, document->getThisFrameDuration(i)/10)){
            qDebug() << "Gif write faile on frame" << i;
            GifEnd(&writer);
            return;
        }
        qDebug()<<"Gif write successful. Frame: " << i;
    }
    GifEnd(&writer);
    qDebug() << "finished";
}
void FileHandling::GIFToPixel(const QString &path, PictureImportDialog &dialog){

    QImageReader reader(path);
    if (!reader.supportsAnimation()){
        // mayhe add warning here later
        return;
    }
    int totalFrames = reader.imageCount();
    QVector<Frame> postFrames;
    MedianCut medianCut;

    for(int x=0; x < totalFrames; x++){
        // if(!reader.jumpToImage(x)) break;
        Frame frame;
        Layer layer;
        layer.type = LayerType::Pixel;
        int targetWidth = dialog.width();
        int targetHeight = dialog.height();
        int paletteSize = dialog.colors();
        QImage image = reader.read();
        if(image.isNull()) continue;
        if(dialog.keepAspect()){
            image = image.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
        }
        else {
            image = image.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        }
        layer.width = image.width();
        layer.height = image.height();
        layer.name = QFileInfo(path).baseName();
        document->resizeCanvas(image.width() , image.height());
        canvas->updateCanvasSize();
        auto palette = medianCut.medianCut(image, paletteSize);
        layer.pixels.resize(image.width() * image.height());
        for (int y = 0; y < document->getCanvasHeight(); y++) {
            for (int x = 0; x < document->getCanvasWidth(); x++) {
                QColor original = image.pixelColor(x, y);
                if(original.alpha() == 0) layer.at(x, y) = original;
                else{
                    QColor mapped = medianCut.nearestColor(image.pixelColor(x, y), palette);
                    layer.at(x, y) = mapped;
                }
            }
        }
        frame.layers.push_back(layer);
        postFrames.append(frame);

    }
    document->loadFrames(postFrames);
    document->buildPalette();
    emit documentUpdated();
}
void FileHandling::saveGPL(const QString &fileName){
    QFile file(fileName);
    if(!usedColors.empty()){
        QTextStream out(&file);
        if(!file.open(QIODevice::WriteOnly)){
            return;
        }
        out << "GIMP Palette" << Qt::endl;
        out << "#Palette Name: Custome Palette" << Qt::endl;
        out << "Columns: " << usedColors.size() << Qt::endl;

        for(int x = 0; x < usedColors.size(); x++){
            out << usedColors.at(x).red() << " "
                << usedColors.at(x).green() << " "
                << usedColors.at(x).blue() << " "
                << usedColors.at(x).name() << Qt::endl;
        }
        file.close();
    }
}
void FileHandling::setPalette(QList<QColor> colors){
    usedColors = colors;
}