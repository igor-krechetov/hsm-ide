#ifndef MAINEDITORCONTROLLER_HPP
#define MAINEDITORCONTROLLER_HPP

#include <QObject>
#include <QSharedPointer>

class MainWindow;
class ProjectController;

class MainEditorController : public QObject {
    Q_OBJECT
public:
    explicit MainEditorController(QObject* parent = nullptr);
    int start();

private:
    QSharedPointer<MainWindow> mMainWindow;
    QSharedPointer<ProjectController> mProjectController;
};

#endif  // MAINEDITORCONTROLLER_HPP
