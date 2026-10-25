#include "MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QShortcut>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QIcon>
#include <QStyle>
#include <QMouseEvent>
#include <QSizeGrip>
#include <QWindow>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlag(Qt::FramelessWindowHint);
    setupUi();
    setupShortcuts();

    // Start with 1 terminal pane at Home directory
    addNewTerminal();

    qApp->installEventFilter(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("ManyTerminals - Multi-Terminal Manager"));
    resize(1280, 800);

    // Apply dark theme stylesheet
    setStyleSheet(QStringLiteral(
        "QMainWindow { background-color: #0f1117; color: #e6edf3; }"
        "QWidget#topBar { background-color: #171922; border-bottom: 1px solid #272c3d; }"
        "QWidget#tileHeader { background-color: #141720; border-bottom: 1px solid #272c3d; }"
        "QFrame#terminalTile { background-color: #1a1d27; border: 1px solid #272c3d; border-radius: 6px; }"
        "QFrame#terminalTile[focused=\"true\"] { border: 1px solid #388bfd; }"
        "QFrame#terminalTile[broadcast=\"true\"] { border: 1px solid #d29922; }"
    ));

    auto *rootWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(rootWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Top Toolbar
    auto *topBar = new QWidget(rootWidget);
    m_topBar = topBar;
    topBar->setObjectName(QStringLiteral("topBar"));
    topBar->setFixedHeight(46);

    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(12, 0, 12, 0);
    topLayout->setSpacing(10);

    // Brand / Logo
    auto *brandLabel = new QLabel(QStringLiteral("⚡ ManyTerminals"), topBar);
    brandLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    brandLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 14px; color: #58a6ff;"));
    topLayout->addWidget(brandLabel);

    topLayout->addSpacing(16);

    // Layout Mode Segmented Buttons
    m_layoutGroup = new QButtonGroup(this);
    auto *layoutContainer = new QWidget(topBar);
    auto *layoutBox = new QHBoxLayout(layoutContainer);
    layoutBox->setContentsMargins(0, 0, 0, 0);
    layoutBox->setSpacing(2);

    auto createSegBtn = [this, layoutBox](const QString &text, LayoutMode mode, bool checked = false) {
        auto *btn = new QPushButton(text, this);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background: #0f1117; color: #8b949e; border: 1px solid #272c3d; padding: 4px 10px; font-size: 11px; font-weight: 600; border-radius: 4px; }"
            "QPushButton:checked { background: #21262d; color: #fff; border-color: #388bfd; }"
            "QPushButton:hover { color: #e6edf3; }"
        ));
        m_layoutGroup->addButton(btn);
        layoutBox->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, mode]() { setLayoutMode(mode); });
    };

    createSegBtn(QStringLiteral("Auto"), LayoutMode::Auto, true);
    createSegBtn(QStringLiteral("1 Cột"), LayoutMode::Col1);
    createSegBtn(QStringLiteral("2 Cột"), LayoutMode::Col2);
    createSegBtn(QStringLiteral("3 Cột"), LayoutMode::Col3);
    createSegBtn(QStringLiteral("4 Cột"), LayoutMode::Col4);

    topLayout->addWidget(layoutContainer);

    topLayout->addSpacing(10);

    // Broadcast Mode Toggle Button
    m_broadcastBtn = new QPushButton(QStringLiteral("📡 Broadcast"), topBar);
    m_broadcastBtn->setCheckable(true);
    m_broadcastBtn->setToolTip(QStringLiteral("Phát phím đồng loạt đến tất cả terminal (Alt+B)"));
    m_broadcastBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1a1d27; color: #8b949e; border: 1px solid #272c3d; padding: 5px 12px; font-size: 12px; font-weight: 600; border-radius: 5px; }"
        "QPushButton:hover { background: #252a38; color: #e6edf3; }"
        "QPushButton:checked { background: rgba(210, 153, 34, 0.2); color: #e3b341; border: 1px solid #d29922; }"
    ));
    connect(m_broadcastBtn, &QPushButton::toggled, this, &MainWindow::toggleBroadcast);
    topLayout->addWidget(m_broadcastBtn);

    topLayout->addStretch(1);

    // Counter Badge
    m_counterLabel = new QLabel(QStringLiteral("1 Terminal"), topBar);
    m_counterLabel->setStyleSheet(QStringLiteral(
        "background: #0f1117; color: #8b949e; border: 1px solid #272c3d; padding: 3px 8px; border-radius: 5px; font-size: 11px; font-weight: 500;"
    ));
    topLayout->addWidget(m_counterLabel);

    // Add Terminal Button
    auto *addBtn = new QPushButton(QStringLiteral("+ New Terminal"), topBar);
    addBtn->setToolTip(QStringLiteral("Thêm terminal mới (Alt+N)"));
    addBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1f6feb; color: #ffffff; border: none; padding: 6px 14px; font-size: 12px; font-weight: bold; border-radius: 5px; }"
        "QPushButton:hover { background: #388bfd; }"
    ));
    connect(addBtn, &QPushButton::clicked, this, [this]() { addNewTerminal(); });
    topLayout->addWidget(addBtn);

    // Keep window controls in the existing toolbar, without a second title row.
    auto *windowControls = new QWidget(topBar);
    auto *controlsLayout = new QHBoxLayout(windowControls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(0);
    windowControls->setStyleSheet(QStringLiteral(
        "QToolButton { border: none; background: transparent; border-radius: 4px; color: #e6edf3; font-size: 18px; }"
        "QToolButton:hover { background: #303744; }"
        "QToolButton#windowClose:hover { background: #da3633; }"
    ));

    auto *minimizeBtn = new QToolButton(windowControls);
    minimizeBtn->setObjectName(QStringLiteral("windowMinimize"));
    minimizeBtn->setText(QStringLiteral("−"));
    minimizeBtn->setToolTip(QStringLiteral("Thu nhỏ cửa sổ"));
    minimizeBtn->setAccessibleName(minimizeBtn->toolTip());
    minimizeBtn->setFixedSize(36, 32);
    connect(minimizeBtn, &QToolButton::clicked, this, &QWidget::showMinimized);
    controlsLayout->addWidget(minimizeBtn);

    m_maximizeBtn = new QToolButton(windowControls);
    m_maximizeBtn->setObjectName(QStringLiteral("windowMaximize"));
    m_maximizeBtn->setText(QStringLiteral("□"));
    m_maximizeBtn->setToolTip(QStringLiteral("Phóng to cửa sổ"));
    m_maximizeBtn->setAccessibleName(m_maximizeBtn->toolTip());
    m_maximizeBtn->setFixedSize(36, 32);
    connect(m_maximizeBtn, &QToolButton::clicked, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    controlsLayout->addWidget(m_maximizeBtn);

    auto *closeBtn = new QToolButton(windowControls);
    closeBtn->setObjectName(QStringLiteral("windowClose"));
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setToolTip(QStringLiteral("Đóng cửa sổ"));
    closeBtn->setAccessibleName(closeBtn->toolTip());
    closeBtn->setFixedSize(36, 32);
    connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
    controlsLayout->addWidget(closeBtn);
    topLayout->addWidget(windowControls);

    rootLayout->addWidget(topBar);

    // Broadcast Warning Banner
    m_broadcastBanner = new QWidget(rootWidget);
    m_broadcastBanner->setFixedHeight(28);
    m_broadcastBanner->setStyleSheet(QStringLiteral("background-color: #3c2a05; border-bottom: 1px solid #d29922; color: #fce8a4; font-size: 11px;"));
    auto *bannerLayout = new QHBoxLayout(m_broadcastBanner);
    bannerLayout->setContentsMargins(12, 0, 12, 0);
    auto *bannerText = new QLabel(QStringLiteral("⚠️ CHẾ ĐỘ BROADCAST ĐANG BẬT: Mọi phím bạn gõ sẽ được gửi đồng thời đến tất cả các terminal!"), m_broadcastBanner);
    bannerText->setStyleSheet(QStringLiteral("color: #fce8a4; font-weight: bold;"));
    bannerLayout->addWidget(bannerText);
    bannerLayout->addStretch(1);
    auto *turnOffBtn = new QPushButton(QStringLiteral("Tắt Broadcast (Alt+B)"), m_broadcastBanner);
    turnOffBtn->setStyleSheet(QStringLiteral("background: #583d09; border: 1px solid #d29922; color: #fff; font-size: 10px; padding: 2px 6px; border-radius: 3px;"));
    connect(turnOffBtn, &QPushButton::clicked, this, [this]() { m_broadcastBtn->setChecked(false); });
    bannerLayout->addWidget(turnOffBtn);
    m_broadcastBanner->hide();

    rootLayout->addWidget(m_broadcastBanner);

    // Central Grid Container
    m_centralContainer = new QWidget(rootWidget);
    m_gridLayout = new QGridLayout(m_centralContainer);
    m_gridLayout->setContentsMargins(6, 6, 6, 6);
    m_gridLayout->setSpacing(6);

    rootLayout->addWidget(m_centralContainer, 1);
    setCentralWidget(rootWidget);

    // Qt supplies the floating-window resize interaction on Wayland and X11.
    m_resizeGrip = new QSizeGrip(this);
    m_resizeGrip->setObjectName(QStringLiteral("windowResizeGrip"));
    m_resizeGrip->resize(m_resizeGrip->sizeHint());
}

void MainWindow::setupShortcuts()
{
    // Alt + N: New terminal
    auto *newTermShortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+N")), this);
    connect(newTermShortcut, &QShortcut::activated, this, [this]() { addNewTerminal(); });

    // Alt + Z: Toggle zoom for focused tile
    auto *zoomShortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+Z")), this);
    connect(zoomShortcut, &QShortcut::activated, this, [this]() {
        if (m_focusedTile) toggleZoom(m_focusedTile);
    });

    // Alt + B: Toggle broadcast mode
    auto *broadcastShortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+B")), this);
    connect(broadcastShortcut, &QShortcut::activated, this, [this]() {
        m_broadcastBtn->setChecked(!m_broadcastBtn->isChecked());
    });

    // Alt + W: Close focused tile
    auto *closeShortcut = new QShortcut(QKeySequence(QStringLiteral("Alt+W")), this);
    connect(closeShortcut, &QShortcut::activated, this, [this]() {
        if (m_focusedTile) closeTerminal(m_focusedTile);
    });

    // Escape: Unzoom if zoomed
    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (m_zoomedTile) toggleZoom(m_zoomedTile);
    });
}

