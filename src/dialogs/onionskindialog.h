#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

class OnionSkinDialog : public QDialog{
    Q_OBJECT

public:
    explicit OnionSkinDialog(QWidget *parent = nullptr);
    int previousFrames() const;
    int nextFrames() const;
    float onionOpacity() const;
    bool onionOn() const;

private:
    QSpinBox *previousFramesSpinBox;
    QSpinBox *nextFramesSpinBox;
    QDoubleSpinBox *onionOpacitySpinBox;
    QCheckBox *onionOnCheckBox;

};
