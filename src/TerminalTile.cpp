#include "TerminalTile.h"
#include <KPluginFactory>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QStyle>

TerminalTile::TerminalTile(const QString &title, const QString &initialCwd, QWidget *parent)
    : QFrame(parent)
    , m_title(title)
    , m_cwd(initialCwd.isEmpty() ? QDir::homePath() : initialCwd)
{
    setupUi();
    createKonsolePart();
}

TerminalTile::~TerminalTile()
{
    if (m_part) {
        delete m_part;
        m_part = nullptr;
    }
}

void TerminalTile::setupUi()
{
    setObjectName(QStringLiteral("terminalTile"));
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Header bar
    m_headerWidget = new QWidget(this);
    m_headerWidget->setObjectName(QStringLiteral("tileHeader"));
    m_headerWidget->setFixedHeight(32);

    auto *headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(8, 0, 6, 0);
    headerLayout->setSpacing(6);

    // Status dot
    m_statusDot = new QLabel(m_headerWidget);
    m_statusDot->setFixedSize(8, 8);
    m_statusDot->setStyleSheet(QStringLiteral("background-color: #3fb950; border-radius: 4px;"));

    // Title label & inline editor
    m_titleLabel = new QLabel(m_title, m_headerWidget);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 12px; color: #e6edf3;"));
    m_titleLabel->setCursor(Qt::PointingHandCursor);
    m_titleLabel->setToolTip(QStringLiteral("Click để đổi tên Agent"));

    m_titleEdit = new QLineEdit(m_title, m_headerWidget);
    m_titleEdit->setStyleSheet(QStringLiteral(
        "background-color: #0d1117; color: #58a6ff; font-weight: bold; font-size: 12px; "
        "border: 1px solid #388bfd; border-radius: 3px; padding: 1px 4px;"
    ));
    m_titleEdit->hide();

    // Working directory label
    m_cwdLabel = new QLabel(m_cwd, m_headerWidget);
    m_cwdLabel->setStyleSheet(QStringLiteral("color: #8b949e; font-size: 11px;"));
    m_cwdLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Zoom button
    m_zoomBtn = new QToolButton(m_headerWidget);
    m_zoomBtn->setText(QStringLiteral("⛶"));
    m_zoomBtn->setToolTip(QStringLiteral("Phóng to / Thu nhỏ (Alt+Z)"));
    m_zoomBtn->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: none; color: #8b949e; font-size: 13px; font-weight: bold; border-radius: 4px; padding: 2px 6px; }"
        "QToolButton:hover { background: rgba(255, 255, 255, 0.1); color: #fff; }"
    ));

    // Clear buffer button
    m_clearBtn = new QToolButton(m_headerWidget);
    m_clearBtn->setText(QStringLiteral("🧹"));
    m_clearBtn->setToolTip(QStringLiteral("Xóa màn hình (Clear)"));
    m_clearBtn->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: none; color: #8b949e; font-size: 11px; border-radius: 4px; padding: 2px 4px; }"
        "QToolButton:hover { background: rgba(255, 255, 255, 0.1); }"
    ));

    // Close button
    m_closeBtn = new QToolButton(m_headerWidget);
    m_closeBtn->setText(QStringLiteral("✕"));
    m_closeBtn->setToolTip(QStringLiteral("Đóng ô terminal (Alt+W)"));
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: none; color: #8b949e; font-size: 12px; font-weight: bold; border-radius: 4px; padding: 2px 6px; }"
        "QToolButton:hover { background: rgba(218, 54, 51, 0.25); color: #f85149; }"
    ));

    headerLayout->addWidget(m_statusDot);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_titleEdit);
    headerLayout->addWidget(m_cwdLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_zoomBtn);
    headerLayout->addWidget(m_clearBtn);
    headerLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(m_headerWidget);

    // Event filter for title click & focus
    m_titleLabel->installEventFilter(this);
    m_headerWidget->installEventFilter(this);

    connect(m_titleEdit, &QLineEdit::editingFinished, this, &TerminalTile::onTitleEdited);
    connect(m_zoomBtn, &QToolButton::clicked, this, [this]() { emit requestZoom(this); });
    connect(m_closeBtn, &QToolButton::clicked, this, [this]() { emit requestClose(this); });
    connect(m_clearBtn, &QToolButton::clicked, this, [this]() {
        sendText(QStringLiteral("clear\n"));
    });
}

