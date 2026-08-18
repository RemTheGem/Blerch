#include "onionskindialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>

OnionSkinDialog::OnionSkinDialog(QWidget *parent) : QDialog(parent) {
    previousFramesSpinBox = new QSpinBox(this);
    nextFramesSpinBox = new QSpinBox(this);
    onionOpacitySpinBox = new QDoubleSpinBox(this);
    onionOnCheckBox = new QCheckBox(this);
    previousFramesSpinBox->setValue(1);
    nextFramesSpinBox->setValue(1);
    onionOpacitySpinBox->setValue(0.25f);
    onionOpacitySpinBox->setRange(0.01, 1.0);
    onionOnCheckBox->setCheckState(Qt::Checked);

    auto layout = new QFormLayout(this);
    layout->addRow("Number of Previous Frames:", previousFramesSpinBox);
    layout->addRow("Number of Next Frames:", nextFramesSpinBox);
    layout->addRow("Opacity: ", onionOpacitySpinBox);
    layout->addWidget(onionOnCheckBox);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
int OnionSkinDialog::previousFrames() const{
    return previousFramesSpinBox->value();
}
int OnionSkinDialog::nextFrames() const{
    return nextFramesSpinBox->value();
}
float OnionSkinDialog::onionOpacity() const {
    return onionOpacitySpinBox->value();
}
bool OnionSkinDialog::onionOn() const{
    return onionOnCheckBox->isChecked();
}