#include "QtCreatorMetadataSerializer.hpp"

#include <QDebug>
#include <QPolygonF>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "StateMachineModel.hpp"
#include "elements/ModelRootState.hpp"
#include "elements/State.hpp"
#include "elements/StateMachineEntity.hpp"
#include "elements/Transition.hpp"

namespace model {

// --- Serialization ---

void QtCreatorMetadataSerializer::beginSerialization(QXmlStreamWriter& writer,
                                                     const QSharedPointer<StateMachineModel>& /*model*/) {
    writer.writeAttribute("xmlns:qt", scxml::QT_NAMESPACE_URI);
    mParentScenePositionStack.clear();
    mParentScenePositionStack.push(QPointF(0.0, 0.0));
}

void QtCreatorMetadataSerializer::notifyEnterState(const StateMachineEntity* entity) {
    const QPointF parentScene = mParentScenePositionStack.top();
    const double localX = entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double localY = entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    mParentScenePositionStack.push(QPointF(parentScene.x() + localX, parentScene.y() + localY));
}

void QtCreatorMetadataSerializer::notifyExitState(const StateMachineEntity* /*entity*/) {
    mParentScenePositionStack.pop();
}

void QtCreatorMetadataSerializer::writeEntityAttributes(AttributeWriter& /*writer*/, const StateMachineEntity* /*entity*/) {
    // No-op: Qt Creator format has no extra attributes on SCXML elements
}

void QtCreatorMetadataSerializer::writeEntityChildMetadata(QXmlStreamWriter& writer, const StateMachineEntity* entity) {
    if (entity->type() == StateMachineEntity::Type::Transition) {
        writeTransitionGeometry(writer, entity);
    } else {
        writeStateGeometry(writer, entity);
    }
}

void QtCreatorMetadataSerializer::endSerialization(QXmlStreamWriter& /*writer*/,
                                                   const QSharedPointer<StateMachineModel>& /*model*/) {
    // No-op: all geometry was written inline
    mParentScenePositionStack.clear();
}

// --- Deserialization ---

DetectedFormat QtCreatorMetadataSerializer::detectFormat(const QXmlStreamReader& reader) const {
    DetectedFormat result = DetectedFormat::PlainSCXML;

    for (const auto& ns : reader.namespaceDeclarations()) {
        if (ns.namespaceUri() == scxml::QT_NAMESPACE_URI) {
            result = DetectedFormat::QtCreator;
            break;
        }
    }

    return result;
}

EntityID_t QtCreatorMetadataSerializer::resolveEntityId(QXmlStreamReader& /*reader*/,
                                                        const QSharedPointer<StateMachineModel>& model) {
    // Qt Creator files have no persisted UIDs — always generate
    return model->idGenerator().generateNextId();
}

bool QtCreatorMetadataSerializer::parseMetadataElement(QXmlStreamReader& reader,
                                                       StateMachineEntity* entity,
                                                       const QSharedPointer<StateMachineModel>& /*model*/) {
    bool consumed = false;

    if (reader.name() == QStringView(u"editorinfo")) {
        parseEditorInfo(reader, entity);
        consumed = true;
    }

    return consumed;
}

bool QtCreatorMetadataSerializer::parseTopLevelElement(QXmlStreamReader& reader,
                                                       const QSharedPointer<StateMachineModel>& model) {
    bool consumed = false;

    if (reader.name() == QStringView(u"editorinfo")) {
        QString initialGeometry = reader.attributes().value("initialGeometry").toString();

        if (!initialGeometry.isEmpty()) {
            mInitialGeometryString = initialGeometry;
        }

        reader.readNext();
        consumed = true;
    }

    return consumed;
}

void QtCreatorMetadataSerializer::endDeserialization(const QSharedPointer<StateMachineModel>& model) {
    postprocessStateGeometry(model);
    postprocessTransitionGeometry(model);
    postprocessInitialGeometry(model);
    mSceneGeometryStrings.clear();
    mGeometryStrings.clear();
    mTransitionTargets.clear();
    mInitialGeometryString.clear();
}

// --- Query ---

SerializationFormat QtCreatorMetadataSerializer::format() const {
    return SerializationFormat::QtCreator;
}

// --- Private: Serialization helpers ---

void QtCreatorMetadataSerializer::writeStateGeometry(QXmlStreamWriter& writer, const StateMachineEntity* entity) {
    const QPointF parentScene = mParentScenePositionStack.top();
    const double localX = entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double localY = entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double sceneX = localX + parentScene.x();
    const double sceneY = localY + parentScene.y();
    const double w = entity->getMetadata(StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double h = entity->getMetadata(StateMachineEntity::MetadataKey::HEIGHT).toDouble();

    QString geometryValue = QString("%1;%2;%3;%4;%5;%6")
                                .arg(QString::number(0.0, 'f', 2))
                                .arg(QString::number(0.0, 'f', 2))
                                .arg(QString::number(sceneX, 'f', 2))
                                .arg(QString::number(sceneY, 'f', 2))
                                .arg(QString::number(w, 'f', 2))
                                .arg(QString::number(h, 'f', 2));

    writer.writeStartElement("qt:editorinfo");
    writer.writeAttribute("scenegeometry", geometryValue);
    writer.writeEndElement();
}

void QtCreatorMetadataSerializer::writeTransitionGeometry(QXmlStreamWriter& writer, const StateMachineEntity* entity) {
    QVariant geometryData = entity->getMetadata(StateMachineEntity::MetadataKey::GEOMETRY);

    if (geometryData.isValid()) {
        QPolygonF linePath = geometryData.value<QPolygonF>();
        QStringList pointStrings;

        // Remove first and last points (endpoints recalculated at render time)
        if (linePath.size() >= 2) {
            linePath.removeFirst();
            linePath.removeLast();
        }

        for (const QPointF& point : linePath) {
            pointStrings.append(
                QString("%1;%2").arg(QString::number(point.x(), 'f', 2)).arg(QString::number(point.y(), 'f', 2)));
        }

        if (pointStrings.isEmpty() == false) {
            writer.writeStartElement("qt:editorinfo");
            writer.writeAttribute("localGeometry", pointStrings.join(';'));
            writer.writeEndElement();
        }
    }
}

// --- Private: Deserialization helpers ---

void QtCreatorMetadataSerializer::parseEditorInfo(QXmlStreamReader& reader, StateMachineEntity* entity) {
    if (nullptr == entity) {
        qCritical() << "entity is null";
        return;
    }

    QString localGeometryValue = reader.attributes().value("localGeometry").toString();

    if (localGeometryValue.isEmpty() == false) {
        const QStringList pointParts = localGeometryValue.split(';');
        QPolygonF linePath;

        // Start and end points are recalculated by the HsmTransition element so we set them to 0
        linePath.append(QPointF(0, 0));  // start point

        for (int i = 0; i + 1 < pointParts.size(); i += 2) {
            bool okX = false;
            bool okY = false;
            const double x = pointParts[i].toDouble(&okX);
            const double y = pointParts[i + 1].toDouble(&okY);

            if (okX && okY) {
                linePath.append(QPointF(x, y));
            } else {
                qWarning() << "Invalid point format in qt:editorinfo localGeometry";
            }
        }

        linePath.append(QPointF(0, 0));  // end point
        entity->setMetadata(StateMachineEntity::MetadataKey::GEOMETRY, linePath);
    } else {
        QString sceneGeometryValue = reader.attributes().value("scenegeometry").toString();
        QString geometryValue = reader.attributes().value("geometry").toString();

        if (!sceneGeometryValue.isEmpty()) {
            const QStringList sceneGeometryParts = sceneGeometryValue.split(';');

            if (sceneGeometryParts.size() == 6) {
                mSceneGeometryStrings.insert(entity->id(), sceneGeometryValue);
            } else {
                qWarning() << "Invalid scenegeometry format in qt:editorinfo:" << sceneGeometryValue
                           << ", entity=" << entity->id();
            }
        } else if (!geometryValue.isEmpty()) {
            const QStringList geometryParts = geometryValue.split(';');

            if (geometryParts.size() == 6) {
                mGeometryStrings.insert(entity->id(), geometryValue);
            } else {
                qWarning() << "Invalid geometry format in qt:editorinfo:" << geometryValue << ", entity=" << entity->id();
            }
        }
    }

    reader.readNext();
}

void QtCreatorMetadataSerializer::postprocessStateGeometry(const QSharedPointer<StateMachineModel>& model) {
    if (!model || !model->root()) {
        return;
    }

    model->root()->forEachChildElement(
        [this, &model](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> child) {
            applyGeometryToState(child, parent, model);
            return true;
        },
        StateMachineEntity::DEPTH_INFINITE,
        false);
}

void QtCreatorMetadataSerializer::applyGeometryToState(const QSharedPointer<StateMachineEntity>& entity,
                                                       const QSharedPointer<StateMachineEntity>& parent,
                                                       const QSharedPointer<StateMachineModel>& model) {
    if (!entity) {
        return;
    }

    const QString geometryValue = mSceneGeometryStrings.contains(entity->id()) ? mSceneGeometryStrings.value(entity->id())
                                                                               : mGeometryStrings.value(entity->id());

    if (geometryValue.isEmpty()) {
        return;
    }

    const QStringList geometryParts = geometryValue.split(';');

    if (geometryParts.size() != 6) {
        qWarning() << "Invalid geometry format in qt:editorinfo:" << geometryValue << ", entity=" << entity->id();
        return;
    }

    double topX = 0.0;
    double topY = 0.0;
    const double w = geometryParts[4].toDouble();
    const double h = geometryParts[5].toDouble();

    if (mSceneGeometryStrings.contains(entity->id())) {
        topX = geometryParts[2].toDouble();
        topY = geometryParts[3].toDouble();
    } else {
        const double centerX = geometryParts[0].toDouble();
        const double centerY = geometryParts[1].toDouble();
        const double offsetX = geometryParts[2].toDouble();
        const double offsetY = geometryParts[3].toDouble();
        topX = centerX + offsetX;
        topY = centerY + offsetY;
    }

    if (parent) {
        // Compute the parent's scene position by accumulating positions up the hierarchy
        double parentSceneX = 0.0;
        double parentSceneY = 0.0;

        auto currentParent = parent;

        while (currentParent) {
            const QVariant px = currentParent->getMetadata(StateMachineEntity::MetadataKey::POSITION_X);
            const QVariant py = currentParent->getMetadata(StateMachineEntity::MetadataKey::POSITION_Y);

            if (px.isValid() && py.isValid()) {
                parentSceneX += px.toDouble();
                parentSceneY += py.toDouble();
            }

            // Walk up to the parent's parent via root search
            const EntityID_t parentId = currentParent->id();
            QSharedPointer<StateMachineEntity> grandparent;

            if (model && model->root()) {
                grandparent = model->root()->findParentState(parentId);
            }

            // Stop if we reached the root or can't find a parent
            if (!grandparent || grandparent == model->root()) {
                break;
            }

            currentParent = grandparent;
        }

        topX -= parentSceneX;
        topY -= parentSceneY;
    }

    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_X, topX);
    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_Y, topY);
    entity->setMetadata(StateMachineEntity::MetadataKey::WIDTH, w);
    entity->setMetadata(StateMachineEntity::MetadataKey::HEIGHT, h);
    entity->setMetadata(StateMachineEntity::MetadataKey::QT_DELTA_X, topX + w / 2.0);
    entity->setMetadata(StateMachineEntity::MetadataKey::QT_DELTA_Y, topY + h / 2.0);
}

