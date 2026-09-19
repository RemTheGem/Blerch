#include "filehandling.h"
#include "../tools/mediancut.h"
#include <QPainter>
#include <QJsonArray>
#include <QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QImageReader>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QDir>

FileHandling::FileHandling(CanvasDocument *document, PixelCanvas *canvas) : document(document), canvas(canvas) {
    connect(document, &CanvasDocument::paletteUpdated, this, &FileHandling::setPalette);
}

QString FileHandling::recoveryDirectory() const{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recovery";
    QDir().mkpath(path);
    return path;
}

void FileHandling::saveImage(const QString &path, int scale)
{
    QImage image(document->currentFrame_().layers[0].width * canvas->getZoom(), document->currentFrame_().layers[0].height * canvas->getZoom(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QImage rendered = document->renderFrame(document->getCurrentFrame());
    rendered = rendered.scaled(rendered.size() * scale, Qt::KeepAspectRatio, Qt::FastTransformation);
    painter.drawImage(0, 0, rendered);
    painter.end();
    rendered.save(path);

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
    layer.id = document->giveLayerId();
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
    for(const auto &frameValue : std::as_const(frameArray)){
        QJsonObject frameObject = frameValue.toObject();
        Frame frame;
        frame.duration = frameObject["duration"].toInt(100);
        QJsonArray layerArray = frameObject["layers"].toArray();
        for(const auto &layerValue : std::as_const(layerArray)){
            QJsonObject layerObject = layerValue.toObject();
            Layer layer;
            layer.name =  layerObject["name"].toString();
            layer.visible = layerObject["visible"].toBool();
            layer.opacity = layerObject["opacity"].toDouble();
            layer.width = width;
            layer.height = height;
            layer.id = document->giveLayerId();
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
    document->addLayer();
    document->activeLayer_().name = QFileInfo(path).baseName();
    auto palette = medianCut.medianCut(image, paletteSize);
    document->activeLayer_().pixels.resize(document->getCanvasWidth() * document->getCanvasHeight());
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