void MainWindow::addNewTerminal(const QString &title)
{
    const QString termTitle = title.isEmpty() ? QStringLiteral("Agent %1").arg(m_termCounter++) : title;
    auto *tile = new TerminalTile(termTitle, QDir::homePath(), m_centralContainer);

    connect(tile, &TerminalTile::requestClose, this, &MainWindow::closeTerminal);
    connect(tile, &TerminalTile::requestZoom, this, &MainWindow::toggleZoom);
    connect(tile, &TerminalTile::focused, this, &MainWindow::onTileFocused);

    m_tiles.append(tile);

    // If another tile was zoomed, restore grid when new terminal is added
    if (m_zoomedTile) {
        toggleZoom(m_zoomedTile);
    } else {
        relayoutGrid();
    }

    onTileFocused(tile);
    updateCounter();
}

void MainWindow::closeTerminal(TerminalTile *tile)
{
    if (!tile) return;

    if (m_zoomedTile == tile) {
        m_zoomedTile = nullptr;
    }

    m_tiles.removeOne(tile);
    tile->deleteLater();

    if (m_focusedTile == tile) {
        m_focusedTile = m_tiles.isEmpty() ? nullptr : m_tiles.last();
    }

    if (m_zoomedTile) {
        // Keep zoomed tile full screen
    } else {
        relayoutGrid();
    }

    if (m_focusedTile) {
        onTileFocused(m_focusedTile);
    }

    updateCounter();
}

