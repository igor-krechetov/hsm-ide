#include "HsmElementsFactory.hpp"

#include <QListWidgetItem>
#include "HsmStateElement.hpp"
#include "HsmStartElement.hpp"
#include "HsmFinalElement.hpp"
#include "HsmEntryPointElement.hpp"
#include "HsmExitPointElement.hpp"
#include "HsmHistoryElement.hpp"

#include <QDir>
#include <QIcon>

std::map<QString, std::tuple<QString, QString, std::function<HsmElement*>()>> HsmElementsFactory::mItemsCatalog = {
        {"start", {"Start", "/../../res/element_start.png", &HsmElementsFactory::_createElementStart}},
        {"final", {"Final", "/../../res/element_final.png", &HsmElementsFactory::_createElementFinal}},
        {"state", {"State", "/../../res/element_state.png", &HsmElementsFactory::_createElementState}},
        {"entrypoint", {"Entry Point", "/../../res/element_entrypoint.png", &HsmElementsFactory::_createElementEntryPoint}},
        {"exitpoint", {"Exit Point", "/../../res/element_exitpoint.png", &HsmElementsFactory::_createElementExitPoint}},
        {"history", {"History", "/../../res/element_history.png", &HsmElementsFactory::_createElementHistory}}
    };


std::list<QListWidgetItem*> HsmElementsFactory::createElementsList()
{
    std::list<QListWidgetItem*> elements;
    QString appDir = QCoreApplication::applicationDirPath();

    for (const auto& itemInfo : mItemsCatalog) {
        QIcon icon(appDir + std::get<1>(itemInfo));
        QString itemName = std::get<0>(itemInfo);
        QListWidgetItem* newItem = new QListWidgetItem(icon, itemName);

        newItem->setData(Qt::UserRole, id);
        elements.push_back(newItem);
    }

    return elements;
}

HsmElement* HsmElementsFactory::createElement(const QString& id)
{
    HsmElement* element = nullptr;
    auto itItem = mItemsCatalog.find(id);

    if (mItemscatalog.end() != itItem) {
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

HsmElement* HsmElementsFactory::_createElementState()
{
    return new HsmStateElement();
}

HsmElement* HsmElementsFactory::_createElementStart()
{
    return new HsmStartElement();
}

HsmElement* HsmElementsFactory::_createElementFinal()
{
    return new HsmFinalElement();
}

HsmElement* HsmElementsFactory::_createElementEntryPoint()
{
    return new HsmEntryPointElement();
}

HsmElement* HsmElementsFactory::_createElementExitPoint()
{
    return new HsmExitPointElement();
}

HsmElement* HsmElementsFactory::_createElementHistory()
{
    return new HsmHistoryElement();
}
