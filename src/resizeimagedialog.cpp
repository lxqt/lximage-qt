/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 * 
 * Resize feature inspired by Gwenview's one
 * Copyright 2010 Aurélien Gâteau <agateau@kde.org>
 * adjam refactored
 * Copyright 2020 Andrea Diamantini <adjam@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "resizeimagedialog.h"
#include <QDialogButtonBox>
#include <QPushButton>

using namespace LxImage;


ResizeImageDialog::ResizeImageDialog(QWidget* parent):
  QDialog(parent) {

  updateFromRatio_ = false;
  updateFromSizeOrPercentage_ = false;

  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->setSizeConstraint(QLayout::SetFixedSize);

  QWidget* content = new QWidget(this);
  ui.setupUi(content);
  mainLayout->addWidget(content);
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ResizeImageDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ResizeImageDialog::reject);
  mainLayout->addWidget(buttonBox);

  content->layout()->setContentsMargins(0, 0, 0, 0);

  connect(ui.mWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeImageDialog::slotWidthChanged);
  connect(ui.mHeightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeImageDialog::slotHeightChanged);
  connect(ui.mWidthPercentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeImageDialog::slotWidthPercentChanged);
  connect(ui.mHeightPercentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeImageDialog::slotHeightPercentChanged);
  connect(ui.mKeepAspectCheckBox, &QCheckBox::toggled, this, &ResizeImageDialog::slotKeepAspectChanged);
}

ResizeImageDialog::~ResizeImageDialog() {

}

void ResizeImageDialog::setOriginalSize(const QSize& size)
{
  originalSize_ = size;
  ui.mOriginalWidthLabel->setText(QString::number(size.width()) + QStringLiteral(" px"));
  ui.mOriginalHeightLabel->setText(QString::number(size.height()) + QStringLiteral(" px"));
  ui.mWidthSpinBox->setValue(size.width());
  ui.mHeightSpinBox->setValue(size.height());
}

QSize ResizeImageDialog::size() const
{
  return QSize (
    ui.mWidthSpinBox->value(),
    ui.mHeightSpinBox->value()
  );
}

void ResizeImageDialog::slotWidthChanged(int width)
{
  // Update width percentage to match width, only if this was a manual adjustment
  if (!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.mWidthPercentSpinBox->setValue((double(width) / originalSize_.width()) * 100);
    updateFromSizeOrPercentage_ = false;
  }

  if (!ui.mKeepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Update height to keep ratio, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.mHeightSpinBox->setValue(originalSize_.height() * width / originalSize_.width());
  updateFromRatio_ = false;
}

void ResizeImageDialog::slotHeightChanged(int height)
{
  // Update height percentage to match height, only if this was a manual adjustment
  if (!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.mHeightPercentSpinBox->setValue((double(height) / originalSize_.height()) * 100);
    updateFromSizeOrPercentage_ = false;
  }

  if (!ui.mKeepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Update width to keep ratio, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.mWidthSpinBox->setValue(originalSize_.width() * height / originalSize_.height());
  updateFromRatio_ = false;
}

void ResizeImageDialog::slotWidthPercentChanged(double widthPercent)
{
  // Update width to match width percentage, only if this was a manual adjustment
  if (!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.mWidthSpinBox->setValue((widthPercent / 100) * originalSize_.width());
    updateFromSizeOrPercentage_ = false;
  }

  if (!ui.mKeepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Keep height percentage in sync with width percentage, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.mHeightPercentSpinBox->setValue(ui.mWidthPercentSpinBox->value());
  updateFromRatio_ = false;
}

void ResizeImageDialog::slotHeightPercentChanged(double heightPercent)
{
  // Update height to match height percentage, only if this was a manual adjustment
  if (!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.mHeightSpinBox->setValue((heightPercent / 100) * originalSize_.height());
    updateFromSizeOrPercentage_ = false;
  }

  if (!ui.mKeepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Keep height percentage in sync with width percentage, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.mWidthPercentSpinBox->setValue(ui.mHeightPercentSpinBox->value());
  updateFromRatio_ = false;
}

void ResizeImageDialog::slotKeepAspectChanged(bool value)
{
  if (value) {
    updateFromSizeOrPercentage_ = true;
    slotWidthChanged(ui.mWidthSpinBox->value());
    slotWidthPercentChanged(ui.mWidthPercentSpinBox->value());
    updateFromSizeOrPercentage_ = false;
  }
}
