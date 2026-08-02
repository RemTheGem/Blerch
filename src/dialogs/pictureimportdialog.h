#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>

class PictureImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PictureImportDialog(QWidget *parent = nullptr);

    int width() const;
    int height() const;
    int colors() const;

    bool keepAspect() const;
private:
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QSpinBox *colorSpinBox;
    QCheckBox *aspectCheckBox;
};

