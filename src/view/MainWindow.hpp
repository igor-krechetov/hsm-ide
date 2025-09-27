#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QPointer>

QT_BEGIN_NAMESPACE
class Ui_hsm_ide;
QT_END_NAMESPACE

class HsmGraphicsView;

namespace model {
    class StateMachineModel;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

    QPointer<HsmGraphicsView> view();

    void setModel(const QSharedPointer<model::StateMachineModel>& model);

private:
    Ui_hsm_ide* ui;
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

#endif  // MAINWINDOW_HPP
