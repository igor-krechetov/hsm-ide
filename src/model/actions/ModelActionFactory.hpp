#ifndef MODELACTIONFACTORY_HPP
#define MODELACTIONFACTORY_HPP

#include <QSharedPointer>
#include <QStringList>
#include <memory>

#include "IModelAction.hpp"

namespace model {

QSharedPointer<IModelAction> createModelAction(ModelAction type);
QSharedPointer<IModelAction> createModelActionFromData(const QString& data, ModelAction fallbackType = ModelAction::CALLBACK);
QString actionName(ModelAction type);
ModelAction actionTypeByName(const QString& name);
QStringList supportedActionNames();

}  // namespace model

#endif
