#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QPointer>

#include "model/ModelTypes.hpp"

QT_BEGIN_NAMESPACE
class Ui_hsm_ide;
QT_END_NAMESPACE

class HsmGraphicsView;
class ProjectController;

namespace model {
    class StateMachineModel;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

    QPointer<HsmGraphicsView> view();

    void setProjectController(QPointer<ProjectController> controller);
    void setModel(const QSharedPointer<model::StateMachineModel>& model);

public slots:
    void handleOpen();
    void handleSave();
    void handleSaveAs();

    void deleteSelectedItems();
    void onGraphicsViewSelectionChanged();
    void onModelTreeSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void selectModelEntityById(const model::EntityID_t id);

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

    QPointer<ProjectController> mActiveProject;
};

#endif  // MAINWINDOW_HPP
