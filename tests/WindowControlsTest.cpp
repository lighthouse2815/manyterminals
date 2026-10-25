#include "MainWindow.h"
#include <QSizeGrip>
#include <QTest>
#include <QToolButton>

class WindowControlsTest : public QObject {
    Q_OBJECT

private slots:
    void controlsPreserveWindowActions()
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));

        auto *minimize = window.findChild<QToolButton*>("windowMinimize");
        auto *maximize = window.findChild<QToolButton*>("windowMaximize");
        auto *close = window.findChild<QToolButton*>("windowClose");
        QVERIFY(minimize && maximize && close);
        QVERIFY(minimize->isVisible() && maximize->isVisible() && close->isVisible());
        QCOMPARE(minimize->parentWidget(), maximize->parentWidget());
        QCOMPARE(maximize->parentWidget(), close->parentWidget());
        QVERIFY(!minimize->text().isEmpty());
        QVERIFY(!maximize->text().isEmpty());
        QVERIFY(!close->text().isEmpty());

        QTest::mouseClick(maximize, Qt::LeftButton);
        QTRY_VERIFY(window.isMaximized());
        QCOMPARE(maximize->toolTip(), QStringLiteral("Khôi phục cửa sổ"));
        QTest::mouseClick(maximize, Qt::LeftButton);
        QTRY_VERIFY(!window.isMaximized());
        QCOMPARE(maximize->toolTip(), QStringLiteral("Phóng to cửa sổ"));

        QTest::mouseClick(minimize, Qt::LeftButton);
        QTRY_VERIFY(window.isMinimized());
        window.showNormal();
        QTRY_VERIFY(!window.isMinimized());

        QTest::mouseClick(close, Qt::LeftButton);
        QTRY_VERIFY(!window.isVisible());
    }

    void toolbarDoubleClickAndFloatingResize()
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *toolbar = window.findChild<QWidget*>("topBar");
        QVERIFY(toolbar);
        QTest::mouseDClick(toolbar, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        QTRY_VERIFY(window.isMaximized());
        QTest::mouseDClick(toolbar, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        QTRY_VERIFY(!window.isMaximized());

        window.resize(1500, 900);
        auto *grip = window.findChild<QSizeGrip*>("windowResizeGrip");
        QVERIFY(grip);
        QTRY_COMPARE(grip->geometry().bottomRight(), window.rect().bottomRight());
    }
};

QTEST_MAIN(WindowControlsTest)
#include "WindowControlsTest.moc"
