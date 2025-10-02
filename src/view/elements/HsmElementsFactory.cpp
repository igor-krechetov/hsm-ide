#include "HsmElementsFactory.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QListWidgetItem>
#include <functional>

#include "HsmEntryPointElement.hpp"
#include "HsmExitPointElement.hpp"
#include "HsmFinalElement.hpp"
#include "HsmHistoryElement.hpp"
#include "HsmInitialElement.hpp"
#include "HsmStateElement.hpp"
#include "private/HsmElement.hpp"

namespace view {

std::map<QString, std::tuple<QString, QString, std::function<HsmElement*(const QSharedPointer<model::StateMachineEntity>&)>>>
    HsmElementsFactory::mItemsCatalog = {
        {"initial", {"initial", ":/icons/element_start.png", &HsmElementsFactory::createElementStart}},
        {"final", {"Final", ":/icons/element_final.png", &HsmElementsFactory::createElementFinal}},
        {"state", {"State", ":/icons/element_state.png", &HsmElementsFactory::createElementState}},
        {"entrypoint", {"Entry Point", ":/icons/element_entrypoint.png", &HsmElementsFactory::createElementEntryPoint}},
        {"exitpoint", {"Exit Point", ":/icons/element_exitpoint.png", &HsmElementsFactory::createElementExitPoint}},
        {"history", {"History", ":/icons/element_history.png", &HsmElementsFactory::createElementHistory}}};

std::list<QListWidgetItem*> HsmElementsFactory::createElementsList() {
    std::list<QListWidgetItem*> elements;
    for (const auto& itemInfo : mItemsCatalog) {
        QIcon icon(std::get<1>(itemInfo.second));
        QString itemName = std::get<0>(itemInfo.second);
        QListWidgetItem* newItem = new QListWidgetItem(icon, itemName);

        newItem->setData(Qt::UserRole, itemInfo.first);
        elements.push_back(newItem);
    }

    return elements;
}

HsmElement* HsmElementsFactory::createElement(const QString& typeId, const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* element = nullptr;
    auto itItem = mItemsCatalog.find(typeId);

    if (mItemsCatalog.end() != itItem) {
        const auto& callback = std::get<2>(itItem->second);

        element = callback(modelElement);
        // element->setModelId(modelElement);
    }

    return element;
}

HsmElement* HsmElementsFactory::createElementState(const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* elem = new HsmStateElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementStart(const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* elem = new HsmInitialElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementFinal(const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* elem = new HsmFinalElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementEntryPoint(const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* elem = new HsmEntryPointElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementExitPoint(const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* elem = new HsmExitPointElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementHistory(const QSharedPointer<model::StateMachineEntity>& modelElement) {
    HsmElement* elem = new HsmHistoryElement();

    elem->init(modelElement);
    return elem;
}

};  // namespace view