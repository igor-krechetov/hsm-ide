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

private:
    HsmLayoutSerializer mLayoutSerializer;
};

}  // namespace model

#endif  // HSMMETADATASERIALIZER_HPP
