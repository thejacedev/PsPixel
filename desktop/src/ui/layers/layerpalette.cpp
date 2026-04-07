#include "layerpalette.h"
#include "layermanager.h"
#include "pixelcanvas.h"
#include "constants.h"
#include <QInputDialog>
#include <QApplication>

using namespace PixelPaint;

LayerPalette::LayerPalette(LayerManager *layerManager, PixelCanvas *canvas, QWidget *parent)
    : QDockWidget("Layers", parent)
    , m_layerManager(layerManager)
    , m_canvas(canvas)
{
    setupUI();

    if (m_layerManager) {
        connect(m_layerManager, &LayerManager::layersChanged, this, &LayerPalette::refreshList);
        connect(m_layerManager, &LayerManager::activeLayerChanged, this, [this](int) {
            refreshList();
        });
    }

    if (m_canvas) {
        connect(m_canvas, &PixelCanvas::referenceImageChanged, this, &LayerPalette::refreshList);
    }

    refreshList();
}

void LayerPalette::setupUI()
{
    setObjectName("LayerPalette");
    setMinimumWidth(150);
    setMaximumWidth(500);
    setMinimumHeight(200);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    QWidget *content = new QWidget();
    setWidget(content);

    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(SPACING_SM, SPACING_SM, SPACING_SM, SPACING_SM);
    layout->setSpacing(SPACING_SM);

    // Opacity slider
    QHBoxLayout *opacityLayout = new QHBoxLayout();
    QLabel *opacityLabel = new QLabel("Opacity:");
    opacityLabel->setStyleSheet(QString("font-size: %1px;").arg(FONT_SIZE_BODY));
    opacityLayout->addWidget(opacityLabel);
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &LayerPalette::onOpacityChanged);
    opacityLayout->addWidget(m_opacitySlider);
    m_opacityLabel = new QLabel("100%");
    m_opacityLabel->setFixedWidth(40);
    m_opacityLabel->setStyleSheet(QString("font-size: %1px;").arg(FONT_SIZE_BODY));
    opacityLayout->addWidget(m_opacityLabel);
    layout->addLayout(opacityLayout);

    // Layer list (top = front, bottom = back)
    m_layerList = new QListWidget();
    m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_layerList->setAlternatingRowColors(true);
    m_layerList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_layerList->setStyleSheet(
        QString("QListWidget {"
        "    background-color: palette(base);"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    selection-background-color: palette(highlight);"
        "    selection-color: palette(highlighted-text);"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 4px %2px;"
        "    border-bottom: 1px solid palette(midlight);"
        "    min-height: 20px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: palette(light);"
        "}"
        "QListWidget::item:selected {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "}").arg(RADIUS_CONTROL).arg(SPACING_SM)
    );
    connect(m_layerList, &QListWidget::currentRowChanged, this, &LayerPalette::onLayerClicked);
    connect(m_layerList, &QListWidget::itemChanged, this, &LayerPalette::onVisibilityToggled);
    layout->addWidget(m_layerList, 1);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(SPACING_XS);

    QString layerButtonStyle = QString(
        "QPushButton {"
        "    background-color: palette(button);"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    padding: 2px;"
        "    font-size: %2px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(light);"
        "    border: 1px solid palette(highlight);"
        "}"
        "QPushButton:pressed {"
        "    background-color: palette(dark);"
        "}").arg(RADIUS_CONTROL).arg(FONT_SIZE_BODY);

    m_addButton = new QPushButton("+");
    m_addButton->setToolTip("Add Layer");
    m_addButton->setMaximumWidth(30);
    m_addButton->setStyleSheet(layerButtonStyle);
    connect(m_addButton, &QPushButton::clicked, this, &LayerPalette::onAddLayer);
    buttonLayout->addWidget(m_addButton);

    m_removeButton = new QPushButton("-");
    m_removeButton->setToolTip("Remove Layer");
    m_removeButton->setMaximumWidth(30);
    m_removeButton->setStyleSheet(layerButtonStyle);
    connect(m_removeButton, &QPushButton::clicked, this, &LayerPalette::onRemoveLayer);
    buttonLayout->addWidget(m_removeButton);

    m_duplicateButton = new QPushButton("D");
    m_duplicateButton->setToolTip("Duplicate Layer");
    m_duplicateButton->setMaximumWidth(30);
    m_duplicateButton->setStyleSheet(layerButtonStyle);
    connect(m_duplicateButton, &QPushButton::clicked, this, &LayerPalette::onDuplicateLayer);
    buttonLayout->addWidget(m_duplicateButton);

    m_upButton = new QPushButton("^");
    m_upButton->setToolTip("Move Layer Up");
    m_upButton->setMaximumWidth(30);
    m_upButton->setStyleSheet(layerButtonStyle);
    connect(m_upButton, &QPushButton::clicked, this, &LayerPalette::onMoveUp);
    buttonLayout->addWidget(m_upButton);

    m_downButton = new QPushButton("v");
    m_downButton->setToolTip("Move Layer Down");
    m_downButton->setMaximumWidth(30);
    m_downButton->setStyleSheet(layerButtonStyle);
    connect(m_downButton, &QPushButton::clicked, this, &LayerPalette::onMoveDown);
    buttonLayout->addWidget(m_downButton);

    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
}

