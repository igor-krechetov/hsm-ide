#ifndef HSMELEMENTSFACTORY_HPP
#define HSMELEMENTSFACTORY_HPP

#include <QString>
#include <list>
#include <map>
#include <tuple>

#include "private/HsmElement.hpp"

class QListWidgetItem;

namespace view {

class HsmElementsFactory {
public:
    static std::list<QListWidgetItem*> createElementsList();
    static HsmElement* createElement(const QString& typeId, const uint32_t modelElementId);

private:
    static HsmElement* _createElementState();
    static HsmElement* _createElementStart();
    static HsmElement* _createElementFinal();
    static HsmElement* _createElementEntryPoint();
    static HsmElement* _createElementExitPoint();
    static HsmElement* _createElementHistory();

private:
    static std::map<QString, std::tuple<QString, QString, std::function<HsmElement*()>>> mItemsCatalog;
};

}; // namespace view

#endif  // HSMELEMENTSFACTORY_HPP