void QtCreatorMetadataSerializer::postprocessTransitionGeometry(const QSharedPointer<StateMachineModel>& model) {
    if (!model || !model->root()) {
        return;
    }

    // Iterate all transitions and adjust intermediate points by source state delta
    model->root()->forEachChildElement(
        [](QSharedPointer<StateMachineEntity> /*parent*/, QSharedPointer<StateMachineEntity> child) {
            if (child->type() != StateMachineEntity::Type::Transition) {
                return true;
            }

            QSharedPointer<Transition> transition = child.dynamicCast<Transition>();

            if (!transition) {
                return true;
            }

            QSharedPointer<State> sourceState = transition->source();

            if (!sourceState) {
                return true;
            }

            QVariant geometryData = transition->getMetadata(StateMachineEntity::MetadataKey::GEOMETRY);

            if (!geometryData.isValid()) {
                return true;
            }

            if (!sourceState->getMetadata(StateMachineEntity::MetadataKey::QT_DELTA_X).isValid() ||
                !sourceState->getMetadata(StateMachineEntity::MetadataKey::QT_DELTA_Y).isValid()) {
                return true;
            }

            const double deltaX = sourceState->getMetadata(StateMachineEntity::MetadataKey::QT_DELTA_X).toDouble();
            const double deltaY = sourceState->getMetadata(StateMachineEntity::MetadataKey::QT_DELTA_Y).toDouble();
            const QPointF sourceDelta(deltaX, deltaY);

            QPolygonF linePath = geometryData.value<QPolygonF>();

            // Only update if line has intermediate points. Skip first and last points
            if (linePath.size() > 2) {
                for (int i = 1; i < (linePath.size() - 1); ++i) {
                    linePath[i] += sourceDelta;
                }
            }

            transition->setMetadata(StateMachineEntity::MetadataKey::GEOMETRY, linePath);
            return true;
        },
        StateMachineEntity::DEPTH_INFINITE,
        false);
}

