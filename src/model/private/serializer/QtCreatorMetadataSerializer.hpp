#ifndef QTCREATORMETADATASERIALIZER_HPP
#define QTCREATORMETADATASERIALIZER_HPP

#include <QMap>
#include <QPointF>
#include <QSharedPointer>
#include <QStack>
#include <QString>

#include "IMetadataSerializer.hpp"

namespace model {

class QtCreatorMetadataSerializer : public IMetadataSerializer {
public:
    QtCreatorMetadataSerializer() = default;
    ~QtCreatorMetadataSerializer() override = default;

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
    void writeStateGeometry(QXmlStreamWriter& writer, const StateMachineEntity* entity);
    void writeTransitionGeometry(QXmlStreamWriter& writer, const StateMachineEntity* entity);

    void parseEditorInfo(QXmlStreamReader& reader, StateMachineEntity* entity);
    void postprocessStateGeometry(const QSharedPointer<StateMachineModel>& model);
    void applyGeometryToState(const QSharedPointer<StateMachineEntity>& entity,
                              const QSharedPointer<StateMachineEntity>& parent,
                              const QSharedPointer<StateMachineModel>& model);
    void postprocessTransitionGeometry(const QSharedPointer<StateMachineModel>& model);
    void postprocessInitialGeometry(const QSharedPointer<StateMachineModel>& model);

private:
    QStack<QPointF> mParentScenePositionStack;
    QMap<EntityID_t, QString> mSceneGeometryStrings;
    QMap<EntityID_t, QString> mGeometryStrings;
    QMap<EntityID_t, QString> mTransitionTargets;
    QString mInitialGeometryString;
};

}  // namespace model

#endif  // QTCREATORMETADATASERIALIZER_HPP
