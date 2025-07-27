#ifndef STATEMACHINEMODEL_HPP
#define STATEMACHINEMODEL_HPP

#include <QObject>
#include <QSharedPointer>
#include <string>
#include <vector>

class StateMachineElement;

class StateMachineModel : public QObject {
    Q_OBJECT
public:
    explicit StateMachineModel(const std::string& name, QObject* parent = nullptr);
    virtual ~StateMachineModel();

    const std::string& name() const;
    void addChild(QSharedPointer<StateMachineElement> child);
    const std::vector<QSharedPointer<StateMachineElement>>& children() const;

private:
    std::string mName;
    std::vector<QSharedPointer<StateMachineElement>> mChildren;
};

#endif  // STATEMACHINEMODEL_HPP
