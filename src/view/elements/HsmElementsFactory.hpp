#ifndef HSMELEMENTSFACTORY_HPP
#define HSMELEMENTSFACTORY_HPP

#include <QSize>
#include <QString>
// TODO: use Qt types
#include <list>
#include <map>
#include <tuple>

#include "model/ModelTypes.hpp"
#include "private/HsmElement.hpp"

class QListWidgetItem;

namespace model {
class StateMachineEntity;
}

namespace view {

class HsmElementsFactory {
public:
    static std::list<QListWidgetItem*> createElementsList();
    static QString getElementIcon(const QString& typeId);
    static QString getStateIcon(const model::StateType type);
    static HsmElement* createElement(const QString& typeId, const QSharedPointer<model::StateMachineEntity>& modelElement);
    static HsmElement* createElement(const QString& typeId,
                                     const QSharedPointer<model::StateMachineEntity>& modelElement,
                                     const QSizeF& size);

private:
    static HsmElement* createElementState(const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);
    static HsmElement* createElementStart(const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);
    static HsmElement* createElementFinal(const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);
    static HsmElement* createElementEntryPoint(const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);
    static HsmElement* createElementExitPoint(const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);
    static HsmElement* createElementHistory(const QSharedPointer<model::StateMachineEntity>& modelElement, const QSizeF& size);

private:
    static std::map<QString,
                    std::tuple<QString, QString, std::function<HsmElement*(const QSharedPointer<model::StateMachineEntity>&, const QSizeF&)>>>
        mItemsCatalog;
};

};  // namespace view

#endif  // HSMELEMENTSFACTORY_HPP
