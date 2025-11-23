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
#include "HsmIncludeElement.hpp"
#include "HsmInitialElement.hpp"
#include "HsmStateElement.hpp"
#include "private/HsmElement.hpp"

namespace view {

QMap<QString,
     std::tuple<QString, QString, std::function<HsmElement*(const QSharedPointer<model::StateMachineEntity>&, const QSizeF&)>>>
    HsmElementsFactory::mItemsCatalog = {
        {"initial", {"initial", ":/icons/elements/initial.png", &HsmElementsFactory::createElementStart}},
        {"final", {"Final", ":/icons/elements/final.png", &HsmElementsFactory::createElementFinal}},
        {"state", {"State", ":/icons/elements/state.png", &HsmElementsFactory::createElementState}},
        {"entrypoint", {"Entry Point", ":/icons/elements/entrypoint.png", &HsmElementsFactory::createElementEntryPoint}},
        {"exitpoint", {"Exit Point", ":/icons/elements/exitpoint.png", &HsmElementsFactory::createElementExitPoint}},
        {"history", {"History", ":/icons/elements/history.png", &HsmElementsFactory::createElementHistory}},
        {"include", {"Include", ":/icons/elements/include.png", &HsmElementsFactory::createElementInclude}}};

std::list<QListWidgetItem*> HsmElementsFactory::createElementsList() {
    const QList<QString> itemsOrder = {"initial", "final", "state", "include", "entrypoint", "exitpoint", "history"};
    std::list<QListWidgetItem*> elements;

    Q_ASSERT(itemsOrder.size() == mItemsCatalog.size());

    for (const QString& itemId : itemsOrder) {
        // for (const auto& itemInfo : mItemsCatalog) {
        auto it = mItemsCatalog.find(itemId);

        if (mItemsCatalog.end() != it) {
            QIcon icon(std::get<1>(it.value()));
            QString itemName = std::get<0>(it.value());
            QListWidgetItem* newItem = new QListWidgetItem(icon, itemName);

            newItem->setData(Qt::UserRole, it.key());
            elements.push_back(newItem);
        } else {
            qFatal("Item %s not registered in HsmElementsFactory", itemId.toUtf8().constData());
        }

        qDebug() << "--- " << it.key();
    }

    return elements;
}

QString HsmElementsFactory::getElementIcon(const QString& typeId) {
    QString iconPath;
    auto itItem = mItemsCatalog.find(typeId);

    if (mItemsCatalog.end() != itItem) {
        iconPath = std::get<1>(itItem.value());
    } else {
        qFatal("Item %s not registered in HsmElementsFactory", typeId.toUtf8().constData());
    }

    return iconPath;
}

QString HsmElementsFactory::getStateIcon(const model::StateType type) {
    QString iconPath;

    switch (type) {
        case model::StateType::REGULAR:
            iconPath = getElementIcon("state");
            break;
        case model::StateType::ENTRYPOINT:
            iconPath = getElementIcon("entrypoint");
            break;
        case model::StateType::EXITPOINT:
            iconPath = getElementIcon("exitpoint");
            break;
        case model::StateType::INITIAL:
            iconPath = getElementIcon("initial");
            break;
        case model::StateType::FINAL:
            iconPath = getElementIcon("final");
            break;
        case model::StateType::HISTORY:
            iconPath = getElementIcon("history");
            break;
        case model::StateType::INCLUDE:
            iconPath = getElementIcon("include");
            break;
        default:
            break;
    }

    return iconPath;
}

HsmElement* HsmElementsFactory::createElement(const QString& typeId,
                                              const QSharedPointer<model::StateMachineEntity>& modelElement) {
    return createElement(typeId, modelElement, QSizeF());
}

HsmElement* HsmElementsFactory::createElement(const QString& typeId,
                                              const QSharedPointer<model::StateMachineEntity>& modelElement,
                                              const QSizeF& size) {
    HsmElement* element = nullptr;
    auto itItem = mItemsCatalog.find(typeId);

    if (mItemsCatalog.end() != itItem) {
        const auto& callback = std::get<2>(itItem.value());

        element = callback(modelElement, size);
    } else {
        qFatal("Item %s not registered in HsmElementsFactory", typeId.toUtf8().constData());
    }

    return element;
}

HsmElement* HsmElementsFactory::createElementState(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                   const QSizeF& size) {
    HsmElement* elem = nullptr;

    if (size.isNull() == false) {
        elem = new HsmStateElement(size);
    } else {
        elem = new HsmStateElement();
    }

    elem->init(modelElement);

    return elem;
}

HsmElement* HsmElementsFactory::createElementStart(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                   const QSizeF& size) {
    HsmElement* elem = new HsmInitialElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementFinal(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                   const QSizeF& size) {
    HsmElement* elem = new HsmFinalElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementEntryPoint(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                        const QSizeF& size) {
    HsmElement* elem = new HsmEntryPointElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementExitPoint(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                       const QSizeF& size) {
    HsmElement* elem = new HsmExitPointElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementHistory(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                     const QSizeF& size) {
    HsmElement* elem = new HsmHistoryElement();

    elem->init(modelElement);
    return elem;
}

HsmElement* HsmElementsFactory::createElementInclude(const QSharedPointer<model::StateMachineEntity>& modelElement,
                                                     const QSizeF& size) {
    HsmElement* elem = new HsmIncludeElement();

    elem->init(modelElement);
    return elem;
}

};  // namespace view