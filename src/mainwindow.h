#pragma once

#include <QList>
#include <QMainWindow>

#include "emotionentry.h"
#include "journalrepository.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class StarMapView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addEntry();
    void editSelectedEntry();
    void deleteSelectedEntry();
    void clearEditor();
    void handleListSelectionChanged();
    void handleStarSelection(const QString &id);

private:
    void buildUi();
    void loadEntries();
    void refreshViews();
    void selectEntryById(const QString &id);
    void showEntryDetails(const EmotionEntry *entry);
    void populateEditor(const EmotionEntry &entry);
    EmotionEntry *findEntryById(const QString &id);
    const EmotionEntry *findEntryById(const QString &id) const;
    QList<EmotionEntry> demoEntries() const;
    EmotionEntry currentFormEntry() const;
    QString dataFilePath() const;

    QList<EmotionEntry> m_entries;
    JournalRepository m_repository;
    QString m_selectedEntryId;
    QString m_editingEntryId;

    QLineEdit *m_titleEdit = nullptr;
    QComboBox *m_emotionCombo = nullptr;
    QDateEdit *m_dateEdit = nullptr;
    QSlider *m_intensitySlider = nullptr;
    QSlider *m_energySlider = nullptr;
    QPlainTextEdit *m_noteEdit = nullptr;
    QListWidget *m_entryList = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_detailTitle = nullptr;
    QLabel *m_detailMeta = nullptr;
    QLabel *m_detailIcon = nullptr;
    QPlainTextEdit *m_detailNote = nullptr;
    StarMapView *m_starMapView = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_clearButton = nullptr;
};
