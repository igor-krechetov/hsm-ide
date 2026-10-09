#ifndef HSMMETADATASERIALIZER_HPP
#define HSMMETADATASERIALIZER_HPP

#include "HsmLayoutSerializer.hpp"
#include "IMetadataSerializer.hpp"

namespace model {

class HsmMetadataSerializer : public IMetadataSerializer {
public:
    HsmMetadataSerializer() = default;
    ~HsmMetadataSerializer() override = default;

    // --- Serialization ---
    void beginSerialization(QXmlStreamWriter& writer, const QSharedPointer<StateMachineModel>& model) override;
    void notifyEnterState(const StateMachineEntity* entity) override;
    void notifyExitState(const StateMachineEntity* entity) override;
    void writeEntityAttributes(AttributeWriter& writer, const StateMachineEntity* entity) override;
    void writeEntityChildMetadata(QXmlStreamWriter& writer, const StateMachineEntity* entity) override;
    void endSerialization(QXmlStreamWriter& writer, const QSharedPointer<StateMachineModel>& model) override;

    // --- Deserialization ---
    DetectedFormat detectFormat(const QXmlStreamReader& reader) const override;
    EntityID_t resolveEntityId(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) override;
    bool parseMetadataElement(QXmlStreamReader& reader,
                              StateMachineEntity* entity,
                              const QSharedPointer<StateMachineModel>& model) override;
    bool parseTopLevelElement(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) override;
    void endDeserialization(const QSharedPointer<StateMachineModel>& model) override;

    // --- Query ---
    SerializationFormat format() const override;

    // Clipboard mode (REQ-103f7 / copy-paste): when enabled, hsm:uid is neither written on
    // serialize nor honored on deserialize (fresh ids are always generated). Used for
    // copy/paste so pasted elements never collide with the originals' UIDs, and so plain
    // user-authored XML pastes cleanly.
    void setIgnoreUid(const bool ignore);

private:
    HsmLayoutSerializer mLayoutSerializer;
    bool mIgnoreUid = false;
};

}  // namespace model

#endif  // HSMMETADATASERIALIZER_HPP
