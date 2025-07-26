#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class Ui_hsm_ide;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui_hsm_ide *ui;
    // QSettingsDialog mSettings;
    // HsmGraphicsView* mMainView;
    // HsmElementsList* mListHsmElements;
    // QLabel* mStatusBarFrame;
    // QLabel* mStatusBarLog;
    // QWidget* mWindow;
    QString mLastDirectory;
    QString mAppTitle;
    QString mConfigPath;

public slots:
    void deleteSelectedItems();
};
#endif // MAINWINDOW_H
