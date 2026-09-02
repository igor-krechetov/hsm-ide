#include "HsmLayoutSerializer.hpp"

#include <QDebug>
#include <QPolygonF>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "IMetadataSerializer.hpp"
#include "ModelTypes.hpp"
#include "StateMachineModel.hpp"
#include "elements/ModelRootState.hpp"
#include "elements/StateMachineEntity.hpp"

namespace model {

void HsmLayoutSerializer::writeEditorSection(const QSharedPointer<StateMachineModel>& model, QXmlStreamWriter& writer) {
    writer.writeStartElement(scxml::HSM_EDITOR_ELEMENT);
    writeLayoutSection(model, writer);
    writer.writeEndElement();  // hsm:editor
}

void HsmLayoutSerializer::writeLayoutSection(const QSharedPointer<StateMachineModel>& model, QXmlStreamWriter& writer) {
    writer.writeStartElement(scxml::HSM_LAYOUT_ELEMENT);

    if (model->root()) {
        model->root()->forEachChildElement(
            [this, &writer](QSharedPointer<StateMachineEntity> /*parent*/, QSharedPointer<StateMachineEntity> child) {
                if (child->type() == StateMachineEntity::Type::State) {
                    if (child->getMetadata(StateMachineEntity::MetadataKey::POSITION_X).isValid()) {
                        writeStateLayout(child.get(), writer);
                    }
                } else if (child->type() == StateMachineEntity::Type::Transition) {
                    QVariant geometryData = child->getMetadata(StateMachineEntity::MetadataKey::GEOMETRY);
                    bool hasRouting = geometryData.isValid();
                    // Label position is not yet stored in model metadata, but check for future support
                    if (hasRouting) {
                        writeTransitionLayout(child.get(), writer);
                    }
                }
                return true;
            },
            StateMachineEntity::DEPTH_INFINITE,
            false);
    }

    writer.writeEndElement();  // hsm:layout
}

void HsmLayoutSerializer::writeStateLayout(const StateMachineEntity* entity, QXmlStreamWriter& writer) {
    const double x = entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double y = entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double width = entity->getMetadata(StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double height = entity->getMetadata(StateMachineEntity::MetadataKey::HEIGHT).toDouble();

    writer.writeStartElement(scxml::HSM_STATE_ELEMENT);
    writer.writeAttribute("uid", QString::number(entity->id()));
    writer.writeAttribute("x", QString::number(x, 'f', 2));
    writer.writeAttribute("y", QString::number(y, 'f', 2));
    writer.writeAttribute("width", QString::number(width, 'f', 2));
    writer.writeAttribute("height", QString::number(height, 'f', 2));
    writer.writeEndElement();  // hsm:state
}

void HsmLayoutSerializer::writeTransitionLayout(const StateMachineEntity* entity, QXmlStreamWriter& writer) {
    writer.writeStartElement(scxml::HSM_TRANSITION_ELEMENT);
    writer.writeAttribute("uid", QString::number(entity->id()));

    QVariant geometryData = entity->getMetadata(StateMachineEntity::MetadataKey::GEOMETRY);

    if (geometryData.isValid()) {
        QPolygonF linePath = geometryData.value<QPolygonF>();

        // Extract INTERMEDIATE points only (skip first and last which are start/end endpoints)
        if (linePath.size() > 2) {
            QStringList pointStrings;

            for (int i = 1; i < linePath.size() - 1; ++i) {
                const QPointF& point = linePath.at(i);
                pointStrings.append(
                    QString("%1,%2").arg(QString::number(point.x(), 'f', 2)).arg(QString::number(point.y(), 'f', 2)));
            }

            if (pointStrings.isEmpty() == false) {
                writer.writeStartElement(scxml::HSM_POINTS_ELEMENT);
                writer.writeAttribute("geometry", pointStrings.join(';'));
                writer.writeEndElement();  // hsm:points
            }
        }
    }

    // Label position: currently not stored in model metadata, structure ready for future support

    writer.writeEndElement();  // hsm:transition
}

void HsmLayoutSerializer::parseEditorSection(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    // Expects reader positioned at <hsm:editor> start element
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringView(u"layout")) {
            parseLayoutSection(reader, model);
        } else {
            reader.skipCurrentElement();
        }
    }
}

void HsmLayoutSerializer::parseLayoutSection(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringView(u"state")) {
            parseStateLayout(reader, model);
        } else if (reader.name() == QStringView(u"transition")) {
            parseTransitionLayout(reader, model);
        } else if (reader.name() == QStringView(u"comment")) {
            parseCommentLayout(reader, model);
        } else {
            reader.skipCurrentElement();
        }
    }
}

