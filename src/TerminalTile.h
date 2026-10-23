#pragma once

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <KParts/ReadOnlyPart>

class TerminalTile : public QFrame {
    Q_OBJECT

public:
    explicit TerminalTile(const QString &title, const QString &initialCwd, QWidget *parent = nullptr);
    ~TerminalTile() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString cwd() const { return m_cwd; }
    bool isZoomed() const { return m_isZoomed; }
    void setZoomed(bool zoomed);

    void focusTerminal();
    void sendKeyEvent(QKeyEvent *event);
    void sendText(const QString &text);
    QWidget* terminalDisplayWidget() const;

signals:
    void requestClose(TerminalTile *tile);
    void requestZoom(TerminalTile *tile);
    void focused(TerminalTile *tile);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onTitleClicked();
    void onTitleEdited();
    void onDirectoryChanged(const QString &dir);
    void onTerminalExited();

private:
    void setupUi();
    void createKonsolePart();

    QString m_title;
    QString m_cwd;
    bool m_isZoomed{false};

    // UI elements
    QWidget *m_headerWidget{nullptr};
    QLabel *m_statusDot{nullptr};
    QLabel *m_titleLabel{nullptr};
    QLineEdit *m_titleEdit{nullptr};
    QLabel *m_cwdLabel{nullptr};
    QToolButton *m_zoomBtn{nullptr};
    QToolButton *m_clearBtn{nullptr};
    QToolButton *m_closeBtn{nullptr};

    KParts::ReadOnlyPart *m_part{nullptr};
    QWidget *m_partWidget{nullptr};
};
