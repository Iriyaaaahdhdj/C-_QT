#include "mainwindow.h"

#include <algorithm>

#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QUuid>

#include "starmapview.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_repository(dataFilePath()) {
    buildUi();
    loadEntries();
    refreshViews();
}

void MainWindow::buildUi() {
    setWindowTitle("Emotion Star Journal");
    resize(1380, 820);

    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(16);

    auto *leftPanel = new QWidget(central);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(12);

    auto *title = new QLabel("Emotion Star Journal", leftPanel);
    title->setStyleSheet("font-size: 28px; font-weight: 700; color: white;");
    auto *subtitle = new QLabel("Turn each day into a star and watch your emotional sky evolve.", leftPanel);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet("font-size: 14px; color: #c8d5f0;");

    auto *formBox = new QGroupBox("New Memory", leftPanel);
    auto *formLayout = new QVBoxLayout(formBox);

    m_titleEdit = new QLineEdit(formBox);
    m_titleEdit->setPlaceholderText("A short title for the day");

    m_emotionCombo = new QComboBox(formBox);
    m_emotionCombo->addItems({"Joy", "Calm", "Sadness", "Anger", "Anxiety", "Hope"});

    m_dateEdit = new QDateEdit(QDate::currentDate(), formBox);
    m_dateEdit->setCalendarPopup(true);

    m_intensitySlider = new QSlider(Qt::Horizontal, formBox);
    m_intensitySlider->setRange(1, 5);
    m_intensitySlider->setValue(3);

    m_energySlider = new QSlider(Qt::Horizontal, formBox);
    m_energySlider->setRange(1, 5);
    m_energySlider->setValue(3);

    m_noteEdit = new QPlainTextEdit(formBox);
    m_noteEdit->setPlaceholderText("What happened today? What do you want future-you to remember?");
    m_noteEdit->setMinimumHeight(120);

    auto *saveButton = new QPushButton("Save As A Star", formBox);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::addEntry);

    formLayout->addWidget(new QLabel("Title", formBox));
    formLayout->addWidget(m_titleEdit);
    formLayout->addWidget(new QLabel("Emotion", formBox));
    formLayout->addWidget(m_emotionCombo);
    formLayout->addWidget(new QLabel("Date", formBox));
    formLayout->addWidget(m_dateEdit);
    formLayout->addWidget(new QLabel("Intensity", formBox));
    formLayout->addWidget(m_intensitySlider);
    formLayout->addWidget(new QLabel("Energy", formBox));
    formLayout->addWidget(m_energySlider);
    formLayout->addWidget(new QLabel("Note", formBox));
    formLayout->addWidget(m_noteEdit);
    formLayout->addWidget(saveButton);

    auto *listBox = new QGroupBox("Star Archive", leftPanel);
    auto *listLayout = new QVBoxLayout(listBox);
    m_summaryLabel = new QLabel(listBox);
    m_entryList = new QListWidget(listBox);
    connect(m_entryList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *, QListWidgetItem *) {
        handleListSelectionChanged();
    });
    listLayout->addWidget(m_summaryLabel);
    listLayout->addWidget(m_entryList);

    leftLayout->addWidget(title);
    leftLayout->addWidget(subtitle);
    leftLayout->addWidget(formBox, 1);
    leftLayout->addWidget(listBox, 1);

    auto *rightPanel = new QWidget(central);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(12);

    m_starMapView = new StarMapView(rightPanel);
    connect(m_starMapView, &StarMapView::entrySelected, this, &MainWindow::handleStarSelection);

    auto *detailBox = new QGroupBox("Selected Star", rightPanel);
    auto *detailLayout = new QVBoxLayout(detailBox);

    m_detailIcon = new QLabel(detailBox);
    m_detailTitle = new QLabel("No entry selected", detailBox);
    m_detailMeta = new QLabel("Pick a star from the map or the archive list.", detailBox);
    m_detailNote = new QPlainTextEdit(detailBox);
    m_detailNote->setReadOnly(true);
    m_detailNote->setMinimumHeight(140);

    m_detailIcon->setStyleSheet("font-size: 26px; font-weight: 700; color: #ffd166;");
    m_detailTitle->setStyleSheet("font-size: 20px; font-weight: 700; color: white;");
    m_detailMeta->setStyleSheet("font-size: 13px; color: #c8d5f0;");

    detailLayout->addWidget(m_detailIcon);
    detailLayout->addWidget(m_detailTitle);
    detailLayout->addWidget(m_detailMeta);
    detailLayout->addWidget(m_detailNote);

    rightLayout->addWidget(m_starMapView, 3);
    rightLayout->addWidget(detailBox, 1);

    rootLayout->addWidget(leftPanel, 2);
    rootLayout->addWidget(rightPanel, 3);
    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow, QWidget { background: #09121f; color: #edf2ff; }
        QGroupBox {
            border: 1px solid #23324d;
            border-radius: 16px;
            margin-top: 10px;
            font-weight: 700;
            padding-top: 12px;
            background: #101b2d;
        }
        QGroupBox::title { left: 16px; padding: 0 4px; color: #f2f6ff; }
        QLineEdit, QDateEdit, QComboBox, QPlainTextEdit, QListWidget {
            background: #152338;
            border: 1px solid #2c3f62;
            border-radius: 10px;
            padding: 8px;
            color: #f3f7ff;
        }
        QPushButton {
            background: #f6c445;
            color: #09121f;
            border: none;
            border-radius: 12px;
            padding: 10px 14px;
            font-weight: 700;
        }
        QPushButton:hover { background: #ffd166; }
        QListWidget::item { padding: 8px; }
        QListWidget::item:selected { background: #23395d; border-radius: 8px; }
    )");
}

void MainWindow::loadEntries() {
    m_entries = m_repository.load();
}

void MainWindow::refreshViews() {
    std::sort(m_entries.begin(), m_entries.end(), [](const EmotionEntry &a, const EmotionEntry &b) {
        return a.date > b.date;
    });

    m_entryList->clear();
    for (const auto &entry : m_entries) {
        auto *item = new QListWidgetItem(
            QString("%1  |  %2  |  %3").arg(entry.date.toString("MM-dd"), entry.emotion, entry.title),
            m_entryList);
        item->setData(Qt::UserRole, entry.id);
        item->setForeground(EmotionEntry::colorForEmotion(entry.emotion));
    }

    int joyCount = 0;
    int intenseCount = 0;
    for (const auto &entry : m_entries) {
        if (entry.emotion == "Joy" || entry.emotion == "Hope") {
            ++joyCount;
        }
        if (entry.intensity >= 4) {
            ++intenseCount;
        }
    }

    m_summaryLabel->setText(
        QString("Stars: %1    Bright memories: %2    Warm emotions: %3")
            .arg(m_entries.size())
            .arg(intenseCount)
            .arg(joyCount));

    m_starMapView->setEntries(m_entries);
    if (!m_entries.isEmpty() && m_entryList->currentRow() < 0) {
        m_entryList->setCurrentRow(0);
    } else if (m_entries.isEmpty()) {
        showEntryDetails(nullptr);
    }
}

void MainWindow::selectEntryById(const QString &id) {
    for (int row = 0; row < m_entryList->count(); ++row) {
        auto *item = m_entryList->item(row);
        if (item->data(Qt::UserRole).toString() == id) {
            m_entryList->setCurrentRow(row);
            return;
        }
    }
}

void MainWindow::showEntryDetails(const EmotionEntry *entry) {
    if (!entry) {
        m_detailIcon->setText("...");
        m_detailTitle->setText("No entry selected");
        m_detailMeta->setText("Pick a star from the map or the archive list.");
        m_detailNote->setPlainText("");
        return;
    }

    m_detailIcon->setText(EmotionEntry::iconForEmotion(entry->emotion));
    m_detailTitle->setText(entry->title);
    m_detailMeta->setText(
        QString("%1  |  %2  |  Intensity %3  |  Energy %4")
            .arg(entry->date.toString("yyyy-MM-dd"), entry->emotion)
            .arg(entry->intensity)
            .arg(entry->energy));
    m_detailNote->setPlainText(entry->note);
}

EmotionEntry MainWindow::currentFormEntry() const {
    EmotionEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.title = m_titleEdit->text().trimmed();
    entry.emotion = m_emotionCombo->currentText();
    entry.note = m_noteEdit->toPlainText().trimmed();
    entry.date = m_dateEdit->date();
    entry.intensity = m_intensitySlider->value();
    entry.energy = m_energySlider->value();
    return entry;
}

QString MainWindow::dataFilePath() const {
    const auto baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(baseDir).filePath("entries.json");
}

void MainWindow::addEntry() {
    const auto entry = currentFormEntry();
    if (entry.title.isEmpty() || entry.note.isEmpty()) {
        m_summaryLabel->setText("Please write both a title and a note before saving.");
        return;
    }

    m_entries.prepend(entry);
    m_repository.save(m_entries);
    refreshViews();
    selectEntryById(entry.id);

    m_titleEdit->clear();
    m_noteEdit->clear();
    m_intensitySlider->setValue(3);
    m_energySlider->setValue(3);
    m_emotionCombo->setCurrentIndex(0);
    m_dateEdit->setDate(QDate::currentDate());
}

void MainWindow::handleListSelectionChanged() {
    auto *item = m_entryList->currentItem();
    if (!item) {
        showEntryDetails(nullptr);
        return;
    }

    const auto id = item->data(Qt::UserRole).toString();
    for (const auto &entry : m_entries) {
        if (entry.id == id) {
            showEntryDetails(&entry);
            return;
        }
    }
}

void MainWindow::handleStarSelection(const QString &id) {
    selectEntryById(id);
}
