#ifndef HSMELEMENTSFACTORY_HPP
#define HSMELEMENTSFACTORY_HPP

#include <list>
#include <map>
#include <tuple>
#include <QString>
#include "private/HsmElement.hpp"

class QListWidgetItem;

class HsmElementsFactory {
public:
    static std::list<QListWidgetItem*> createElementsList();
    static HsmElement* createElement(const QString& id);

private:
    static HsmElement* _createElementState();
    static HsmElement* _createElementStart();
    static HsmElement* _createElementFinal();
    static HsmElement* _createElementEntryPoint();
    static HsmElement* _createElementExitPoint();
    static HsmElement* _createElementHistory();

private:
    static std::map<QString, std::tuple<QString, QString, std::function<HsmElement* ()>>> mItemsCatalog;
};

#endif // HSMELEMENTSFACTORY_HPP