void MainWindow::toggleZoom(TerminalTile *tile)
{
    if (!tile) return;

    if (m_zoomedTile == tile) {
        // Unzoom
        m_zoomedTile = nullptr;
        tile->setZoomed(false);
        for (auto *t : m_tiles) {
            t->show();
        }
        relayoutGrid();
    } else {
        // Zoom this tile
        if (m_zoomedTile) {
            m_zoomedTile->setZoomed(false);
        }
        m_zoomedTile = tile;
        tile->setZoomed(true);

        for (auto *t : m_tiles) {
            if (t != tile) {
                t->hide();
            }
        }

        // Put tile in single full-screen cell
        while (m_gridLayout->count() > 0) {
            m_gridLayout->takeAt(0);
        }
        m_gridLayout->addWidget(tile, 0, 0);
        tile->show();
    }

    tile->focusTerminal();
}

void MainWindow::onTileFocused(TerminalTile *tile)
{
    m_focusedTile = tile;
    updateTileStyles();
}

void MainWindow::toggleBroadcast(bool checked)
{
    m_broadcastMode = checked;
    m_broadcastBanner->setVisible(checked);
    updateTileStyles();
}

void MainWindow::setLayoutMode(LayoutMode mode)
{
    m_layoutMode = mode;
    if (m_zoomedTile) {
        toggleZoom(m_zoomedTile);
    } else {
        relayoutGrid();
    }
}

