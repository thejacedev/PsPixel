#ifndef LAYERPALETTE_H
#define LAYERPALETTE_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

namespace PixelPaint {

class LayerManager;

class LayerPalette : public QDockWidget
{
    Q_OBJECT

public:
    explicit LayerPalette(LayerManager *layerManager, QWidget *parent = nullptr);

    void refreshList();

private slots:
    void onAddLayer();
    void onRemoveLayer();
    void onMoveUp();
    void onMoveDown();
    void onDuplicateLayer();
    void onLayerClicked(int row);
    void onOpacityChanged(int value);
    void onVisibilityToggled(QListWidgetItem *item);

private:
    void setupUI();

    LayerManager *m_layerManager;
    QListWidget *m_layerList;
    QSlider *m_opacitySlider;
    QLabel *m_opacityLabel;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QPushButton *m_duplicateButton;
};

} // namespace PixelPaint

#endif // LAYERPALETTE_H
