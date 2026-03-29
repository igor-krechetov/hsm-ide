#ifndef MODELACTIONFACTORY_HPP
#define MODELACTIONFACTORY_HPP

#include <QSharedPointer>
#include <QStringList>

#include "IModelAction.hpp"

namespace model {

class ModelActionFactory {
public:
    static QSharedPointer<IModelAction> createModelAction(ModelAction type = ModelAction::NONE);
    static QSharedPointer<IModelAction> createModelActionFromData(const QString& data,
                                                                  ModelAction fallbackType = ModelAction::NONE);
    static QString actionName(ModelAction type);
    static ModelAction actionTypeByName(const QString& name);
    static QStringList supportedActionNames();
};

}  // namespace model

#endif