void MainWindow::relayoutGrid()
{
    if (m_zoomedTile) return;

    // Clear current layout
    while (m_gridLayout->count() > 0) {
        m_gridLayout->takeAt(0);
    }

    const int count = m_tiles.size();
    if (count == 0) return;

    int cols = 1;
    switch (m_layoutMode) {
        case LayoutMode::Col1: cols = 1; break;
        case LayoutMode::Col2: cols = 2; break;
        case LayoutMode::Col3: cols = 3; break;
        case LayoutMode::Col4: cols = 4; break;
        case LayoutMode::Auto: {
            if (count == 1) cols = 1;
            else if (count == 2) cols = 2;
            else if (count <= 4) cols = 2;
            else if (count <= 6) cols = 3;
            else if (count <= 9) cols = 3;
            else cols = 4;
            break;
        }
    }

    for (int i = 0; i < count; ++i) {
        int r = i / cols;
        int c = i % cols;
        m_tiles[i]->show();
        m_gridLayout->addWidget(m_tiles[i], r, c);
    }
}

void MainWindow::updateTileStyles()
{
    for (auto *tile : m_tiles) {
        const bool isFocused = (tile == m_focusedTile);
        tile->setProperty("focused", isFocused);
        tile->setProperty("broadcast", m_broadcastMode);
        tile->style()->unpolish(tile);
        tile->style()->polish(tile);
    }
}

void MainWindow::updateCounter()
{
    const int count = m_tiles.size();
    m_counterLabel->setText(count == 1 ? QStringLiteral("1 Terminal") : QStringLiteral("%1 Terminals").arg(count));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_topBar) {
        if (event->type() == QEvent::MouseButtonDblClick
            && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
            m_maximizeBtn->click();
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress
            && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton
            && windowHandle()) {
            return windowHandle()->startSystemMove();
        }
    }

    if (!m_broadcastMode) {
        return QMainWindow::eventFilter(watched, event);
    }

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        static bool replicating = false;
        if (replicating) {
            return false;
        }

        auto *keyEvent = static_cast<QKeyEvent*>(event);

        // Check if event belongs to any terminal display widget
        TerminalTile *senderTile = nullptr;
        for (auto *tile : m_tiles) {
            QWidget *disp = tile->terminalDisplayWidget();
            if (disp && (watched == disp || disp->isAncestorOf(qobject_cast<QWidget*>(watched)))) {
                senderTile = tile;
                break;
            }
        }

        if (senderTile) {
            replicating = true;
            for (auto *tile : m_tiles) {
                if (tile != senderTile) {
                    tile->sendKeyEvent(keyEvent);
                }
            }
            replicating = false;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_maximizeBtn) {
        m_maximizeBtn->setText(isMaximized() ? QStringLiteral("❐") : QStringLiteral("□"));
        m_maximizeBtn->setToolTip(isMaximized()
            ? QStringLiteral("Khôi phục cửa sổ") : QStringLiteral("Phóng to cửa sổ"));
        m_maximizeBtn->setAccessibleName(m_maximizeBtn->toolTip());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_resizeGrip) {
        m_resizeGrip->move(width() - m_resizeGrip->width(), height() - m_resizeGrip->height());
        m_resizeGrip->raise();
    }
}
