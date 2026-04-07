#include "keybindsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>

using namespace PixelPaint;

static QVector<KeyBinding> defaultBindings()
{
    return {
        {"tool.select",     "Tools",  QKeySequence("V"), QKeySequence("V")},
        {"tool.magicwand", "Tools",  QKeySequence("W"), QKeySequence("W")},
        {"tool.lasso",     "Tools",  QKeySequence("A"), QKeySequence("A")},
        {"tool.brush",      "Tools",  QKeySequence("B"), QKeySequence("B")},
        {"tool.eraser",     "Tools",  QKeySequence("E"), QKeySequence("E")},
        {"tool.eyedropper", "Tools",  QKeySequence("I"), QKeySequence("I")},
        {"tool.fill",       "Tools",  QKeySequence("G"), QKeySequence("G")},
        {"tool.line",       "Tools",  QKeySequence("L"), QKeySequence("L")},
        {"tool.rectangle",  "Tools",  QKeySequence("R"), QKeySequence("R")},
        {"tool.circle",     "Tools",  QKeySequence("O"), QKeySequence("O")},
        {"edit.undo",       "Edit",   QKeySequence::Undo, QKeySequence::Undo},
        {"edit.redo",       "Edit",   QKeySequence::Redo, QKeySequence::Redo},
        {"edit.copy",       "Edit",   QKeySequence::Copy, QKeySequence::Copy},
        {"edit.paste",      "Edit",   QKeySequence::Paste, QKeySequence::Paste},
        {"edit.flipH",      "Edit",   QKeySequence("Ctrl+Shift+H"), QKeySequence("Ctrl+Shift+H")},
        {"edit.flipV",      "Edit",   QKeySequence("Ctrl+Shift+V"), QKeySequence("Ctrl+Shift+V")},
        {"edit.rotateCW",   "Edit",   QKeySequence("Ctrl+Shift+R"), QKeySequence("Ctrl+Shift+R")},
        {"edit.rotateCCW",  "Edit",   QKeySequence("Ctrl+Shift+L"), QKeySequence("Ctrl+Shift+L")},
        {"view.mirrorH",    "View",   QKeySequence("Alt+H"), QKeySequence("Alt+H")},
        {"view.mirrorV",    "View",   QKeySequence("Alt+V"), QKeySequence("Alt+V")},
        {"view.zoomIn",     "View",   QKeySequence::ZoomIn, QKeySequence::ZoomIn},
        {"view.zoomOut",    "View",   QKeySequence::ZoomOut, QKeySequence::ZoomOut},
        {"file.save",       "File",   QKeySequence::Save, QKeySequence::Save},
        {"file.open",       "File",   QKeySequence::Open, QKeySequence::Open},
        {"file.export",     "File",   QKeySequence("Ctrl+E"), QKeySequence("Ctrl+E")},
        {"file.exportLayers","File",  QKeySequence("Ctrl+Shift+E"), QKeySequence("Ctrl+Shift+E")},
    };
}

KeybindsDialog::KeybindsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Keyboard Shortcuts");
    setMinimumSize(500, 450);
    m_bindings = defaultBindings();

    // Load saved bindings
    QMap<QString, QKeySequence> saved = loadBindings();
    for (KeyBinding &b : m_bindings) {
        if (saved.contains(b.action)) {
            b.currentKey = saved[b.action];
        }
    }

    setupUI();
    populateTable();
}

void KeybindsDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("Customize keyboard shortcuts. Double-click a shortcut to change it.");
    title->setWordWrap(true);
    layout->addWidget(title);

    m_table = new QTableWidget();
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"Category", "Action", "Shortcut"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);
    layout->addWidget(m_table, 1);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *resetBtn = new QPushButton("Reset All to Defaults");
    connect(resetBtn, &QPushButton::clicked, this, &KeybindsDialog::onResetAll);
    buttons->addWidget(resetBtn);

    buttons->addStretch();

    QPushButton *cancelBtn = new QPushButton("Cancel");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(cancelBtn);

    QPushButton *applyBtn = new QPushButton("Save");
    applyBtn->setDefault(true);
    connect(applyBtn, &QPushButton::clicked, this, &KeybindsDialog::onApply);
    buttons->addWidget(applyBtn);

    layout->addLayout(buttons);
}

void KeybindsDialog::populateTable()
{
    m_table->setRowCount(m_bindings.size());
    for (int i = 0; i < m_bindings.size(); ++i) {
        const KeyBinding &b = m_bindings[i];

        QTableWidgetItem *catItem = new QTableWidgetItem(b.category);
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 0, catItem);

        // Readable action name
        QString readable = b.action;
        readable.replace("tool.", "").replace("edit.", "").replace("view.", "").replace("file.", "");
        readable[0] = readable[0].toUpper();
        QTableWidgetItem *actionItem = new QTableWidgetItem(readable);
        actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 1, actionItem);

        // Editable shortcut via QKeySequenceEdit
        QKeySequenceEdit *edit = new QKeySequenceEdit(b.currentKey);
        m_table->setCellWidget(i, 2, edit);
    }
}

void KeybindsDialog::onResetAll()
{
    m_bindings = defaultBindings();
    populateTable();
}

void KeybindsDialog::onApply()
{
    // Read back from table
    QMap<QString, QKeySequence> bindings;
    for (int i = 0; i < m_bindings.size(); ++i) {
        QKeySequenceEdit *edit = qobject_cast<QKeySequenceEdit*>(m_table->cellWidget(i, 2));
        if (edit) {
            m_bindings[i].currentKey = edit->keySequence();
            bindings[m_bindings[i].action] = edit->keySequence();
        }
    }
    saveBindings(bindings);
    QMessageBox::information(this, "Shortcuts Saved", "Keyboard shortcuts saved. Restart the app to apply changes.");
    accept();
}

QMap<QString, QKeySequence> KeybindsDialog::loadBindings()
{
    QMap<QString, QKeySequence> result;
    QSettings settings;
    int count = settings.beginReadArray("keybindings");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QString action = settings.value("action").toString();
        QKeySequence key = QKeySequence(settings.value("key").toString());
        result[action] = key;
    }
    settings.endArray();
    return result;
}

void KeybindsDialog::saveBindings(const QMap<QString, QKeySequence> &bindings)
{
    QSettings settings;
    settings.beginWriteArray("keybindings");
    int i = 0;
    for (auto it = bindings.begin(); it != bindings.end(); ++it, ++i) {
        settings.setArrayIndex(i);
        settings.setValue("action", it.key());
        settings.setValue("key", it.value().toString());
    }
    settings.endArray();
}

QKeySequence KeybindsDialog::bindingFor(const QString &action)
{
    QMap<QString, QKeySequence> saved = loadBindings();
    if (saved.contains(action)) return saved[action];

    // Return default
    QVector<KeyBinding> defaults = defaultBindings();
    for (const KeyBinding &b : defaults) {
        if (b.action == action) return b.defaultKey;
    }
    return QKeySequence();
}
