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
#include "HsmStartElement.hpp"
#include "HsmStateElement.hpp"
#include "private/HsmElement.hpp"

std::map<QString, std::tuple<QString, QString, std::function<HsmElement*()>>> HsmElementsFactory::mItemsCatalog = {
    {"start", {"Start", "/../../res/element_start.png", &HsmElementsFactory::_createElementStart}},
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

HsmElement* HsmElementsFactory::createElement(const QString& id) {
    HsmElement* element = nullptr;
    auto itItem = mItemsCatalog.find(id);

    if (mItemsCatalog.end() != itItem) {
        const auto& callback = std::get<2>(itItem->second);

        element = callback();

        // if (callback == "_createElementState") {
        //     element = _createElementState();
        // }
        // else if (callback == "_createElementStart") {
        //     element = _createElementStart();
        // }
        // else if (callback == "_createElementFinal") {
        //     element = _createElementFinal();
        // }
        // else if (callback == "_createElementEntryPoint") {
        //     element = _createElementEntryPoint();
        // }
        // else if (callback == "_createElementExitPoint") {
        //     element = _createElementExitPoint();
        // }
        // else if (callback == "_createElementHistory") {
        //     element = _createElementHistory();
        // }
    }

    return element;
}

HsmElement* HsmElementsFactory::_createElementState() {
    HsmElement* elem = new HsmStateElement();

    elem->init();
    return elem;
}

HsmElement* HsmElementsFactory::_createElementStart() {
    HsmElement* elem = new HsmStartElement();

    elem->init();
    return elem;
}

HsmElement* HsmElementsFactory::_createElementFinal() {
    HsmElement* elem = new HsmFinalElement();

    elem->init();
    return elem;
}

HsmElement* HsmElementsFactory::_createElementEntryPoint() {
    HsmElement* elem = new HsmEntryPointElement();

    elem->init();
    return elem;
}

HsmElement* HsmElementsFactory::_createElementExitPoint() {
    HsmElement* elem = new HsmExitPointElement();

    elem->init();
    return elem;
}

HsmElement* HsmElementsFactory::_createElementHistory() {
    HsmElement* elem = new HsmHistoryElement();

    elem->init();
    return elem;
}
