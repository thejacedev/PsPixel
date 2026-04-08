#include "spritesheetexporter.h"
#include "ui/layers/layermanager.h"
#include "constants.h"
#include <QPainter>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QtMath>

using namespace PixelPaint;

bool SpritesheetExporter::exportSpritesheet(LayerManager *layerManager, QWidget *parent)
{
    if (!layerManager || layerManager->layerCount() == 0) return false;

    // Options dialog
    QDialog dialog(parent);
    dialog.setWindowTitle("Export Spritesheet");
    dialog.setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(SPACING_MD);

    QLabel *title = new QLabel("Spritesheet Options");
    title->setStyleSheet(QString("font-size: %1px; font-weight: bold;").arg(FONT_SIZE_TITLE));
    layout->addWidget(title);

    // Layout type
    QHBoxLayout *layoutRow = new QHBoxLayout();
    layoutRow->addWidget(new QLabel("Layout:"));
    QComboBox *layoutCombo = new QComboBox();
    layoutCombo->addItem("Horizontal Strip");
    layoutCombo->addItem("Vertical Strip");
    layoutCombo->addItem("Grid");
    layoutRow->addWidget(layoutCombo);
    layout->addLayout(layoutRow);

    // Grid columns
    QHBoxLayout *colsRow = new QHBoxLayout();
    QLabel *colsLabel = new QLabel("Columns:");
    colsRow->addWidget(colsLabel);
    QSpinBox *colsSpin = new QSpinBox();
    colsSpin->setRange(1, layerManager->layerCount());
    colsSpin->setValue(qMin(4, layerManager->layerCount()));
    colsSpin->setEnabled(false);
    colsRow->addWidget(colsSpin);
    layout->addLayout(colsRow);

    // Padding
    QHBoxLayout *padRow = new QHBoxLayout();
    padRow->addWidget(new QLabel("Padding:"));
    QSpinBox *padSpin = new QSpinBox();
    padSpin->setRange(0, 32);
    padSpin->setValue(0);
    padSpin->setSuffix(" px");
    padRow->addWidget(padSpin);
    layout->addLayout(padRow);

    // Preview info
    QLabel *infoLabel = new QLabel();
    infoLabel->setStyleSheet(QString("font-size: %1px; color: palette(mid);").arg(FONT_SIZE_CAPTION));
    layout->addWidget(infoLabel);

    // Update info and column visibility
    auto updateInfo = [&]() {
        int frameCount = layerManager->layerCount();
        const QImage &first = layerManager->layerAt(0).image;
        int fw = first.width(), fh = first.height();
        int pad = padSpin->value();
        int cols, rows;

        switch (layoutCombo->currentIndex()) {
            case 0: cols = frameCount; rows = 1; break;
            case 1: cols = 1; rows = frameCount; break;
            default: cols = colsSpin->value(); rows = qCeil((double)frameCount / cols); break;
        }

        int outW = cols * fw + (cols - 1) * pad;
        int outH = rows * fh + (rows - 1) * pad;
        infoLabel->setText(QString("%1 frames, %2x%3 px output").arg(frameCount).arg(outW).arg(outH));
        colsLabel->setEnabled(layoutCombo->currentIndex() == 2);
        colsSpin->setEnabled(layoutCombo->currentIndex() == 2);
    };

    QObject::connect(layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) { updateInfo(); });
    QObject::connect(colsSpin, QOverload<int>::of(&QSpinBox::valueChanged), [&](int) { updateInfo(); });
    QObject::connect(padSpin, QOverload<int>::of(&QSpinBox::valueChanged), [&](int) { updateInfo(); });
    updateInfo();

    // Buttons
    layout->addSpacing(SPACING_MD);
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    QPushButton *cancelBtn = new QPushButton("Cancel");
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btnRow->addWidget(cancelBtn);
    QPushButton *exportBtn = new QPushButton("Export");
    exportBtn->setStyleSheet(
        QString("QPushButton { background-color: %1; color: white; padding: %2px %3px; "
        "border: none; border-radius: %4px; font-weight: bold; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }")
        .arg(ACCENT_HEX).arg(SPACING_XS).arg(SPACING_LG)
        .arg(RADIUS_CONTROL).arg(ACCENT_HOVER_HEX).arg(ACCENT_PRESSED_HEX));
    QObject::connect(exportBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnRow->addWidget(exportBtn);
    layout->addLayout(btnRow);

    if (dialog.exec() != QDialog::Accepted) return false;

    // Build options
    Options options;
    options.layout = static_cast<Layout>(layoutCombo->currentIndex());
    options.gridColumns = colsSpin->value();
    options.padding = padSpin->value();

    QImage sheet = assembleSpritesheet(layerManager, options);
    if (sheet.isNull()) return false;

    QString filePath = QFileDialog::getSaveFileName(parent, "Save Spritesheet",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/spritesheet.png",
        "PNG Files (*.png)");

    if (filePath.isEmpty()) return false;
    return sheet.save(filePath);
}

QImage SpritesheetExporter::assembleSpritesheet(LayerManager *layerManager, const Options &options)
{
    if (!layerManager || layerManager->layerCount() == 0) return QImage();

    int frameCount = layerManager->layerCount();
    const QImage &first = layerManager->layerAt(0).image;
    int fw = first.width(), fh = first.height();
    int pad = options.padding;

    int cols, rows;
    switch (options.layout) {
        case HorizontalStrip: cols = frameCount; rows = 1; break;
        case VerticalStrip:   cols = 1; rows = frameCount; break;
        case Grid:            cols = options.gridColumns; rows = qCeil((double)frameCount / cols); break;
    }

    int outW = cols * fw + (cols - 1) * pad;
    int outH = rows * fh + (rows - 1) * pad;

    QImage result(outW, outH, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter p(&result);
    for (int i = 0; i < frameCount; ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = col * (fw + pad);
        int y = row * (fh + pad);
        p.drawImage(x, y, layerManager->layerAt(i).image);
    }
    p.end();

    return result;
}
