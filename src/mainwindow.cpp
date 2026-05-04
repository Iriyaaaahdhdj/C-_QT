#include "mainwindow.h"

#include <algorithm>

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QUuid>

#include "starmapview.h"

namespace {
QFrame *createStatCard(const QString &value, const QString &label, QWidget *parent,
                       QLabel **valueOut = nullptr, QLabel **labelOut = nullptr) {
    auto *card = new QFrame(parent);
    card->setObjectName("statCard");

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(4);

    auto *valueLabel = new QLabel(value, card);
    valueLabel->setObjectName("statNumber");
    auto *textLabel = new QLabel(label, card);
    textLabel->setObjectName("statText");
    textLabel->setWordWrap(true);

    if (valueOut) {
        *valueOut = valueLabel;
    }
    if (labelOut) {
        *labelOut = textLabel;
    }

    layout->addWidget(valueLabel);
    layout->addWidget(textLabel);
    return card;
}

QPushButton *createPromptButton(const QString &title, const QString &subtitle, QWidget *parent) {
    auto *button = new QPushButton(parent);
    button->setObjectName("promptAction");
    button->setText(title + "\n" + subtitle);
    button->setMinimumHeight(72);
    return button;
}

QStringList normalizeTags(const QString &raw) {
    QString text = raw;
    text.replace("，", ",");
    text.replace("、", ",");
    text.replace("；", ",");
    text.replace(";", ",");
    text.replace("/", ",");
    text.replace("\n", ",");

    QStringList result;
    const QStringList parts = text.split(QRegularExpression("\\s*,\\s*"), Qt::SkipEmptyParts);
    for (const auto &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

QString tagsToText(const QStringList &tags) {
    return tags.join("，");
}

QString tagBubbleText(const QStringList &tags) {
    if (tags.isEmpty()) {
        return "还没有标签";
    }

    QStringList decorated;
    for (const auto &tag : tags) {
        decorated.append("#" + tag);
    }
    return decorated.join("  ");
}

QString elementNameForEmotion(const QString &emotion) {
    const QString value = EmotionEntry::normalizedEmotion(emotion);
    if (value == "快乐") return "火";
    if (value == "平静") return "水";
    if (value == "难过") return "金";
    if (value == "愤怒") return "木";
    if (value == "焦虑") return "土";
    return "木";
}

QString trendReadingForEntry(const EmotionEntry &entry) {
    const QString element = elementNameForEmotion(entry.emotion);
    const bool highIntensity = entry.intensity >= 4;
    const bool lowEnergy = entry.energy <= 2;
    const bool highEnergy = entry.energy >= 4;

    if (element == "火") {
        if (highIntensity && highEnergy) return "火行偏旺，情绪在持续外放，适合把高光和推动力留下来。";
        if (lowEnergy) return "火行转柔，开心里带着一点疲惫，更适合慢慢回味。";
        return "火行温亮，说明这份快乐正在稳定地照亮你。";
    }
    if (element == "水") {
        if (highEnergy) return "水行流动，表面平静但内在仍有推进感，适合继续整理想法。";
        return "水行安定，这段时间更像是在慢慢沉淀自己。";
    }
    if (element == "金") {
        if (highIntensity) return "金行偏重，情绪有收缩感，先允许难过存在，比急着化解更重要。";
        return "金行微凉，这份难过正在被你慢慢看清。";
    }
    if (element == "木") {
        if (highIntensity) return "木行上冲，边界感很明确，适合把触发点和真正需求写下来。";
        return "木行生发，这份情绪正在提醒你，有些东西值得被认真守护。";
    }
    if (highIntensity && lowEnergy) {
        return "土行偏滞，焦虑感在堆积，先拆出最小的一步会更轻。";
    }
    if (highEnergy) {
        return "土行仍有承载力，虽然在意很多，但你还保有继续往前的力量。";
    }
    return "土行轻压，这份不安正在寻找落点，写出来会让它变得更可被整理。";
}

QString compactPreviewText(const QString &text, int maxLength = 88) {
    QString normalized = text;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');
    normalized.replace('\n', ' ');
    normalized = normalized.simplified();
    if (normalized.size() <= maxLength) {
        return normalized;
    }
    return normalized.left(maxLength - 1) + "…";
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_repository(dataFilePath()) {
    buildUi();
    loadEntries();
    refreshViews();
}

void MainWindow::buildUi() {
    setWindowTitle("情绪星图日记");
    resize(1480, 920);

    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(18);

    auto *leftPanel = new QWidget(central);
    leftPanel->setFixedWidth(450);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_leftStack = new QStackedWidget(leftPanel);
    m_leftStack->setObjectName("leftStack");
    leftLayout->addWidget(m_leftStack);

    auto *homeScroll = new QScrollArea(m_leftStack);
    homeScroll->setWidgetResizable(true);
    homeScroll->setFrameShape(QFrame::NoFrame);
    homeScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *homePage = new QWidget(homeScroll);
    auto *homeLayout = new QVBoxLayout(homePage);
    homeLayout->setContentsMargins(0, 0, 0, 0);
    homeLayout->setSpacing(14);

    auto *heroCard = new QFrame(homePage);
    heroCard->setObjectName("heroCard");
    auto *heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(22, 20, 22, 20);
    heroLayout->setSpacing(10);

    auto *badge = new QLabel("我的情绪控制台", heroCard);
    badge->setObjectName("heroBadge");
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedWidth(124);

    m_homeHeadlineLabel = new QLabel("今天的你，值得被温柔接住。", heroCard);
    m_homeHeadlineLabel->setObjectName("heroTitle");

    m_homeSubheadlineLabel = new QLabel("这里不是一张冷冰冰的表单，而是你和今天重新对话的入口。", heroCard);
    m_homeSubheadlineLabel->setObjectName("heroSubtitle");
    m_homeSubheadlineLabel->setWordWrap(true);

    heroLayout->addWidget(badge, 0, Qt::AlignLeft);
    heroLayout->addWidget(m_homeHeadlineLabel);
    heroLayout->addWidget(m_homeSubheadlineLabel);

    auto *statsRow = new QHBoxLayout();
    statsRow->setSpacing(10);
    statsRow->addWidget(createStatCard("--", "我的最近情绪", homePage, &m_statMoodValue, &m_statMoodLabel));
    statsRow->addWidget(createStatCard("0", "我的连续记录", homePage, &m_statStreakValue, &m_statStreakLabel));
    statsRow->addWidget(createStatCard("--", "我的最后更新", homePage, &m_statUpdateValue, &m_statUpdateLabel));

    auto *calendarBox = new QGroupBox("我的本月回看", homePage);
    auto *calendarLayout = new QVBoxLayout(calendarBox);
    calendarLayout->setContentsMargins(18, 24, 18, 18);
    calendarLayout->setSpacing(12);

    m_calendarMonthLabel = new QLabel("2026年05月", calendarBox);
    m_calendarMonthLabel->setObjectName("companionTitle");
    m_calendarSummaryLabel = new QLabel("这个月的情绪还在慢慢长出来。", calendarBox);
    m_calendarSummaryLabel->setObjectName("sectionHint");
    m_calendarSummaryLabel->setWordWrap(true);

    auto *weekHeader = new QHBoxLayout();
    weekHeader->setSpacing(8);
    const QStringList weekdays = {"一", "二", "三", "四", "五", "六", "日"};
    for (const auto &day : weekdays) {
        auto *label = new QLabel(day, calendarBox);
        label->setObjectName("weekLabel");
        label->setAlignment(Qt::AlignCenter);
        weekHeader->addWidget(label);
    }

    auto *calendarGrid = new QGridLayout();
    calendarGrid->setHorizontalSpacing(8);
    calendarGrid->setVerticalSpacing(8);
    for (int i = 0; i < 35; ++i) {
        auto *cell = new QLabel("", calendarBox);
        cell->setAlignment(Qt::AlignCenter);
        cell->setObjectName("calendarCell");
        cell->setMinimumSize(42, 42);
        m_calendarCells.append(cell);
        calendarGrid->addWidget(cell, i / 7, i % 7);
    }

    m_tagTrendLabel = new QLabel("本月关键词：还没有标签", calendarBox);
    m_tagTrendLabel->setObjectName("summaryLabel");
    m_tagTrendLabel->setWordWrap(true);

    calendarLayout->addWidget(m_calendarMonthLabel);
    calendarLayout->addWidget(m_calendarSummaryLabel);
    calendarLayout->addLayout(weekHeader);
    calendarLayout->addLayout(calendarGrid);
    calendarLayout->addWidget(m_tagTrendLabel);

    auto *filterBox = new QGroupBox("我的标签视角", homePage);
    auto *filterLayout = new QVBoxLayout(filterBox);
    filterLayout->setContentsMargins(18, 24, 18, 18);
    filterLayout->setSpacing(10);

    auto *filterIntro = new QLabel("按生活主题切换星空，只看某一种来源的情绪轨迹。", filterBox);
    filterIntro->setObjectName("sectionHint");
    filterIntro->setWordWrap(true);

    m_tagFilterCombo = new QComboBox(filterBox);
    m_tagFilterCombo->addItem("全部情绪星", "");
    connect(m_tagFilterCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        m_activeTagFilter = m_tagFilterCombo->currentData().toString();
        refreshViews();
    });

    m_filterHintLabel = new QLabel("当前正在看全部情绪星。", filterBox);
    m_filterHintLabel->setObjectName("summaryLabel");
    m_filterHintLabel->setWordWrap(true);

    filterLayout->addWidget(filterIntro);
    filterLayout->addWidget(m_tagFilterCombo);
    filterLayout->addWidget(m_filterHintLabel);

    auto *timelineBox = new QGroupBox("我的标签时间轴", homePage);
    auto *timelineLayout = new QVBoxLayout(timelineBox);
    timelineLayout->setContentsMargins(18, 24, 18, 18);
    timelineLayout->setSpacing(10);

    auto *timelineIntro = new QLabel("同一个生活主题，会在这里按时间慢慢连成一条线。", timelineBox);
    timelineIntro->setObjectName("sectionHint");
    timelineIntro->setWordWrap(true);

    m_timelineSummaryLabel = new QLabel("当你开始使用标签后，这里会出现一条属于你的情绪时间轴。", timelineBox);
    m_timelineSummaryLabel->setObjectName("summaryLabel");
    m_timelineSummaryLabel->setWordWrap(true);

    m_timelineList = new QListWidget(timelineBox);
    m_timelineList->setObjectName("timelineList");
    m_timelineList->setMinimumHeight(196);
    connect(m_timelineList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (!current) {
                    return;
                }
                const QString id = current->data(Qt::UserRole).toString();
                if (!id.isEmpty()) {
                    selectEntryById(id);
                }
            });

    timelineLayout->addWidget(timelineIntro);
    timelineLayout->addWidget(m_timelineSummaryLabel);
    timelineLayout->addWidget(m_timelineList);

    auto *trendBox = new QGroupBox("这周和这个月的我", homePage);
    auto *trendLayout = new QVBoxLayout(trendBox);
    trendLayout->setContentsMargins(18, 24, 18, 18);
    trendLayout->setSpacing(10);

    auto *trendIntro = new QLabel("不只是记录，还要慢慢读懂自己。这里会把最近的变化翻译成更直观的话。", trendBox);
    trendIntro->setObjectName("sectionHint");
    trendIntro->setWordWrap(true);

    m_weekTrendLabel = new QLabel("这周的我：还在慢慢积累。", trendBox);
    m_weekTrendLabel->setObjectName("companionBody");
    m_weekTrendLabel->setWordWrap(true);

    m_monthTrendLabel = new QLabel("这个月的我：情绪星空还在形成。", trendBox);
    m_monthTrendLabel->setObjectName("companionBody");
    m_monthTrendLabel->setWordWrap(true);

    trendLayout->addWidget(trendIntro);
    trendLayout->addWidget(m_weekTrendLabel);
    trendLayout->addWidget(m_monthTrendLabel);

    auto *ritualBox = new QGroupBox("我的今天", homePage);
    auto *ritualLayout = new QVBoxLayout(ritualBox);
    ritualLayout->setContentsMargins(18, 24, 18, 18);
    ritualLayout->setSpacing(12);

    auto *ritualHint = new QLabel("像 Moo 日记那样轻一点地开始，不需要想很完整，先按下一个入口就好。", ritualBox);
    ritualHint->setObjectName("sectionHint");
    ritualHint->setWordWrap(true);

    m_openEditorButton = new QPushButton("记录我的今天", ritualBox);
    m_openEditorButton->setObjectName("primaryAction");
    connect(m_openEditorButton, &QPushButton::clicked, this, &MainWindow::openNewEntryPage);

    auto *ritualGrid = new QGridLayout();
    ritualGrid->setHorizontalSpacing(10);
    ritualGrid->setVerticalSpacing(10);

    m_viewTodayButton = createPromptButton("回到我的今天", "定位今天留下的情绪", ritualBox);
    m_viewLatestButton = createPromptButton("看看最新记录", "回到最近一次表达", ritualBox);
    m_viewRandomButton = createPromptButton("随机漫游一下", "看看过去某颗星", ritualBox);
    auto *releaseButton = createPromptButton("先放下这件事", "带着提示进入记录页", ritualBox);

    m_viewTodayButton->setObjectName("softCardButton");
    m_viewLatestButton->setObjectName("softCardButton");
    m_viewRandomButton->setObjectName("softCardButton");
    releaseButton->setObjectName("softCardButton");

    connect(m_viewLatestButton, &QPushButton::clicked, this, &MainWindow::viewLatestEntry);
    connect(m_viewTodayButton, &QPushButton::clicked, this, &MainWindow::viewTodayEntry);
    connect(m_viewRandomButton, &QPushButton::clicked, this, &MainWindow::viewRandomEntry);
    connect(releaseButton, &QPushButton::clicked, this, [this]() {
        applyEditorPrompt("我想轻一点的晚上",
                          "让我停不下来的那件事是：\n\n如果把它写出来，我最想先放下的是：",
                          "焦虑");
        m_summaryLabel->setText("已经为你打开一个更容易下笔的入口。");
    });

    ritualGrid->addWidget(m_viewTodayButton, 0, 0);
    ritualGrid->addWidget(m_viewLatestButton, 0, 1);
    ritualGrid->addWidget(m_viewRandomButton, 1, 0);
    ritualGrid->addWidget(releaseButton, 1, 1);

    ritualLayout->addWidget(ritualHint);
    ritualLayout->addWidget(m_openEditorButton);
    ritualLayout->addLayout(ritualGrid);

    auto *companionBox = new QGroupBox("我的情绪安放", homePage);
    auto *companionLayout = new QVBoxLayout(companionBox);
    companionLayout->setContentsMargins(18, 24, 18, 18);
    companionLayout->setSpacing(12);

    auto *companionHint = new QLabel("参考解忧娃娃那种被陪伴的感觉，我把这里做成更像“先接住自己”的地方。", companionBox);
    companionHint->setObjectName("sectionHint");
    companionHint->setWordWrap(true);

    auto *companionCard = new QFrame(companionBox);
    companionCard->setObjectName("companionCard");
    auto *companionCardLayout = new QVBoxLayout(companionCard);
    companionCardLayout->setContentsMargins(16, 16, 16, 16);
    companionCardLayout->setSpacing(6);

    m_companionTitleLabel = new QLabel("先听听此刻的自己。", companionCard);
    m_companionTitleLabel->setObjectName("companionTitle");
    m_companionBodyLabel = new QLabel("如果你还不知道怎么写，就先从今天最在意的一件小事开始。", companionCard);
    m_companionBodyLabel->setObjectName("companionBody");
    m_companionBodyLabel->setWordWrap(true);
    companionCardLayout->addWidget(m_companionTitleLabel);
    companionCardLayout->addWidget(m_companionBodyLabel);

    auto *companionActions = new QHBoxLayout();
    companionActions->setSpacing(10);

    auto *talkButton = new QPushButton("先把这件事说出来", companionBox);
    talkButton->setObjectName("secondaryAction");
    connect(talkButton, &QPushButton::clicked, this, [this]() {
        applyEditorPrompt("今天最想说的一件事",
                          "如果现在只能说一句，我最想说的是：\n",
                          "平静");
    });

    auto *keepMomentButton = new QPushButton("留住一个小瞬间", companionBox);
    keepMomentButton->setObjectName("secondaryAction");
    connect(keepMomentButton, &QPushButton::clicked, this, [this]() {
        applyEditorPrompt("我想留下的一个瞬间",
                          "今天有一个让我想记住的小瞬间：\n",
                          "希望");
    });

    auto *comfortButton = new QPushButton("给自己一句安慰", companionBox);
    comfortButton->setObjectName("ghostAction");
    connect(comfortButton, &QPushButton::clicked, this, [this]() {
        const EmotionEntry *latest = m_entries.isEmpty() ? nullptr : &m_entries.front();
        const QString emotion = latest ? latest->emotion : "希望";
        m_companionTitleLabel->setText("现在的你，也可以慢一点。");
        m_companionBodyLabel->setText(comfortLineForEmotion(emotion));
    });

    companionActions->addWidget(talkButton);
    companionActions->addWidget(keepMomentButton);
    companionLayout->addWidget(companionHint);
    companionLayout->addWidget(companionCard);
    companionLayout->addLayout(companionActions);
    companionLayout->addWidget(comfortButton);

    auto *listBox = new QGroupBox("我的星空档案", homePage);
    auto *listLayout = new QVBoxLayout(listBox);
    listLayout->setContentsMargins(18, 24, 18, 18);
    listLayout->setSpacing(12);

    auto *archiveHint = new QLabel("这里会保存我写下的每一次情绪记录。点开任意一条，右侧星空会同步定位到它。", listBox);
    archiveHint->setObjectName("sectionHint");
    archiveHint->setWordWrap(true);

    m_summaryLabel = new QLabel(listBox);
    m_summaryLabel->setObjectName("summaryLabel");
    m_summaryLabel->setWordWrap(true);

    m_entryList = new QListWidget(listBox);
    connect(m_entryList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *, QListWidgetItem *) {
        handleListSelectionChanged();
    });

    auto *archiveActionRow = new QHBoxLayout();
    archiveActionRow->setSpacing(10);

    m_editButton = new QPushButton("编辑这条记录", listBox);
    m_editButton->setObjectName("secondaryAction");
    m_deleteButton = new QPushButton("删除", listBox);
    m_deleteButton->setObjectName("dangerButton");
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::editSelectedEntry);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedEntry);

    archiveActionRow->addWidget(m_editButton, 2);
    archiveActionRow->addWidget(m_deleteButton, 1);

    listLayout->addWidget(archiveHint);
    listLayout->addWidget(m_summaryLabel);
    listLayout->addWidget(m_entryList, 1);
    listLayout->addLayout(archiveActionRow);

    homeLayout->addWidget(heroCard);
    homeLayout->addLayout(statsRow);
    homeLayout->addWidget(calendarBox);
    homeLayout->addWidget(filterBox);
    homeLayout->addWidget(timelineBox);
    homeLayout->addWidget(trendBox);
    homeLayout->addWidget(ritualBox);
    homeLayout->addWidget(companionBox);
    homeLayout->addWidget(listBox, 1);
    homeLayout->addStretch();

    homeScroll->setWidget(homePage);

    auto *editorScroll = new QScrollArea(m_leftStack);
    editorScroll->setWidgetResizable(true);
    editorScroll->setFrameShape(QFrame::NoFrame);
    editorScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *editorPage = new QWidget(editorScroll);
    auto *editorLayout = new QVBoxLayout(editorPage);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(14);

    auto *editorShell = new QFrame(editorPage);
    editorShell->setObjectName("heroCard");
    auto *editorShellLayout = new QVBoxLayout(editorShell);
    editorShellLayout->setContentsMargins(22, 20, 22, 20);
    editorShellLayout->setSpacing(12);

    auto *editorTopRow = new QHBoxLayout();
    auto *editorTitleWrap = new QVBoxLayout();
    editorTitleWrap->setSpacing(4);

    auto *editorBadge = new QLabel("我的记录页", editorShell);
    editorBadge->setObjectName("heroBadge");
    editorBadge->setAlignment(Qt::AlignCenter);
    editorBadge->setFixedWidth(96);

    auto *editorTitle = new QLabel("记录我的今天", editorShell);
    editorTitle->setObjectName("heroTitle");

    auto *editorSubtitle = new QLabel("像 iCity 或 Moo 那样轻一点地开始。你不需要一次写得很完整，先从一个入口写下去就好。", editorShell);
    editorSubtitle->setObjectName("heroSubtitle");
    editorSubtitle->setWordWrap(true);

    editorTitleWrap->addWidget(editorBadge, 0, Qt::AlignLeft);
    editorTitleWrap->addWidget(editorTitle);
    editorTitleWrap->addWidget(editorSubtitle);

    m_backButton = new QPushButton("返回", editorShell);
    m_backButton->setObjectName("backAction");
    m_backButton->setFixedWidth(88);
    connect(m_backButton, &QPushButton::clicked, this, &MainWindow::handleBackFromEditor);

    editorTopRow->addLayout(editorTitleWrap, 1);
    editorTopRow->addWidget(m_backButton, 0, Qt::AlignTop);

    auto *starterBox = new QGroupBox("快速开始", editorShell);
    auto *starterLayout = new QVBoxLayout(starterBox);
    starterLayout->setContentsMargins(18, 24, 18, 18);
    starterLayout->setSpacing(10);

    auto *starterHint = new QLabel("如果你不知道怎么写，可以先点一个提词入口。", starterBox);
    starterHint->setObjectName("sectionHint");

    auto *starterGrid = new QGridLayout();
    starterGrid->setHorizontalSpacing(10);
    starterGrid->setVerticalSpacing(10);

    auto *promptMood = createPromptButton("今天最明显的情绪", "先写感受，再写原因", starterBox);
    auto *promptEvent = createPromptButton("让我停不下来的事", "先把卡住的点说出来", starterBox);
    auto *promptMoment = createPromptButton("我想留下的瞬间", "把一个小小高光记下来", starterBox);

    promptMood->setObjectName("softCardButton");
    promptEvent->setObjectName("softCardButton");
    promptMoment->setObjectName("softCardButton");

    connect(promptMood, &QPushButton::clicked, this, [this]() {
        applyEditorPrompt("今天最明显的情绪", "如果用一句话描述今天最明显的情绪，它是：\n因为：", "平静");
    });
    connect(promptEvent, &QPushButton::clicked, this, [this]() {
        applyEditorPrompt("让我停不下来的那件事", "我一直放不下的那件事是：\n它最让我在意的是：", "焦虑");
    });
    connect(promptMoment, &QPushButton::clicked, this, [this]() {
        applyEditorPrompt("我想留下的一个瞬间", "今天有一个值得被轻轻留下来的瞬间：\n", "希望");
    });

    starterGrid->addWidget(promptMood, 0, 0);
    starterGrid->addWidget(promptEvent, 0, 1);
    starterGrid->addWidget(promptMoment, 1, 0, 1, 2);

    starterLayout->addWidget(starterHint);
    starterLayout->addLayout(starterGrid);

    auto *formBox = new QGroupBox("记录内容", editorShell);
    auto *formLayout = new QVBoxLayout(formBox);
    formLayout->setContentsMargins(18, 24, 18, 18);
    formLayout->setSpacing(12);

    auto *formIntro = new QLabel("留下一句标题、一种情绪和一点今天的故事。", formBox);
    formIntro->setObjectName("sectionHint");

    m_titleEdit = new QLineEdit(formBox);
    m_titleEdit->setPlaceholderText("例如：终于松一口气的晚上");

    m_emotionCombo = new QComboBox(formBox);
    m_emotionCombo->addItems({"快乐", "平静", "难过", "愤怒", "焦虑", "希望"});

    m_dateEdit = new QDateEdit(QDate::currentDate(), formBox);
    m_dateEdit->setCalendarPopup(true);

    m_intensitySlider = new QSlider(Qt::Horizontal, formBox);
    m_intensitySlider->setRange(1, 5);
    m_intensitySlider->setValue(3);

    m_energySlider = new QSlider(Qt::Horizontal, formBox);
    m_energySlider->setRange(1, 5);
    m_energySlider->setValue(3);

    m_tagEdit = new QLineEdit(formBox);
    m_tagEdit->setPlaceholderText("给今天加几个标签：学习，朋友，睡眠，家庭");

    auto *tagPresetGrid = new QGridLayout();
    tagPresetGrid->setHorizontalSpacing(8);
    tagPresetGrid->setVerticalSpacing(8);
    const QStringList presetTags = {"学习", "朋友", "家庭", "独处", "睡眠", "压力", "目标", "开心"};
    for (int i = 0; i < presetTags.size(); ++i) {
        auto *tagButton = new QPushButton("#" + presetTags.at(i), formBox);
        tagButton->setObjectName("tagButton");
        connect(tagButton, &QPushButton::clicked, this, [this, preset = presetTags.at(i)]() {
            appendTagToEditor(preset);
        });
        tagPresetGrid->addWidget(tagButton, i / 4, i % 4);
    }

    m_noteEdit = new QPlainTextEdit(formBox);
    m_noteEdit->setPlaceholderText("今天发生了什么？这份情绪是从哪里来的？");
    m_noteEdit->setMinimumHeight(180);

    formLayout->addWidget(formIntro);
    formLayout->addWidget(new QLabel("标题", formBox));
    formLayout->addWidget(m_titleEdit);
    formLayout->addWidget(new QLabel("情绪类型", formBox));
    formLayout->addWidget(m_emotionCombo);

    auto *dateEnergyRow = new QHBoxLayout();
    dateEnergyRow->setSpacing(10);

    auto *dateColumn = new QVBoxLayout();
    dateColumn->setSpacing(6);
    dateColumn->addWidget(new QLabel("日期", formBox));
    dateColumn->addWidget(m_dateEdit);

    auto *energyColumn = new QVBoxLayout();
    energyColumn->setSpacing(6);
    energyColumn->addWidget(new QLabel("精神能量", formBox));
    energyColumn->addWidget(m_energySlider);

    dateEnergyRow->addLayout(dateColumn, 1);
    dateEnergyRow->addLayout(energyColumn, 1);

    auto *intensityColumn = new QVBoxLayout();
    intensityColumn->setSpacing(6);
    intensityColumn->addWidget(new QLabel("情绪强度", formBox));
    intensityColumn->addWidget(m_intensitySlider);

    formLayout->addLayout(dateEnergyRow);
    formLayout->addLayout(intensityColumn);
    formLayout->addWidget(new QLabel("生活标签", formBox));
    formLayout->addWidget(m_tagEdit);
    formLayout->addLayout(tagPresetGrid);
    formLayout->addWidget(new QLabel("内容", formBox));
    formLayout->addWidget(m_noteEdit);

    auto *footerCard = new QFrame(formBox);
    footerCard->setObjectName("companionCard");
    auto *footerLayout = new QVBoxLayout(footerCard);
    footerLayout->setContentsMargins(14, 14, 14, 14);
    footerLayout->setSpacing(4);

    auto *footerTitle = new QLabel("写不下去也没关系。", footerCard);
    footerTitle->setObjectName("companionTitle");
    auto *footerBody = new QLabel("哪怕今天只写下一句话，它也会成为你的星图里真实存在的一颗星。", footerCard);
    footerBody->setObjectName("companionBody");
    footerBody->setWordWrap(true);
    footerLayout->addWidget(footerTitle);
    footerLayout->addWidget(footerBody);

    auto *primaryActions = new QHBoxLayout();
    primaryActions->setSpacing(10);
    m_saveButton = new QPushButton("保存并返回", formBox);
    m_saveButton->setObjectName("primaryAction");
    m_clearButton = new QPushButton("重新填写", formBox);
    m_clearButton->setObjectName("secondaryAction");
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::addEntry);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::clearEditor);
    primaryActions->addWidget(m_saveButton, 2);
    primaryActions->addWidget(m_clearButton, 1);

    formLayout->addWidget(footerCard);
    formLayout->addLayout(primaryActions);

    editorShellLayout->addLayout(editorTopRow);
    editorShellLayout->addWidget(starterBox);
    editorShellLayout->addWidget(formBox);
    editorLayout->addWidget(editorShell);
    editorLayout->addStretch();

    editorScroll->setWidget(editorPage);

    m_leftStack->addWidget(homeScroll);
    m_leftStack->addWidget(editorScroll);

    auto markDirty = [this]() {
        if (!m_ignoreEditorChanges) {
            setEditorDirty(true);
        }
    };
    connect(m_titleEdit, &QLineEdit::textChanged, this, [markDirty](const QString &) { markDirty(); });
    connect(m_noteEdit, &QPlainTextEdit::textChanged, this, markDirty);
    connect(m_tagEdit, &QLineEdit::textChanged, this, [markDirty](const QString &) { markDirty(); });
    connect(m_emotionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [markDirty](int) { markDirty(); });
    connect(m_dateEdit, &QDateEdit::dateChanged, this, [markDirty](const QDate &) { markDirty(); });
    connect(m_intensitySlider, &QSlider::valueChanged, this, [markDirty](int) { markDirty(); });
    connect(m_energySlider, &QSlider::valueChanged, this, [markDirty](int) { markDirty(); });

    auto *rightPanel = new QWidget(central);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(14);

    auto *viewerCard = new QFrame(rightPanel);
    viewerCard->setObjectName("viewerCard");
    auto *viewerLayout = new QVBoxLayout(viewerCard);
    viewerLayout->setContentsMargins(14, 14, 14, 14);
    viewerLayout->setSpacing(12);

    auto *viewerHeader = new QHBoxLayout();
    auto *viewerTitleWrap = new QVBoxLayout();
    viewerTitleWrap->setSpacing(2);
    auto *viewerTitle = new QLabel("你的情绪星空", viewerCard);
    viewerTitle->setObjectName("viewerTitle");
    auto *viewerSubtitle = new QLabel("拖动球面星空来观察不同角度。每条记录会按日期排布，并结合情绪内容映射到对应的星象方位。", viewerCard);
    viewerSubtitle->setObjectName("viewerSubtitle");
    viewerSubtitle->setWordWrap(true);
    viewerTitleWrap->addWidget(viewerTitle);
    viewerTitleWrap->addWidget(viewerSubtitle);
    viewerHeader->addLayout(viewerTitleWrap, 1);

    m_starMapView = new StarMapView(viewerCard);
    m_starMapView->setMinimumHeight(560);
    connect(m_starMapView, &StarMapView::entrySelected, this, &MainWindow::handleStarSelection);

    auto *detailBox = new QGroupBox("五行情绪卡", viewerCard);
    detailBox->setObjectName("detailBox");
    detailBox->setMaximumHeight(218);
    auto *detailLayout = new QVBoxLayout(detailBox);
    detailLayout->setContentsMargins(16, 18, 16, 16);
    detailLayout->setSpacing(10);

    m_detailIcon = new QLabel(detailBox);
    m_detailTitle = new QLabel("当前没有选中日记", detailBox);
    m_detailMeta = new QLabel("请从右侧星空中点亮一颗星，或从左侧档案里选择一条记录。", detailBox);
    m_detailMeta->setWordWrap(true);
    m_detailNote = new QLabel(detailBox);
    m_detailNote->setWordWrap(true);
    m_detailNote->setTextFormat(Qt::RichText);
    m_detailNote->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_detailNote->setMinimumHeight(76);
    m_detailIcon->setAlignment(Qt::AlignCenter);
    m_detailIcon->setFixedSize(46, 46);

    m_detailIcon->setObjectName("detailIcon");
    m_detailTitle->setObjectName("detailTitle");
    m_detailMeta->setObjectName("detailMeta");
    m_detailNote->setObjectName("detailNoteBody");

    auto *detailHeader = new QHBoxLayout();
    detailHeader->setSpacing(12);
    auto *detailTitleWrap = new QVBoxLayout();
    detailTitleWrap->setSpacing(3);
    detailTitleWrap->addWidget(m_detailTitle);
    detailTitleWrap->addWidget(m_detailMeta);
    detailHeader->addWidget(m_detailIcon, 0, Qt::AlignTop);
    detailHeader->addLayout(detailTitleWrap, 1);

    detailLayout->addLayout(detailHeader);
    detailLayout->addWidget(m_detailNote);

    viewerLayout->addLayout(viewerHeader);
    viewerLayout->addWidget(m_starMapView, 1);
    viewerLayout->addWidget(detailBox);

    rightLayout->addWidget(viewerCard, 1);

    rootLayout->addWidget(leftPanel);
    rootLayout->addWidget(rightPanel, 1);
    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow { background: #e9eef5; }
        QWidget { color: #152033; font-size: 14px; }
        QStackedWidget#leftStack { background: transparent; }
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { width: 8px; background: transparent; }
        QScrollBar::handle:vertical { background: rgba(148,163,184,0.45); border-radius: 4px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QFrame#heroCard, QFrame#viewerCard, QFrame#statCard {
            background: rgba(255,255,255,0.82);
            border: 1px solid rgba(255,255,255,0.92);
            border-radius: 28px;
        }
        QLabel#heroBadge {
            background: rgba(245,247,252,0.98);
            color: #64748b;
            border: 1px solid rgba(219,228,243,0.98);
            border-radius: 14px;
            padding: 6px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#heroTitle { font-size: 29px; font-weight: 700; color: #0f172a; }
        QLabel#heroSubtitle { font-size: 14px; color: #61738b; line-height: 1.5; }
        QLabel#sectionHint { font-size: 12px; color: #7a879b; padding-left: 2px; }
        QLabel#statNumber { font-size: 22px; font-weight: 700; color: #10233f; }
        QLabel#statText { font-size: 12px; color: #6a7a92; }
        QLabel#viewerTitle { font-size: 24px; font-weight: 700; color: white; }
        QLabel#viewerSubtitle { font-size: 13px; color: #a9b8d3; }
        QLabel#detailIcon {
            font-size: 22px;
            font-weight: 700;
            color: #ffe4a3;
            background: rgba(255,255,255,0.08);
            border: 1px solid rgba(255,255,255,0.12);
            border-radius: 23px;
        }
        QLabel#detailTitle { font-size: 17px; font-weight: 700; color: white; }
        QLabel#detailMeta { font-size: 12px; color: #c8d5f0; line-height: 1.45; }
        QLabel#detailNoteBody {
            background: rgba(255,255,255,0.04);
            border: 1px solid rgba(73,92,128,0.58);
            border-radius: 16px;
            color: #eef4ff;
            padding: 12px 14px;
            font-size: 13px;
            line-height: 1.55;
        }
        QLabel#summaryLabel { color: #61738b; font-size: 13px; padding-left: 2px; }
        QLabel#companionTitle { font-size: 17px; font-weight: 700; color: #0f172a; }
        QLabel#companionBody { font-size: 13px; color: #5f6f86; line-height: 1.45; }
        QLabel#weekLabel { font-size: 12px; color: #8a97ab; }
        QLabel#calendarCell {
            background: rgba(248,250,255,0.96);
            border: 1px solid rgba(220,229,243,0.95);
            border-radius: 14px;
            color: #51627a;
            font-size: 12px;
            font-weight: 600;
        }
        QFrame#companionCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(242,247,255,0.98), stop:1 rgba(233,240,252,0.98));
            border: 1px solid rgba(220,229,243,0.95);
            border-radius: 22px;
        }
        QGroupBox {
            border: 1px solid rgba(255,255,255,0.88);
            border-radius: 26px;
            margin-top: 10px;
            font-weight: 700;
            padding-top: 14px;
            background: rgba(255,255,255,0.8);
        }
        QGroupBox::title {
            left: 18px;
            padding: 0 6px;
            color: #10213c;
            font-size: 15px;
        }
        QLineEdit, QDateEdit, QComboBox, QPlainTextEdit, QListWidget {
            background: rgba(249,251,255,0.98);
            border: 1px solid #d9e3f2;
            border-radius: 18px;
            padding: 11px 13px;
            color: #142033;
            selection-background-color: #dce8ff;
        }
        QPlainTextEdit, QListWidget { padding-top: 12px; }
        QSlider::groove:horizontal { height: 6px; background: #dfe7f4; border-radius: 3px; }
        QSlider::sub-page:horizontal { background: #17345f; border-radius: 3px; }
        QSlider::handle:horizontal {
            width: 18px; margin: -6px 0; border-radius: 9px;
            background: white; border: 1px solid #ced9eb;
        }
        QPushButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f6f9ff, stop:1 #dfe8fb);
            color: #10213d;
            border-radius: 18px;
            padding: 12px 16px;
            border: 1px solid rgba(215,224,242,0.95);
            font-weight: 700;
            text-align: left;
        }
        QPushButton:hover { background: #fbfdff; }
        QPushButton#primaryAction {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #17345f, stop:1 #0f2446);
            color: white;
            border: 1px solid rgba(24,52,95,0.9);
            padding: 15px 18px;
        }
        QPushButton#secondaryAction { background: rgba(248,250,255,0.98); }
        QPushButton#ghostAction, QPushButton#backAction { background: rgba(245,248,253,0.74); }
        QPushButton#dangerButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ff8f87, stop:1 #ff6c60);
            color: white;
            border: 1px solid rgba(255,108,96,0.72);
            text-align: center;
        }
        QPushButton#dangerButton:hover { background: #ff7a6f; }
        QPushButton#softCardButton, QPushButton#promptAction {
            background: rgba(248,250,255,0.98);
            border-radius: 22px;
            padding: 14px 16px;
            font-size: 13px;
            line-height: 1.45;
        }
        QPushButton#tagButton {
            background: rgba(243,248,255,0.98);
            border-radius: 14px;
            padding: 10px 12px;
            font-size: 12px;
            color: #28405c;
        }
        QListWidget::item { padding: 11px; margin: 3px 0; }
        QListWidget::item:selected {
            background: #dde8ff;
            color: #10213d;
            border-radius: 14px;
        }
        QListWidget#timelineList::item {
            padding: 13px 12px;
            margin: 4px 0;
            border-radius: 16px;
        }
        QFrame#viewerCard, QGroupBox#detailBox {
            background: rgba(7,12,24,0.92);
            border: 1px solid rgba(43,60,92,0.92);
        }
        QGroupBox#detailBox::title { color: white; }
    )");

    showHomePage();
}

