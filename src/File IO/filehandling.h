#ifndef FILEHANDLING_H
#define FILEHANDLING_H
#include "../model/canvasdocument.h"
#include "../PixelCanvas.h"
#include "../dialogs/pictureimportdialog.h"

#include <QString>
#include <QJsonObject>

class FileHandling : public QObject
{
    Q_OBJECT
public:
    FileHandling(CanvasDocument *document, PixelCanvas *canvas);
    void saveImage(const QString &path); // save image as png
    void saveProject(const QString &path); // save the project as a json file
    void loadFromJson(QJsonObject obj); // load project from a json file
    void loadPicture(const QString &path); // load a picture on a separate layer for reference
    void saveSpriteSheet(const QString &path, int columns, int scale);
    void saveGIF(const QString &path, int scale);
    void pictureToPixel(const QString &path, PictureImportDialog &dialog); // turn imported picture into pixel art
    void GIFToPixel(const QString &path, PictureImportDialog &dialog);
    void setPalette(QList<QColor> colors);
    void saveGPL(const QString &fileName);
    QString recoveryDirectory() const;
private:
    CanvasDocument *document;
    PixelCanvas *canvas;
    QList<QColor> usedColors;
signals:
    void documentUpdated();
};

#endif // FILEHANDLING_H
