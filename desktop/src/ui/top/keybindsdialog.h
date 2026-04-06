#ifndef KEYBINDSDIALOG_H
#define KEYBINDSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QMap>
#include <QSettings>

namespace PixelPaint {

struct KeyBinding {
    QString action;
    QString category;
    QKeySequence defaultKey;
    QKeySequence currentKey;
};

class KeybindsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeybindsDialog(QWidget *parent = nullptr);

    static QMap<QString, QKeySequence> loadBindings();
    static void saveBindings(const QMap<QString, QKeySequence> &bindings);
    static QKeySequence bindingFor(const QString &action);

private slots:
    void onResetAll();
    void onApply();

private:
    void setupUI();
    void populateTable();

    QTableWidget *m_table;
    QVector<KeyBinding> m_bindings;
};

} // namespace PixelPaint

#endif // KEYBINDSDIALOG_H