void HsmLayoutSerializer::parseStateLayout(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    const QXmlStreamAttributes attrs = reader.attributes();
    const QString uidStr = attrs.value("uid").toString();

    if (uidStr.isEmpty()) {
        qWarning() << "HsmLayoutSerializer: <hsm:state> missing uid attribute at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    bool uidOk = false;
    const EntityID_t uid = uidStr.toUInt(&uidOk);

    if (!uidOk || uid == 0 || uid == INVALID_MODEL_ID) {
        qWarning() << "HsmLayoutSerializer: <hsm:state> invalid uid value" << uidStr << "at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    QSharedPointer<StateMachineEntity> entity = model->root()->findChild(uid);

    if (!entity) {
        qWarning() << "HsmLayoutSerializer: <hsm:state> uid" << uid << "does not match any entity at line"
                   << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    bool xOk = false;
    bool yOk = false;
    bool wOk = false;
    bool hOk = false;
    const double x = attrs.value("x").toDouble(&xOk);
    const double y = attrs.value("y").toDouble(&yOk);
    const double width = attrs.value("width").toDouble(&wOk);
    const double height = attrs.value("height").toDouble(&hOk);

    if (!xOk || !yOk || !wOk || !hOk) {
        qWarning() << "HsmLayoutSerializer: <hsm:state> uid" << uid << "has non-numeric coordinate/dimension attributes at line"
                   << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_X, x);
    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_Y, y);
    entity->setMetadata(StateMachineEntity::MetadataKey::WIDTH, width);
    entity->setMetadata(StateMachineEntity::MetadataKey::HEIGHT, height);

    reader.skipCurrentElement();
}

void HsmLayoutSerializer::parseTransitionLayout(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    const QXmlStreamAttributes attrs = reader.attributes();
    const QString uidStr = attrs.value("uid").toString();

    if (uidStr.isEmpty()) {
        qWarning() << "HsmLayoutSerializer: <hsm:transition> missing uid attribute at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    bool uidOk = false;
    const EntityID_t uid = uidStr.toUInt(&uidOk);

    if (!uidOk || uid == 0 || uid == INVALID_MODEL_ID) {
        qWarning() << "HsmLayoutSerializer: <hsm:transition> invalid uid value" << uidStr << "at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    QSharedPointer<StateMachineEntity> entity = model->root()->findChild(uid, StateMachineEntity::Type::Transition);

    if (!entity) {
        qWarning() << "HsmLayoutSerializer: <hsm:transition> uid" << uid << "does not match any transition at line"
                   << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.name() == QStringView(u"points")) {
            const QString geometryStr = reader.attributes().value("geometry").toString();

            if (geometryStr.isEmpty() == false) {
                const QStringList pairs = geometryStr.split(';');
                QPolygonF linePath;
                bool parseOk = true;

                // Add placeholder start point (derived from connected states at render time)
                linePath.append(QPointF(0, 0));

                for (const QString& pair : pairs) {
                    const QStringList coords = pair.split(',');

                    if (coords.size() != 2) {
                        parseOk = false;
                        break;
                    }

                    bool xOk = false;
                    bool yOk = false;
                    const double px = coords[0].toDouble(&xOk);
                    const double py = coords[1].toDouble(&yOk);

                    if (!xOk || !yOk) {
                        parseOk = false;
                        break;
                    }

                    linePath.append(QPointF(px, py));
                }

                // Add placeholder end point (derived from connected states at render time)
                linePath.append(QPointF(0, 0));

                if (parseOk) {
                    entity->setMetadata(StateMachineEntity::MetadataKey::GEOMETRY, QVariant::fromValue(linePath));
                } else {
                    qWarning() << "HsmLayoutSerializer: <hsm:points> malformed geometry" << geometryStr << "for uid" << uid
                               << "at line" << reader.lineNumber();
                }
            }

            reader.skipCurrentElement();
        } else if (reader.name() == QStringView(u"label")) {
            // Label position: parse x, y for future support
            // Currently no model MetadataKey for label offset, but structure is ready
            bool xOk = false;
            bool yOk = false;
            const double labelX = reader.attributes().value("x").toDouble(&xOk);
            const double labelY = reader.attributes().value("y").toDouble(&yOk);
            Q_UNUSED(labelX);
            Q_UNUSED(labelY);

            if (!xOk || !yOk) {
                qWarning() << "HsmLayoutSerializer: <hsm:label> has non-numeric x/y for uid" << uid << "at line"
                           << reader.lineNumber();
            }

            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }
}

void HsmLayoutSerializer::parseCommentLayout(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    const QXmlStreamAttributes attrs = reader.attributes();
    const QString uidStr = attrs.value("uid").toString();

    if (uidStr.isEmpty()) {
        qWarning() << "HsmLayoutSerializer: <hsm:comment> missing uid attribute at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    bool uidOk = false;
    const EntityID_t uid = uidStr.toUInt(&uidOk);

    if (!uidOk || uid == 0 || uid == INVALID_MODEL_ID) {
        qWarning() << "HsmLayoutSerializer: <hsm:comment> invalid uid value" << uidStr << "at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    QSharedPointer<StateMachineEntity> entity = model->root()->findChild(uid);

    if (!entity) {
        qWarning() << "HsmLayoutSerializer: <hsm:comment> uid" << uid << "does not match any entity at line"
                   << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    bool xOk = false;
    bool yOk = false;
    bool wOk = false;
    bool hOk = false;
    const double x = attrs.value("x").toDouble(&xOk);
    const double y = attrs.value("y").toDouble(&yOk);
    const double width = attrs.value("width").toDouble(&wOk);
    const double height = attrs.value("height").toDouble(&hOk);

    if (!xOk || !yOk || !wOk || !hOk) {
        qWarning() << "HsmLayoutSerializer: <hsm:comment> uid" << uid
                   << "has non-numeric coordinate/dimension attributes at line" << reader.lineNumber();
        reader.skipCurrentElement();
        return;
    }

    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_X, x);
    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_Y, y);
    entity->setMetadata(StateMachineEntity::MetadataKey::WIDTH, width);
    entity->setMetadata(StateMachineEntity::MetadataKey::HEIGHT, height);

    reader.skipCurrentElement();
}

}  // namespace model
