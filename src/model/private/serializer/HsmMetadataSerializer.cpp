#include "HsmMetadataSerializer.hpp"

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "StateMachineModel.hpp"
#include "elements/StateMachineEntity.hpp"

namespace model {

// --- Serialization ---

void HsmMetadataSerializer::beginSerialization(QXmlStreamWriter& writer, const QSharedPointer<StateMachineModel>& /*model*/) {
    writer.writeAttribute("xmlns:hsm", scxml::HSM_NAMESPACE_URI);
}

void HsmMetadataSerializer::notifyEnterState(const StateMachineEntity* /*entity*/) {
    // No-op: HSM stores local coordinates, no scene position accumulation needed
}

void HsmMetadataSerializer::notifyExitState(const StateMachineEntity* /*entity*/) {
    // No-op
}

void HsmMetadataSerializer::writeEntityAttributes(AttributeWriter& writer, const StateMachineEntity* entity) {
    writer.writeAttribute(scxml::HSM_UID_ATTR, QString::number(entity->id()));
}

void HsmMetadataSerializer::writeEntityChildMetadata(QXmlStreamWriter& /*writer*/, const StateMachineEntity* /*entity*/) {
    // No-op: HSM defers all layout data to the <hsm:editor> section written in endSerialization
}

void HsmMetadataSerializer::endSerialization(QXmlStreamWriter& writer, const QSharedPointer<StateMachineModel>& model) {
    mLayoutSerializer.writeEditorSection(model, writer);
}

// --- Deserialization ---

DetectedFormat HsmMetadataSerializer::detectFormat(const QXmlStreamReader& reader) const {
    DetectedFormat result = DetectedFormat::PlainSCXML;

    for (const auto& ns : reader.namespaceDeclarations()) {
        if (ns.namespaceUri() == scxml::HSM_NAMESPACE_URI) {
            result = DetectedFormat::HSM;
            break;
        }
    }

    return result;
}

EntityID_t HsmMetadataSerializer::resolveEntityId(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    EntityID_t result = INVALID_MODEL_ID;
    QString uidStr = reader.attributes().value(scxml::HSM_UID_ATTR).toString();

    if (!uidStr.isEmpty()) {
        bool ok = false;
        quint64 rawValue = uidStr.toULongLong(&ok);

        if (ok && rawValue >= 1 && rawValue <= 0xFFFFFFFE) {
            EntityID_t uid = static_cast<EntityID_t>(rawValue);

            // registerRestoredId is the single source of truth for id ownership. It fails when
            // the value is already taken - either by a UID seen earlier in the file or by a
            // replacement previously issued via generateNextId(). In every such case we must
            // fall back to a freshly generated id, otherwise two entities could share an id.
            if (model->idGenerator().registerRestoredId(uid)) {
                result = uid;
            } else {
                qWarning() << "Duplicate hsm:uid" << uid << "- generating replacement";
            }
        } else {
            qWarning() << "Invalid hsm:uid value:" << uidStr << "- generating replacement";
        }
    }

    if (result == INVALID_MODEL_ID) {
        result = model->idGenerator().generateNextId();
    }

    return result;
}

bool HsmMetadataSerializer::parseMetadataElement(QXmlStreamReader& /*reader*/,
                                                 StateMachineEntity* /*entity*/,
                                                 const QSharedPointer<StateMachineModel>& /*model*/) {
    // HSM has no inline metadata elements within entities
    return false;
}

bool HsmMetadataSerializer::parseTopLevelElement(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) {
    bool consumed = false;

    if (reader.name() == QStringView(u"editor")) {
        mLayoutSerializer.parseEditorSection(reader, model);
        consumed = true;
    }

    return consumed;
}

void HsmMetadataSerializer::endDeserialization(const QSharedPointer<StateMachineModel>& model) {
    model->idGenerator().synchronizeCounter();
}

// --- Query ---

SerializationFormat HsmMetadataSerializer::format() const {
    return SerializationFormat::HSM;
}

}  // namespace model
