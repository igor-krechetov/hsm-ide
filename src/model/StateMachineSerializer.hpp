
#ifndef STATEMACHINESERIALIZER_HPP
#define STATEMACHINESERIALIZER_HPP

#include <QSharedPointer>
#include <QString>

#include "private/IModelVisitor.hpp"

class QXmlStreamWriter;

namespace model {
class StateMachineModel;

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

    /**
     * @brief Validates the structure of SCXML content
     * @param scxml The SCXML string to validate
     * @return True if the SCXML is valid, false otherwise
     */
    bool validateScxmlStructure(const QString& scxml);

    // from IModelVisitor
protected:
    void visitRegularState(class RegularState* state) override;
    void visitEntryPoint(class EntryPoint* entryPoint) override;
    void visitExitPoint(class ExitPoint* exitPoint) override;
    void visitFinalState(class FinalState* finalState) override;
    void visitHistoryState(class HistoryState* historyState) override;
    void visitInitialState(class InitialState* initialState) override;
    void visitTransition(class Transition* transition) override;

private:
    /**
     * @brief Helper function to handle parse errors
     * @param errorMessage The error message to log
     */
    void handleParseError(const QString& errorMessage);

private:
    QSharedPointer<QXmlStreamWriter> mXmlWriter;
};

};  // namespace model

#endif  // STATEMACHINESERIALIZER_HPP
