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
class QStackedWidget;
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
    void openNewEntryPage();
    void handleBackFromEditor();
    void viewLatestEntry();
    void viewTodayEntry();
    void viewRandomEntry();
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
    bool saveCurrentEntry(bool returnToHomeAfterSave);
    bool confirmLeaveEditor();
    void showHomePage();
    void showEditorPage();
    void setEditorDirty(bool dirty);
    void applyEditorPrompt(const QString &title, const QString &note, const QString &emotion = QString());
    QString comfortLineForEmotion(const QString &emotion) const;
    QString homeHeadlineForEmotion(const QString &emotion) const;
    void appendTagToEditor(const QString &tag);
    void refreshCalendarReview();

    QList<EmotionEntry> m_entries;
    JournalRepository m_repository;
    QString m_selectedEntryId;
    QString m_editingEntryId;
    bool m_editorDirty = false;
    bool m_ignoreEditorChanges = false;

    QLineEdit *m_titleEdit = nullptr;
    QComboBox *m_emotionCombo = nullptr;
    QComboBox *m_tagFilterCombo = nullptr;
    QDateEdit *m_dateEdit = nullptr;
    QSlider *m_intensitySlider = nullptr;
    QSlider *m_energySlider = nullptr;
    QPlainTextEdit *m_noteEdit = nullptr;
    QLineEdit *m_tagEdit = nullptr;
    QListWidget *m_entryList = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_statMoodValue = nullptr;
    QLabel *m_statMoodLabel = nullptr;
    QLabel *m_statStreakValue = nullptr;
    QLabel *m_statStreakLabel = nullptr;
    QLabel *m_statUpdateValue = nullptr;
    QLabel *m_statUpdateLabel = nullptr;
    QLabel *m_homeHeadlineLabel = nullptr;
    QLabel *m_homeSubheadlineLabel = nullptr;
    QLabel *m_companionTitleLabel = nullptr;
    QLabel *m_companionBodyLabel = nullptr;
    QLabel *m_calendarMonthLabel = nullptr;
    QLabel *m_calendarSummaryLabel = nullptr;
    QLabel *m_tagTrendLabel = nullptr;
    QLabel *m_filterHintLabel = nullptr;
    QLabel *m_weekTrendLabel = nullptr;
    QLabel *m_monthTrendLabel = nullptr;
    QList<QLabel *> m_calendarCells;
    QLabel *m_detailTitle = nullptr;
    QLabel *m_detailMeta = nullptr;
    QLabel *m_detailIcon = nullptr;
    QPlainTextEdit *m_detailNote = nullptr;
    StarMapView *m_starMapView = nullptr;
    QStackedWidget *m_leftStack = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_openEditorButton = nullptr;
    QPushButton *m_backButton = nullptr;
    QPushButton *m_viewLatestButton = nullptr;
    QPushButton *m_viewTodayButton = nullptr;
    QPushButton *m_viewRandomButton = nullptr;
    QString m_activeTagFilter;
};
