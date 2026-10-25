#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include "TerminalTile.h"

class QSizeGrip;

enum class LayoutMode {
    Auto,
    Col1,
    Col2,
    Col3,
    Col4
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void addNewTerminal(const QString &title = QString());
    void closeTerminal(TerminalTile *tile);
    void toggleZoom(TerminalTile *tile);
    void onTileFocused(TerminalTile *tile);
    void toggleBroadcast(bool checked);
    void setLayoutMode(LayoutMode mode);

private:
    void setupUi();
    void setupShortcuts();
    void relayoutGrid();
    void updateTileStyles();
    void updateCounter();

    QList<TerminalTile*> m_tiles;
    TerminalTile *m_focusedTile{nullptr};
    TerminalTile *m_zoomedTile{nullptr};
    LayoutMode m_layoutMode{LayoutMode::Auto};
    bool m_broadcastMode{false};
    int m_termCounter{1};

    // UI Controls
    QWidget *m_centralContainer{nullptr};
    QGridLayout *m_gridLayout{nullptr};
    QWidget *m_broadcastBanner{nullptr};
    QPushButton *m_broadcastBtn{nullptr};
    QLabel *m_counterLabel{nullptr};
    QButtonGroup *m_layoutGroup{nullptr};
    QWidget *m_topBar{nullptr};
    QToolButton *m_maximizeBtn{nullptr};
    QSizeGrip *m_resizeGrip{nullptr};
};