void TerminalTile::createKonsolePart()
{
    auto result = KPluginFactory::instantiatePlugin<KParts::ReadOnlyPart>(
        KPluginMetaData(QStringLiteral("kf6/parts/konsolepart")), this
    );

    if (!result.plugin) {
        auto *errLabel = new QLabel(
            QStringLiteral("Không thể tải thành phần terminal KonsolePart: ") + result.errorString, this
        );
        errLabel->setStyleSheet(QStringLiteral("color: #f85149; padding: 16px;"));
        layout()->addWidget(errLabel);
        return;
    }

    m_part = result.plugin;

    // Connect KonsolePart signals
    connect(m_part, SIGNAL(currentDirectoryChanged(QString)), this, SLOT(onDirectoryChanged(QString)));
    connect(m_part, &QObject::destroyed, this, &TerminalTile::onTerminalExited);

    // Create terminal session at cwd
    QMetaObject::invokeMethod(
        m_part, "createSession",
        Q_ARG(QString, QString()),
        Q_ARG(QString, m_cwd)
    );

    m_partWidget = m_part->widget();
    if (m_partWidget) {
        layout()->addWidget(m_partWidget);
        m_partWidget->installEventFilter(this);

        // Find internal terminal display and track focus
        for (QWidget *child : m_partWidget->findChildren<QWidget*>()) {
            child->installEventFilter(this);
        }
    }
}

void TerminalTile::setTitle(const QString &title)
{
    m_title = title;
    m_titleLabel->setText(m_title);
    m_titleEdit->setText(m_title);
}

void TerminalTile::setZoomed(bool zoomed)
{
    m_isZoomed = zoomed;
    m_zoomBtn->setText(zoomed ? QStringLiteral("❐") : QStringLiteral("⛶"));
    m_zoomBtn->setToolTip(zoomed ? QStringLiteral("Thu nhỏ về lưới (Esc)") : QStringLiteral("Phóng to ô này (Alt+Z)"));
}

void TerminalTile::onTitleClicked()
{
    m_titleLabel->hide();
    m_titleEdit->show();
    m_titleEdit->setFocus();
    m_titleEdit->selectAll();
}

void TerminalTile::onTitleEdited()
{
    const QString newTitle = m_titleEdit->text().trimmed();
    if (!newTitle.isEmpty()) {
        m_title = newTitle;
        m_titleLabel->setText(m_title);
    }
    m_titleEdit->hide();
    m_titleLabel->show();
    focusTerminal();
}

void TerminalTile::onDirectoryChanged(const QString &dir)
{
    m_cwd = dir;
    m_cwdLabel->setText(dir);
    m_cwdLabel->setToolTip(dir);
}

void TerminalTile::onTerminalExited()
{
    m_statusDot->setStyleSheet(QStringLiteral("background-color: #f85149; border-radius: 4px;"));
}

QWidget* TerminalTile::terminalDisplayWidget() const
{
    if (!m_partWidget) return nullptr;
    for (QWidget *child : m_partWidget->findChildren<QWidget*>()) {
        if (child->metaObject()->className() == QStringLiteral("Konsole::TerminalDisplay")) {
            return child;
        }
    }
    return m_partWidget;
}

void TerminalTile::focusTerminal()
{
    QWidget *w = terminalDisplayWidget();
    if (w) {
        w->setFocus();
    }
}

void TerminalTile::sendKeyEvent(QKeyEvent *event)
{
    QWidget *w = terminalDisplayWidget();
    if (w) {
        auto *copy = new QKeyEvent(
            event->type(),
            event->key(),
            event->modifiers(),
            event->text(),
            event->isAutoRepeat(),
            event->count()
        );
        QCoreApplication::postEvent(w, copy);
    }
}

void TerminalTile::sendText(const QString &text)
{
    for (const QChar &ch : text) {
        int key = ch.unicode();
        if (ch == QLatin1Char('\n') || ch == QLatin1Char('\r')) {
            key = Qt::Key_Return;
        }
        auto *press = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier, QString(ch));
        auto *release = new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier, QString(ch));
        QWidget *w = terminalDisplayWidget();
        if (w) {
            QCoreApplication::postEvent(w, press);
            QCoreApplication::postEvent(w, release);
        }
    }
}

bool TerminalTile::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_titleLabel && event->type() == QEvent::MouseButtonPress) {
        onTitleClicked();
        return true;
    }

    if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress) {
        emit focused(this);
    }

    return QFrame::eventFilter(watched, event);
}
