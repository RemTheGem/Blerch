#include "pictureimportdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>

PictureImportDialog::PictureImportDialog(QWidget *parent)
    : QDialog(parent)
{
    widthSpinBox = new QSpinBox(this);
    heightSpinBox = new QSpinBox(this);
    colorSpinBox = new QSpinBox(this);
    aspectCheckBox = new QCheckBox("Keep Aspect Ratio", this);
    widthSpinBox->setValue(32);
    widthSpinBox->setRange(2,1024);
    heightSpinBox->setValue(32);
    heightSpinBox->setRange(2,1024);
    colorSpinBox->setValue(16);
    colorSpinBox->setRange(0, 256);
    aspectCheckBox->setCheckState(Qt::Checked);

    auto layout = new QFormLayout(this);
    layout->addRow("Width:", widthSpinBox);
    layout->addRow("Height:", heightSpinBox);
    layout->addRow("Colors:", colorSpinBox);
    layout->addWidget(aspectCheckBox);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
int PictureImportDialog::width() const{
    return widthSpinBox->value();
}
int PictureImportDialog::height() const{
    return heightSpinBox->value();
}
int PictureImportDialog::colors() const{
    return colorSpinBox->value();
}
bool PictureImportDialog::keepAspect() const{
    return aspectCheckBox->isChecked();
}