void LayerPalette::refreshList()
{
    if (!m_layerManager) return;

    m_layerList->blockSignals(true);
    m_layerList->clear();

    bool refActive = m_canvas && m_canvas->referenceActive();

    // Reference image entry at the top (if loaded)
    if (m_canvas && m_canvas->hasReferenceImage()) {
        QListWidgetItem *refItem = new QListWidgetItem();
        refItem->setText("Reference");
        refItem->setFlags(refItem->flags() | Qt::ItemIsUserCheckable);
        refItem->setCheckState(Qt::Checked);
        refItem->setData(Qt::UserRole, -1); // special index
        refItem->setForeground(QBrush(ACCENT_COLOR));
        QFont f = refItem->font();
        f.setItalic(true);
        if (refActive) {
            refItem->setBackground(QBrush(ACCENT_MEDIUM));
            f.setBold(true);
        }
        refItem->setFont(f);
        m_layerList->addItem(refItem);
    }

    // Show layers top-to-bottom (highest index = frontmost = top of list)
    for (int i = m_layerManager->layerCount() - 1; i >= 0; --i) {
        const Layer &layer = m_layerManager->layerAt(i);
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(layer.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(layer.visible ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, i);

        if (!refActive && i == m_layerManager->activeLayerIndex()) {
            item->setBackground(QBrush(ACCENT_STRONG));
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }

        m_layerList->addItem(item);
    }

    // Select the right row
    if (refActive) {
        m_layerList->setCurrentRow(0);
        m_opacitySlider->blockSignals(true);
        m_opacitySlider->setValue(static_cast<int>(m_canvas->referenceOpacity() * 100));
        m_opacitySlider->blockSignals(false);
        m_opacityLabel->setText(QString("%1%").arg(static_cast<int>(m_canvas->referenceOpacity() * 100)));
    } else {
        int refOffset = (m_canvas && m_canvas->hasReferenceImage()) ? 1 : 0;
        int activeRow = refOffset + m_layerManager->layerCount() - 1 - m_layerManager->activeLayerIndex();
        m_layerList->setCurrentRow(activeRow);

        const Layer &active = m_layerManager->activeLayer();
        m_opacitySlider->blockSignals(true);
        m_opacitySlider->setValue(static_cast<int>(active.opacity * 100));
        m_opacitySlider->blockSignals(false);
        m_opacityLabel->setText(QString("%1%").arg(static_cast<int>(active.opacity * 100)));
    }

    m_layerList->blockSignals(false);
}

void LayerPalette::onAddLayer()
{
    if (!m_layerManager) return;

    bool ok;
    QString name = QInputDialog::getText(this, "New Layer", "Layer name:",
                                         QLineEdit::Normal,
                                         QString("Layer %1").arg(m_layerManager->layerCount() + 1), &ok);
    if (ok && !name.isEmpty()) {
        const Layer &current = m_layerManager->activeLayer();
        int idx = m_layerManager->addLayer(name, current.image.width(), current.image.height());
        m_layerManager->setActiveLayer(idx);
    }
}

void LayerPalette::onRemoveLayer()
{
    if (!m_layerManager || m_layerManager->layerCount() <= 1) return;
    m_layerManager->removeLayer(m_layerManager->activeLayerIndex());
}

void LayerPalette::onMoveUp()
{
    if (!m_layerManager) return;
    int idx = m_layerManager->activeLayerIndex();
    if (idx < m_layerManager->layerCount() - 1) {
        m_layerManager->moveLayer(idx, idx + 1);
    }
}

void LayerPalette::onMoveDown()
{
    if (!m_layerManager) return;
    int idx = m_layerManager->activeLayerIndex();
    if (idx > 0) {
        m_layerManager->moveLayer(idx, idx - 1);
    }
}

void LayerPalette::onDuplicateLayer()
{
    if (!m_layerManager) return;
    m_layerManager->duplicateLayer(m_layerManager->activeLayerIndex());
}

void LayerPalette::onLayerClicked(int row)
{
    // Guard against signals during refreshList rebuild
    if (!m_layerManager || row < 0 || row >= m_layerList->count()) return;

    QListWidgetItem *item = m_layerList->item(row);
    if (!item) return;

    int layerIndex = item->data(Qt::UserRole).toInt();

    if (layerIndex == -1) {
        // Reference layer selected — enable ref mode
        if (m_canvas) m_canvas->setReferenceActive(true);
        refreshList();
    } else {
        // Normal layer selected — disable ref mode, switch layer
        if (m_canvas) m_canvas->setReferenceActive(false);
        m_layerManager->setActiveLayer(layerIndex);
        // refreshList is triggered by activeLayerChanged signal
    }
}

void LayerPalette::onOpacityChanged(int value)
{
    if (!m_layerManager) return;

    if (m_canvas && m_canvas->referenceActive()) {
        m_canvas->setReferenceOpacity(value / 100.0);
    } else {
        m_layerManager->setLayerOpacity(m_layerManager->activeLayerIndex(), value / 100.0);
    }
    m_opacityLabel->setText(QString("%1%").arg(value));
}

void LayerPalette::onVisibilityToggled(QListWidgetItem *item)
{
    if (!m_layerManager || !item) return;
    int layerIndex = item->data(Qt::UserRole).toInt();

    if (layerIndex == -1 && m_canvas) {
        // Toggle reference visibility via opacity
        if (item->checkState() == Qt::Checked) {
            m_canvas->setReferenceOpacity(1.0);
        } else {
            m_canvas->setReferenceOpacity(0.0);
        }
        return;
    }

    m_layerManager->setLayerVisible(layerIndex, item->checkState() == Qt::Checked);
}
