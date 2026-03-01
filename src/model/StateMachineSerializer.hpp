
#ifndef STATEMACHINESERIALIZER_HPP
#define STATEMACHINESERIALIZER_HPP

#include <QMap>
#include <QSharedPointer>
#include <QString>

#include "ModelTypes.hpp"
#include "private/IModelVisitor.hpp"

class QXmlStreamWriter;
class QXmlStreamReader;

namespace model {
class StateMachineModel;
class StateMachineEntity;
class RegularState;
class EntryPoint;
class ExitPoint;
class FinalState;
class HistoryState;
class InitialState;
class Transition;

class StateMachineSerializer : public IModelVisitor {
public:
    StateMachineSerializer();
    virtual ~StateMachineSerializer() = default;

    /**
     * @brief Serializes a state machine model to SCXML format
     * @param model The state machine model to serialize
     * @return SCXML representation as a QString
     */
    QString serializeToScxml(const QSharedPointer<model::StateMachineModel>& modelPtr);

    /**
     * @brief Deserializes SCXML format to a state machine model
     * @param scxml The SCXML string to deserialize
     * @return A pointer to the deserialized StateMachineModel
     */
    QSharedPointer<model::StateMachineModel> deserializeFromScxml(const QString& scxml);

    bool deserializeFromScxml(const QString& scxml, QSharedPointer<model::StateMachineModel>& outModel);

    /**
     * @brief Validates the structure of SCXML content
     * @param scxml The SCXML string to validate
     * @return True if the SCXML is valid, false otherwise
     */
    bool validateScxmlStructure(const QString& scxml);

    // from IModelVisitor
protected:
    void visitRegularState(const RegularState* state) override;
    void visitEntryPoint(const EntryPoint* entryPoint) override;
    void visitExitPoint(const ExitPoint* exitPoint) override;
    void visitFinalState(const FinalState* finalState) override;
    void visitHistoryState(const HistoryState* historyState) override;
    void visitInitialState(const InitialState* initialState) override;
    void visitIncludeEntity(const IncludeEntity* include) override;
    void visitTransition(const Transition* transition) override;

protected:
    void serializeEntryMetadata(const StateMachineEntity* entity);
    void deserializeEntryMetadata(StateMachineEntity* entity);

private:
    /**
     * @brief Helper function to handle parse errors
     * @param errorMessage The error message to log
     */
    void handleParseError(const QString& errorMessage);

    bool parseAllChildEntities(const QSharedPointer<StateMachineEntity>& parent);

    QSharedPointer<StateMachineEntity> parseChildEntity(const QSharedPointer<StateMachineEntity>& parent, QSharedPointer<StateMachineEntity>* outNewParent = nullptr);
    QSharedPointer<RegularState> parseRegularState();
    QSharedPointer<EntryPoint> parseEntryPoint();
    QSharedPointer<ExitPoint> parseExitPoint();
    QSharedPointer<FinalState> parseFinalState();
    QSharedPointer<HistoryState> parseHistoryState();
    QSharedPointer<InitialState> parseInitialState();
    QSharedPointer<IncludeEntity> parseIncludeEntity();
    QSharedPointer<Transition> parseTransition();

    QString parseOnEntry();
    QString parseOnExit();
    QString parseInvoke();

    QString tryGetElementAttribute(const QString& name);

private:
    QSharedPointer<QXmlStreamWriter> mXmlWriter;
    QSharedPointer<QXmlStreamReader> mXmlReader;

    QMap<EntityID_t, QString> mTransitionTargets;
    QSharedPointer<model::StateMachineModel> mModel;
};

};  // namespace model

#endif  // STATEMACHINESERIALIZER_HPP
