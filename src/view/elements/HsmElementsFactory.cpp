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

std::map<QString, std::tuple<QString, QString, std::function<HsmElement*(model::EntityID_t)>>>
    HsmElementsFactory::mItemsCatalog = {
        {"initial", {"initial", "/../../res/element_start.png", &HsmElementsFactory::_createElementStart}},
        {"final", {"Final", "/../../res/element_final.png", &HsmElementsFactory::_createElementFinal}},
        {"state", {"State", "/../../res/element_state.png", &HsmElementsFactory::_createElementState}},
        {"entrypoint", {"Entry Point", "/../../res/element_entrypoint.png", &HsmElementsFactory::_createElementEntryPoint}},
        {"exitpoint", {"Exit Point", "/../../res/element_exitpoint.png", &HsmElementsFactory::_createElementExitPoint}},
        {"history", {"History", "/../../res/element_history.png", &HsmElementsFactory::_createElementHistory}}};

std::list<QListWidgetItem*> HsmElementsFactory::createElementsList() {
    std::list<QListWidgetItem*> elements;
    QString appDir = QCoreApplication::applicationDirPath();

    for (const auto& itemInfo : mItemsCatalog) {
        QIcon icon(appDir + std::get<1>(itemInfo.second));
        QString itemName = std::get<0>(itemInfo.second);
        QListWidgetItem* newItem = new QListWidgetItem(icon, itemName);

        newItem->setData(Qt::UserRole, itemInfo.first);
        elements.push_back(newItem);
    }

    return elements;
}

HsmElement* HsmElementsFactory::createElement(const QString& typeId, const model::EntityID_t modelElementId) {
    HsmElement* element = nullptr;
    auto itItem = mItemsCatalog.find(typeId);

    if (mItemsCatalog.end() != itItem) {
        const auto& callback = std::get<2>(itItem->second);

        element = callback(modelElementId);
        // element->setModelId(modelElementId);
    }

    return element;
}

HsmElement* HsmElementsFactory::_createElementState(const model::EntityID_t modelElementId) {
    HsmElement* elem = new HsmStateElement();

    elem->init(modelElementId);
    return elem;
}

HsmElement* HsmElementsFactory::_createElementStart(const model::EntityID_t modelElementId) {
    HsmElement* elem = new HsmInitialElement();

    elem->init(modelElementId);
    return elem;
}

HsmElement* HsmElementsFactory::_createElementFinal(const model::EntityID_t modelElementId) {
    HsmElement* elem = new HsmFinalElement();

    elem->init(modelElementId);
    return elem;
}

HsmElement* HsmElementsFactory::_createElementEntryPoint(const model::EntityID_t modelElementId) {
    HsmElement* elem = new HsmEntryPointElement();

    elem->init(modelElementId);
    return elem;
}

HsmElement* HsmElementsFactory::_createElementExitPoint(const model::EntityID_t modelElementId) {
    HsmElement* elem = new HsmExitPointElement();

    elem->init(modelElementId);
    return elem;
}

HsmElement* HsmElementsFactory::_createElementHistory(const model::EntityID_t modelElementId) {
    HsmElement* elem = new HsmHistoryElement();

    elem->init(modelElementId);
    return elem;
}

};  // namespace view