void QtCreatorMetadataSerializer::postprocessInitialGeometry(const QSharedPointer<StateMachineModel>& model) {
    if (mInitialGeometryString.isEmpty() || !model || !model->root()) {
        return;
    }

    // Find existing InitialState at root level (created from initial="" attribute parsing)
    QSharedPointer<StateMachineEntity> initialEntity;

    model->root()->forEachChildElement(
        [&initialEntity](QSharedPointer<StateMachineEntity> /*parent*/, QSharedPointer<StateMachineEntity> child) {
            bool continueTraversal = true;

            if (child->type() == StateMachineEntity::Type::State) {
                auto state = child.dynamicCast<State>();

                if (state && state->stateType() == StateType::INITIAL) {
                    initialEntity = child;
                    continueTraversal = false;
                }
            }

            return continueTraversal;
        },
        1,
        false);

    if (!initialEntity) {
        return;
    }

    // Parse initialGeometry: "centerX;centerY;offsetX;offsetY;width;height"
    const QStringList parts = mInitialGeometryString.split(';');

    if (parts.size() != 6) {
        return;
    }

    const double centerX = parts[0].toDouble();
    const double centerY = parts[1].toDouble();
    const double offsetX = parts[2].toDouble();
    const double offsetY = parts[3].toDouble();
    const double w = parts[4].toDouble();
    const double h = parts[5].toDouble();

    // topLeft = center + offset (scene coordinates, parent is root at 0,0)
    const double topX = centerX + offsetX;
    const double topY = centerY + offsetY;

    initialEntity->setMetadata(StateMachineEntity::MetadataKey::POSITION_X, topX);
    initialEntity->setMetadata(StateMachineEntity::MetadataKey::POSITION_Y, topY);
    initialEntity->setMetadata(StateMachineEntity::MetadataKey::WIDTH, w);
    initialEntity->setMetadata(StateMachineEntity::MetadataKey::HEIGHT, h);
}

}  // namespace model