void MainWindow::loadEntries() {
    m_entries = m_repository.load();
    if (m_entries.isEmpty()) {
        m_entries = demoEntries();
        m_repository.save(m_entries);
    }
}

void MainWindow::refreshCalendarReview() {
    const QDate today = QDate::currentDate();
    const QDate monthStart(today.year(), today.month(), 1);
    const int startOffset = monthStart.dayOfWeek() - 1;
    const int daysInMonth = monthStart.daysInMonth();

    m_calendarMonthLabel->setText(today.toString("yyyy年MM月"));

    QMap<QString, int> emotionCount;
    QMap<QString, int> tagCount;
    int monthlyEntries = 0;

    for (auto *cell : m_calendarCells) {
        cell->clear();
        cell->setStyleSheet("");
        cell->setToolTip("");
    }

    for (const auto &entry : m_entries) {
        if (entry.date.year() == today.year() && entry.date.month() == today.month()) {
            ++monthlyEntries;
            emotionCount[entry.emotion] += 1;
            for (const auto &tag : entry.tags) {
                tagCount[tag] += 1;
            }
        }
    }

    QString dominantEmotion = "还没有记录";
    int dominantCount = 0;
    for (auto it = emotionCount.constBegin(); it != emotionCount.constEnd(); ++it) {
        if (it.value() > dominantCount) {
            dominantCount = it.value();
            dominantEmotion = it.key();
        }
    }

    for (int day = 1; day <= daysInMonth; ++day) {
        const int index = startOffset + day - 1;
        if (index < 0 || index >= m_calendarCells.size()) {
            continue;
        }

        const QDate current(today.year(), today.month(), day);
        auto *cell = m_calendarCells.at(index);
        cell->setText(QString::number(day));

        const EmotionEntry *matchedEntry = nullptr;
        for (const auto &entry : m_entries) {
            if (entry.date == current) {
                matchedEntry = &entry;
                break;
            }
        }

        if (!matchedEntry) {
            cell->setStyleSheet("background: rgba(248,250,255,0.96); border: 1px solid rgba(220,229,243,0.95); border-radius: 14px; color: #94a3b8;");
            continue;
        }

        const QColor base = EmotionEntry::colorForEmotion(matchedEntry->emotion);
        cell->setStyleSheet(QString(
            "background: rgba(%1,%2,%3,0.55); border: 1px solid rgba(%1,%2,%3,0.85); border-radius: 14px; color: #0f172a; font-weight: 700;")
                                .arg(base.red()).arg(base.green()).arg(base.blue()));
        cell->setToolTip(QString("%1\n%2\n%3")
                             .arg(current.toString("MM-dd"))
                             .arg(matchedEntry->emotion)
                             .arg(matchedEntry->title));
    }

    QList<QPair<QString, int>> tagPairs;
    for (auto it = tagCount.constBegin(); it != tagCount.constEnd(); ++it) {
        tagPairs.append({it.key(), it.value()});
    }
    std::sort(tagPairs.begin(), tagPairs.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    QStringList topTags;
    for (int i = 0; i < std::min(3, int(tagPairs.size())); ++i) {
        topTags.append("#" + tagPairs.at(i).first);
    }

    m_calendarSummaryLabel->setText(QString("本月已记录 %1 天，最常出现的是“%2”。").arg(monthlyEntries).arg(dominantEmotion));
    m_tagTrendLabel->setText(topTags.isEmpty()
                                 ? "本月关键词：还没有标签"
                                 : QString("本月关键词：%1").arg(topTags.join("  ")));
}

void MainWindow::refreshViews() {
    std::sort(m_entries.begin(), m_entries.end(), [](const EmotionEntry &a, const EmotionEntry &b) {
        return a.date > b.date;
    });

    m_entryList->clear();
    for (const auto &entry : m_entries) {
        const QString tagPreview = entry.tags.isEmpty() ? "" : QString("  ·  %1").arg("#" + entry.tags.first());
        auto *item = new QListWidgetItem(
            QString("%1  |  %2  |  %3%4").arg(entry.date.toString("MM-dd"), entry.emotion, entry.title, tagPreview),
            m_entryList);
        item->setData(Qt::UserRole, entry.id);
        item->setForeground(EmotionEntry::colorForEmotion(entry.emotion));
    }

    int warmCount = 0;
    int brightCount = 0;
    int streak = 0;
    QMap<QString, int> tagFrequency;
    for (const auto &entry : m_entries) {
        if (entry.emotion == "快乐" || entry.emotion == "希望") {
            ++warmCount;
        }
        if (entry.intensity >= 4) {
            ++brightCount;
        }
        for (const auto &tag : entry.tags) {
            tagFrequency[tag] += 1;
        }
    }

    if (!m_entries.isEmpty()) {
        QDate cursor = m_entries.front().date;
        for (const auto &entry : m_entries) {
            if (entry.date == cursor) {
                ++streak;
                cursor = cursor.addDays(-1);
            } else if (entry.date < cursor) {
                break;
            }
        }
    }

    QString topTag;
    int topTagCount = 0;
    for (auto it = tagFrequency.constBegin(); it != tagFrequency.constEnd(); ++it) {
        if (it.value() > topTagCount) {
            topTagCount = it.value();
            topTag = it.key();
        }
    }

    m_summaryLabel->setText(
        topTag.isEmpty()
            ? QString("星星总数：%1    耀眼记忆：%2    温暖情绪：%3").arg(m_entries.size()).arg(brightCount).arg(warmCount)
            : QString("星星总数：%1    耀眼记忆：%2    本周高频标签：#%3").arg(m_entries.size()).arg(brightCount).arg(topTag));

    if (!m_entries.isEmpty()) {
        const auto &latest = m_entries.front();
        m_statMoodValue->setText(latest.emotion);
        m_statMoodLabel->setText("我的最近情绪");
        m_statStreakValue->setText(QString::number(streak));
        m_statStreakLabel->setText(streak == 1 ? "我已连续记录 1 天" : QString("我已连续记录 %1 天").arg(streak));
        m_statUpdateValue->setText(latest.date.toString("MM-dd"));
        m_statUpdateLabel->setText("我的最后更新");

        m_homeHeadlineLabel->setText(homeHeadlineForEmotion(latest.emotion));
        m_homeSubheadlineLabel->setText(
            QString("最新一条是“%1”。%2")
                .arg(latest.title)
                .arg(latest.tags.isEmpty() ? "你可以继续写下今天，也可以回头看看自己是怎样一路走到这里的。"
                                           : QString("它被你放进了 %1 这些生活主题里。").arg(tagBubbleText(latest.tags))));
        m_companionTitleLabel->setText("此刻的你，最值得被好好放下。");
        m_companionBodyLabel->setText(comfortLineForEmotion(latest.emotion));
    } else {
        m_statMoodValue->setText("--");
        m_statMoodLabel->setText("我的最近情绪");
        m_statStreakValue->setText("0");
        m_statStreakLabel->setText("我的连续记录");
        m_statUpdateValue->setText("--");
        m_statUpdateLabel->setText("我的最后更新");
        m_homeHeadlineLabel->setText("今天的你，值得被温柔接住。");
        m_homeSubheadlineLabel->setText("从一条很短很短的记录开始，也没有关系。");
        m_companionTitleLabel->setText("先听听此刻的自己。");
        m_companionBodyLabel->setText("如果你还不知道怎么写，就先从今天最在意的一件小事开始。");
    }

    const QDate today = QDate::currentDate();
    const QDate weekStart = today.addDays(-6);
    int weeklyCount = 0;
    int weeklyEnergy = 0;
    QMap<QString, int> weeklyEmotionCount;
    int monthlyBrightest = 0;
    QString monthlyBrightestDay = "--";
    for (const auto &entry : m_entries) {
        if (entry.date >= weekStart && entry.date <= today) {
            ++weeklyCount;
            weeklyEnergy += entry.energy;
            weeklyEmotionCount[entry.emotion] += 1;
        }
        if (entry.date.year() == today.year() && entry.date.month() == today.month() &&
            entry.intensity > monthlyBrightest) {
            monthlyBrightest = entry.intensity;
            monthlyBrightestDay = entry.date.toString("MM-dd");
        }
    }

    QString weeklyDominant = "还没有明显情绪";
    int weeklyDominantCount = 0;
    for (auto it = weeklyEmotionCount.constBegin(); it != weeklyEmotionCount.constEnd(); ++it) {
        if (it.value() > weeklyDominantCount) {
            weeklyDominantCount = it.value();
            weeklyDominant = it.key();
        }
    }

    const double averageEnergy = weeklyCount == 0 ? 0.0 : double(weeklyEnergy) / double(weeklyCount);
    m_weekTrendLabel->setText(
        weeklyCount == 0
            ? "这周的我：还没有留下记录，可以先写下今天。"
            : QString("这周的我：共记录 %1 天，最常出现的是“%2”，平均能量 %3。")
                  .arg(weeklyCount)
                  .arg(weeklyDominant)
                  .arg(averageEnergy, 0, 'f', 1));

    m_monthTrendLabel->setText(
        topTag.isEmpty()
            ? QString("这个月的我：最亮的一天是 %1，情绪星空正在慢慢成形。").arg(monthlyBrightestDay)
            : QString("这个月的我：最亮的一天是 %1，反复出现的生活主题是 #%2。")
                  .arg(monthlyBrightestDay)
                  .arg(topTag));

    {
        const QSignalBlocker blocker(m_tagFilterCombo);
        const QString previousFilter = m_activeTagFilter;
        m_tagFilterCombo->clear();
        m_tagFilterCombo->addItem("全部情绪星", "");

        QList<QPair<QString, int>> filterTagPairs;
        for (auto it = tagFrequency.constBegin(); it != tagFrequency.constEnd(); ++it) {
            filterTagPairs.append({it.key(), it.value()});
        }
        std::sort(filterTagPairs.begin(), filterTagPairs.end(), [](const auto &a, const auto &b) {
            if (a.second == b.second) {
                return a.first < b.first;
            }
            return a.second > b.second;
        });
        for (const auto &pair : filterTagPairs) {
            m_tagFilterCombo->addItem(QString("#%1  ·  %2 条记录").arg(pair.first).arg(pair.second), pair.first);
        }

        int restoreIndex = m_tagFilterCombo->findData(previousFilter);
        if (restoreIndex < 0) {
            restoreIndex = 0;
        }
        m_tagFilterCombo->setCurrentIndex(restoreIndex);
    }
    m_activeTagFilter = m_tagFilterCombo->currentData().toString();

    QList<EmotionEntry> visibleEntries = m_entries;
    if (!m_activeTagFilter.isEmpty()) {
        visibleEntries.clear();
        for (const auto &entry : m_entries) {
            if (entry.tags.contains(m_activeTagFilter)) {
                visibleEntries.append(entry);
            }
        }
    }

    m_filterHintLabel->setText(
        m_activeTagFilter.isEmpty()
            ? "当前正在看全部情绪星。"
            : QString("当前正在看标签 #%1 下的情绪轨迹，共 %2 颗星。")
                  .arg(m_activeTagFilter)
                  .arg(visibleEntries.size()));

    const QString timelineTag = !m_activeTagFilter.isEmpty() ? m_activeTagFilter : topTag;
    m_timelineList->clear();
    if (timelineTag.isEmpty()) {
        m_timelineSummaryLabel->setText("当你给记录加上生活标签后，这里会把同一主题慢慢连成一条时间轴。");
        auto *placeholder = new QListWidgetItem("还没有可以展开的标签时间轴", m_timelineList);
        placeholder->setFlags(Qt::NoItemFlags);
        placeholder->setForeground(QColor("#8fa1ba"));
    } else {
        QList<EmotionEntry> timelineEntries;
        QMap<QString, int> timelineEmotionCount;
        int totalIntensity = 0;
        int totalEnergy = 0;
        for (const auto &entry : m_entries) {
            if (entry.tags.contains(timelineTag)) {
                timelineEntries.append(entry);
                timelineEmotionCount[entry.emotion] += 1;
                totalIntensity += entry.intensity;
                totalEnergy += entry.energy;
            }
        }

        QString dominantEmotion = "平静";
        int dominantCount = 0;
        for (auto it = timelineEmotionCount.constBegin(); it != timelineEmotionCount.constEnd(); ++it) {
            if (it.value() > dominantCount) {
                dominantEmotion = it.key();
                dominantCount = it.value();
            }
        }

        const double averageIntensity =
            timelineEntries.isEmpty() ? 0.0 : double(totalIntensity) / double(timelineEntries.size());
        const double averageEnergy =
            timelineEntries.isEmpty() ? 0.0 : double(totalEnergy) / double(timelineEntries.size());
        const QDate latestDate = timelineEntries.isEmpty() ? QDate() : timelineEntries.front().date;
        const QDate earliestDate = timelineEntries.isEmpty() ? QDate() : timelineEntries.back().date;

        m_timelineSummaryLabel->setText(
            QString("当前时间轴：#%1 · 共 %2 次记录，从 %3 到 %4。它最常把你带向“%5”，平均强度 %6，平均能量 %7。")
                .arg(timelineTag)
                .arg(timelineEntries.size())
                .arg(earliestDate.toString("MM-dd"))
                .arg(latestDate.toString("MM-dd"))
                .arg(dominantEmotion)
                .arg(averageIntensity, 0, 'f', 1)
                .arg(averageEnergy, 0, 'f', 1));

        for (const auto &entry : timelineEntries) {
            const QString lineOne =
                QString("%1   ·   %2").arg(entry.date.toString("MM-dd")).arg(entry.emotion);
            const QString lineTwo =
                QString("%1\n%2").arg(entry.title).arg(compactPreviewText(entry.note, 46));
            auto *item = new QListWidgetItem(lineOne + "\n" + lineTwo, m_timelineList);
            item->setData(Qt::UserRole, entry.id);
            item->setForeground(EmotionEntry::colorForEmotion(entry.emotion));
            item->setToolTip(QString("#%1  ·  %2").arg(timelineTag).arg(entry.title));
        }
    }

    refreshCalendarReview();
    m_starMapView->setEntries(visibleEntries);

    if (!m_entries.isEmpty() && (m_selectedEntryId.isEmpty() || !findEntryById(m_selectedEntryId))) {
        m_selectedEntryId = m_entries.front().id;
    }

    bool selectedVisible = m_activeTagFilter.isEmpty();
    if (!selectedVisible) {
        for (const auto &entry : visibleEntries) {
            if (entry.id == m_selectedEntryId) {
                selectedVisible = true;
                break;
            }
        }
    }
    m_starMapView->setSelectedEntryId(selectedVisible ? m_selectedEntryId : QString());

    if (!m_selectedEntryId.isEmpty()) {
        selectEntryById(m_selectedEntryId);
    } else {
        showEntryDetails(nullptr);
    }
}

void MainWindow::selectEntryById(const QString &id) {
    m_selectedEntryId = id;
    int entryRow = -1;
    for (int row = 0; row < m_entryList->count(); ++row) {
        if (m_entryList->item(row)->data(Qt::UserRole).toString() == id) {
            entryRow = row;
            break;
        }
    }

    int timelineRow = -1;
    for (int row = 0; row < m_timelineList->count(); ++row) {
        if (m_timelineList->item(row)->data(Qt::UserRole).toString() == id) {
            timelineRow = row;
            break;
        }
    }

    {
        const QSignalBlocker blocker(m_entryList);
        if (entryRow >= 0) {
            m_entryList->setCurrentRow(entryRow);
        } else {
            m_entryList->clearSelection();
        }
    }
    {
        const QSignalBlocker blocker(m_timelineList);
        if (timelineRow >= 0) {
            m_timelineList->setCurrentRow(timelineRow);
        } else {
            m_timelineList->clearSelection();
        }
    }

    m_starMapView->setSelectedEntryId(id);
    if (const auto *entry = findEntryById(id)) {
        showEntryDetails(entry);
    } else {
        showEntryDetails(nullptr);
    }
}

void MainWindow::showEntryDetails(const EmotionEntry *entry) {
    if (!entry) {
        m_detailIcon->setText("五");
        m_detailTitle->setText("当前没有选中日记");
        m_detailMeta->setText("请从右侧星空中点亮一颗星，或从左侧档案里选择一条记录。");
        m_detailNote->setText("当你点亮一颗星后，这里会出现一句更轻一点的摘要，以及这份情绪的强度和能量走势。");
        return;
    }

    const QString element = elementNameForEmotion(entry->emotion);
    const QString reading = trendReadingForEntry(*entry);
    const QString notePreview = compactPreviewText(entry->note).toHtmlEscaped();

    m_detailIcon->setText(element);
    m_detailTitle->setText(entry->title);
    m_detailMeta->setText(
        QString("%1  |  %2  |  五行：%3\n标签：%4\n走势：%5")
            .arg(entry->date.toString("yyyy-MM-dd"), entry->emotion)
            .arg(element)
            .arg(tagBubbleText(entry->tags))
            .arg(reading));
    m_detailNote->setText(
        QString("<div style='color:#90a5c9;font-size:12px;'>记录片段</div>"
                "<div style='margin-top:6px;color:#f7fbff;'>%1</div>"
                "<div style='margin-top:10px;color:#c7d5ef;font-size:12px;'>"
                "<span style='padding-right:16px;'>强度 %2 / 5</span>"
                "<span>能量 %3 / 5</span>"
                "</div>")
            .arg(notePreview)
            .arg(entry->intensity)
            .arg(entry->energy));
}

void MainWindow::populateEditor(const EmotionEntry &entry) {
    m_ignoreEditorChanges = true;
    m_editingEntryId = entry.id;
    m_titleEdit->setText(entry.title);
    m_emotionCombo->setCurrentText(entry.emotion);
    m_dateEdit->setDate(entry.date);
    m_intensitySlider->setValue(entry.intensity);
    m_energySlider->setValue(entry.energy);
    m_tagEdit->setText(tagsToText(entry.tags));
    m_noteEdit->setPlainText(entry.note);
    m_saveButton->setText("保存修改并返回");
    m_ignoreEditorChanges = false;
    setEditorDirty(false);
}

EmotionEntry *MainWindow::findEntryById(const QString &id) {
    for (auto &entry : m_entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

const EmotionEntry *MainWindow::findEntryById(const QString &id) const {
    for (const auto &entry : m_entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

QList<EmotionEntry> MainWindow::demoEntries() const {
    return {
        {"demo-1", "深夜终于松一口气", "平静",
         "作业的第一版终于写出来了。回寝室的路上风很轻，整个人像被夜色安静地托住了一样。",
         {"学习", "熬夜"}, QDate::currentDate().addDays(-2), 3, 2},
        {"demo-2", "课堂上的高光时刻", "快乐",
         "讨论的时候突然把一个点想通了，脑子里像有几颗本来分散的星一下子连成了线。",
         {"学习", "成就感"}, QDate::currentDate().addDays(-1), 5, 4},
        {"demo-3", "对未来还是有点不安", "焦虑",
         "很多事情一起压过来，但把它们写下来之后，那种模糊的慌张就变得可以面对了。",
         {"目标", "压力"}, QDate::currentDate(), 4, 5},
    };
}

EmotionEntry MainWindow::currentFormEntry() const {
    EmotionEntry entry;
    entry.id = m_editingEntryId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_editingEntryId;
    entry.title = m_titleEdit->text().trimmed();
    entry.emotion = m_emotionCombo->currentText();
    entry.note = m_noteEdit->toPlainText().trimmed();
    entry.tags = normalizeTags(m_tagEdit->text());
    entry.date = m_dateEdit->date();
    entry.intensity = m_intensitySlider->value();
    entry.energy = m_energySlider->value();
    return entry;
}

QString MainWindow::dataFilePath() const {
    const auto baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(baseDir).filePath("entries.json");
}

bool MainWindow::saveCurrentEntry(bool returnToHomeAfterSave) {
    const auto entry = currentFormEntry();
    if (entry.title.isEmpty() || entry.note.isEmpty()) {
        QMessageBox::information(this, "内容还没写完", "保存前请先填写标题和内容。");
        return false;
    }

    if (m_editingEntryId.isEmpty()) {
        m_entries.prepend(entry);
    } else if (auto *stored = findEntryById(m_editingEntryId)) {
        *stored = entry;
    }

    if (!m_repository.save(m_entries)) {
        QMessageBox::warning(this, "保存失败", "这条记录暂时没有成功写入本地文件。");
        return false;
    }

    refreshViews();
    selectEntryById(entry.id);
    clearEditor();

    if (returnToHomeAfterSave) {
        showHomePage();
        m_summaryLabel->setText("这条记录已经好好保存下来了。");
    }
    return true;
}

bool MainWindow::confirmLeaveEditor() {
    if (!m_editorDirty) {
        return true;
    }

    QMessageBox box(this);
    box.setWindowTitle("返回前确认");
    box.setText("这条记录还没有保存。");
    box.setInformativeText("你可以先保存，再返回主页；也可以选择不保存，直接离开。");
    auto *saveButton = box.addButton("保存", QMessageBox::AcceptRole);
    auto *discardButton = box.addButton("不保存", QMessageBox::DestructiveRole);
    box.addButton("取消", QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == saveButton) {
        return saveCurrentEntry(true);
    }
    if (box.clickedButton() == discardButton) {
        clearEditor();
        return true;
    }
    return false;
}

void MainWindow::showHomePage() {
    m_leftStack->setCurrentIndex(0);
}

void MainWindow::showEditorPage() {
    m_leftStack->setCurrentIndex(1);
}

void MainWindow::setEditorDirty(bool dirty) {
    m_editorDirty = dirty;
    if (m_backButton) {
        m_backButton->setText(dirty ? "返回 *" : "返回");
    }
}

void MainWindow::applyEditorPrompt(const QString &title, const QString &note, const QString &emotion) {
    clearEditor();
    m_ignoreEditorChanges = true;
    m_titleEdit->setText(title);
    m_noteEdit->setPlainText(note);
    if (!emotion.isEmpty()) {
        m_emotionCombo->setCurrentText(emotion);
    }
    m_ignoreEditorChanges = false;
    setEditorDirty(true);
    showEditorPage();
    m_noteEdit->setFocus();
    m_noteEdit->moveCursor(QTextCursor::End);
}

QString MainWindow::comfortLineForEmotion(const QString &emotion) const {
    if (emotion == "焦虑") {
        return "你不用一下子把所有事情都解决。先把最吵的那一件写下来，今天就已经在向前了。";
    }
    if (emotion == "难过") {
        return "难过不是退步，它只是说明你真的在认真经历生活。先允许自己把这份感受放在这里。";
    }
    if (emotion == "愤怒") {
        return "生气也说明你在保护自己的边界。先把那一刻写清楚，再决定要不要和它和解。";
    }
    if (emotion == "快乐") {
        return "开心的时候也值得被认真记住。把它写下来，未来你会感谢今天替自己留下证据。";
    }
    if (emotion == "平静") {
        return "平静不是空白，它是你和自己相处得刚刚好的时刻。这样的瞬间也值得留下。";
    }
    return "即使现在还说不清楚，也没关系。先留下一个小小的开头，情绪会慢慢有形状。";
}

QString MainWindow::homeHeadlineForEmotion(const QString &emotion) const {
    if (emotion == "焦虑") {
        return "今天的你，也可以慢一点。";
    }
    if (emotion == "难过") {
        return "今天的你，值得被轻轻抱住。";
    }
    if (emotion == "愤怒") {
        return "今天的你，也在认真守护自己。";
    }
    if (emotion == "快乐") {
        return "今天的你，像一颗慢慢变亮的星。";
    }
    if (emotion == "平静") {
        return "今天的你，和自己待得很好。";
    }
    return "今天的你，心里还有光。";
}

void MainWindow::appendTagToEditor(const QString &tag) {
    QStringList tags = normalizeTags(m_tagEdit->text());
    if (!tags.contains(tag)) {
        tags.append(tag);
    }
    m_tagEdit->setText(tagsToText(tags));
}

void MainWindow::addEntry() {
    saveCurrentEntry(true);
}

void MainWindow::editSelectedEntry() {
    if (const auto *entry = findEntryById(m_selectedEntryId)) {
        populateEditor(*entry);
        showEditorPage();
    }
}

void MainWindow::deleteSelectedEntry() {
    if (m_selectedEntryId.isEmpty()) {
        return;
    }

    if (QMessageBox::question(this, "删除这条记录", "这条记录会从星空档案中移除，确定继续吗？",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    const auto removingId = m_selectedEntryId;
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [&removingId](const EmotionEntry &entry) {
                       return entry.id == removingId;
                   }),
                    m_entries.end());

    m_repository.save(m_entries);

    if (m_editingEntryId == removingId) {
        m_editingEntryId.clear();
    }
    m_selectedEntryId.clear();
    clearEditor();
    refreshViews();
    m_summaryLabel->setText("这条记录已经从我的星空档案里移除了。");
}

void MainWindow::clearEditor() {
    m_ignoreEditorChanges = true;
    m_editingEntryId.clear();
    m_titleEdit->clear();
    m_noteEdit->clear();
    m_tagEdit->clear();
    m_intensitySlider->setValue(3);
    m_energySlider->setValue(3);
    m_emotionCombo->setCurrentIndex(0);
    m_dateEdit->setDate(QDate::currentDate());
    m_saveButton->setText("保存并返回");
    m_ignoreEditorChanges = false;
    setEditorDirty(false);
}

void MainWindow::openNewEntryPage() {
    clearEditor();
    showEditorPage();
}

void MainWindow::handleBackFromEditor() {
    if (!confirmLeaveEditor()) {
        return;
    }
    showHomePage();
}

void MainWindow::viewLatestEntry() {
    if (m_entries.isEmpty()) {
        m_summaryLabel->setText("我还没有留下记录，先写下今天的情绪吧。");
        return;
    }
    selectEntryById(m_entries.front().id);
    m_summaryLabel->setText("已为我定位到最新的一条情绪记录。");
}

void MainWindow::viewTodayEntry() {
    const QDate today = QDate::currentDate();
    for (const auto &entry : m_entries) {
        if (entry.date == today) {
            selectEntryById(entry.id);
            m_summaryLabel->setText("已回到我今天写下的情绪记录。");
            return;
        }
    }
    m_summaryLabel->setText("我今天还没有留下记录。");
}

void MainWindow::viewRandomEntry() {
    if (m_entries.isEmpty()) {
        m_summaryLabel->setText("我的星空档案还是空的。");
        return;
    }
    const int index = QRandomGenerator::global()->bounded(m_entries.size());
    selectEntryById(m_entries.at(index).id);
    m_summaryLabel->setText("随机打开了一颗属于我的情绪星。");
}

void MainWindow::handleListSelectionChanged() {
    auto *item = m_entryList->currentItem();
    if (!item) {
        m_selectedEntryId.clear();
        m_starMapView->setSelectedEntryId(QString());
        showEntryDetails(nullptr);
        return;
    }

    const auto id = item->data(Qt::UserRole).toString();
    m_selectedEntryId = id;
    m_starMapView->setSelectedEntryId(id);
    if (const auto *entry = findEntryById(id)) {
        showEntryDetails(entry);
    }
}

void MainWindow::handleStarSelection(const QString &id) {
    selectEntryById(id);
}
