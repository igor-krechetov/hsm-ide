#ifndef HSMELEMENTSFACTORY_HPP
#define HSMELEMENTSFACTORY_HPP

#include <QString>
#include <list>
#include <map>
#include <tuple>

#include "model/ModelTypes.hpp"
#include "private/HsmElement.hpp"

class QListWidgetItem;

namespace view {

class HsmElementsFactory {
public:
    static std::list<QListWidgetItem*> createElementsList();
    static HsmElement* createElement(const QString& typeId, const model::EntityID_t modelElementId);

private:
    static HsmElement* _createElementState(const model::EntityID_t modelElementId);
    static HsmElement* _createElementStart(const model::EntityID_t modelElementId);
    static HsmElement* _createElementFinal(const model::EntityID_t modelElementId);
    static HsmElement* _createElementEntryPoint(const model::EntityID_t modelElementId);
    static HsmElement* _createElementExitPoint(const model::EntityID_t modelElementId);
    static HsmElement* _createElementHistory(const model::EntityID_t modelElementId);

private:
    static std::map<QString, std::tuple<QString, QString, std::function<HsmElement*(model::EntityID_t)>>> mItemsCatalog;
};

}; // namespace view

#endif  // HSMELEMENTSFACTORY_HPP
