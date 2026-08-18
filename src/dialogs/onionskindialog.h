#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QPushButton>

class OnionSkinDialog : public QDialog{
    Q_OBJECT

public:
    explicit OnionSkinDialog(QWidget *parent = nullptr);
    int previousFrames() const;
    int nextFrames() const;
    QColor previousFrameColor() const;
    QColor nextFrameColor() const;
    float onionOpacity() const;
    bool onionOn() const;

private:
    QSpinBox *previousFramesSpinBox;
    QSpinBox *nextFramesSpinBox;
    QDoubleSpinBox *onionOpacitySpinBox;
    QPushButton *nextFrameColorButton;
    QPushButton *previousFrameColorButton;
    QCheckBox *onionOnCheckBox;
    QColor previousFramesColor = Qt::red;
    QColor nextFramesColor = Qt::green;

};
