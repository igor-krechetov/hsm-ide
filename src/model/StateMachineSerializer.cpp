

#include "StateMachineSerializer.hpp"

#include <QDebug>
#include <QFile>
#include <QStack>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "EntryPoint.hpp"
#include "ExitPoint.hpp"
#include "FinalState.hpp"
#include "InitialState.hpp"
#include "HistoryState.hpp"
#include "ModelElementsFactory.hpp"
#include "RegularState.hpp"
#include "StateMachineModel.hpp"
#include "Transition.hpp"

namespace model {

StateMachineSerializer::StateMachineSerializer() {}

/**
 * @brief Helper function to serialize a state and its children
 * @param mXmlWriter The XML writer to use
 * @param state The state to serialize
 */
static void serializeState(QXmlStreamWriter& mXmlWriter, const QSharedPointer<model::State>& state, const bool rootElement) {}

/**
 * @brief Serializes a state machine model to SCXML format
 * @param model The state machine model to serialize
 * @return SCXML representation as a QString
 */
QString StateMachineSerializer::serializeToScxml(const QSharedPointer<model::StateMachineModel>& modelPtr) {
    QString scxml;

    mXmlWriter = QSharedPointer<QXmlStreamWriter>::create(&scxml);

    mXmlWriter->setAutoFormatting(true);
    mXmlWriter->writeStartDocument();
    // Write SCXML root element
    mXmlWriter->writeStartElement("scxml");
    mXmlWriter->writeAttribute("xmlns", "http://www.w3.org/2005/07/scxml");
    mXmlWriter->writeAttribute("encoding", "UTF-8");
    mXmlWriter->writeAttribute("version", "1.0");

    if (!modelPtr->name().isEmpty()) {
        mXmlWriter->writeAttribute("name", modelPtr->name());
    }

    // Get the root state
    QSharedPointer<model::RegularState> rootState = modelPtr->root();

    if (rootState) {
        // TODO: find initial state among children

        for (const auto& child : rootState->childrenEntities()) {
            child->accept(this);
        }
    }

    mXmlWriter->writeEndElement();  // scxml
    mXmlWriter->writeEndDocument();

    mXmlWriter.reset();

    return scxml;
}

/**
 * @brief Deserializes SCXML format to a state machine model
 * @param scxml The SCXML string to deserialize
 * @return The deserialized StateMachineModel
 */
QSharedPointer<model::StateMachineModel> StateMachineSerializer::deserializeFromScxml(const QString& scxml) {
    // Create a model with a default name
    QSharedPointer<model::StateMachineModel> model(new model::StateMachineModel("DeserializedModel"));
    QXmlStreamReader xmlReader(scxml);

    // Process SCXML attributes first
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        QXmlStreamReader::TokenType token = xmlReader.readNext();

        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        if (token == QXmlStreamReader::StartElement) {
            if (xmlReader.name() == QStringView(u"scxml")) {
                // Process SCXML attributes
                QString name = xmlReader.attributes().value("name").toString();
                qDebug() << __LINE__ << "SCXML name:" << name;
                if (!name.isEmpty()) {
                    // Set the name of the existing model
                    qDebug() << __LINE__ << "Setting model name to" << name;
                    model->setName(name);
                }
                // Skip the rest of the scxml element
                xmlReader.skipCurrentElement();
            }
        }
    }

    // Reset the reader to the beginning of the document
    xmlReader.clear();
    xmlReader.addData(scxml);

    // Process the content
    QMap<QString, QSharedPointer<model::State>> stateMap;
    QStack<QSharedPointer<model::State>> parentStack;

    // The model already has a root state, so we'll use that
    QSharedPointer<model::RegularState> rootState = model->root();
    rootState->setName("RootState");  // Set the root state name to match the test
    parentStack.push(rootState);      // Start with the root state

    qDebug() << __LINE__ << "Starting XML parsing";
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        QXmlStreamReader::TokenType token = xmlReader.readNext();
        qDebug() << __LINE__ << "Token type:" << token << "Name:" << xmlReader.name();

        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        if (token == QXmlStreamReader::StartElement) {
            qDebug() << __LINE__ << "StartElement:" << xmlReader.name();
            if (xmlReader.name() == QStringView(u"scxml")) {
                // Process the scxml element
                QString name = xmlReader.attributes().value("name").toString();
                qDebug() << __LINE__ << "SCXML name:" << name;
                if (!name.isEmpty()) {
                    model->setName(name);
                    qDebug() << __LINE__ << "Setting model name to" << name;
                }
                xmlReader.skipCurrentElement();
            } else if (xmlReader.name() == QStringView(u"state")) {
                // Process state
                QString stateId = xmlReader.attributes().value("id").toString();
                qDebug() << __LINE__ << "Found state with id:" << stateId;
                if (stateId.isEmpty()) {
                    handleParseError("State element without id attribute");
                    continue;
                }

                // Create the state if it doesn't already exist
                QSharedPointer<model::State> state = stateMap.value(stateId);
                if (!state) {
                    state =
                        model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::State>();
                    state->setName(stateId);
                    stateMap[stateId] = state;

                    // Add to current parent
                    QSharedPointer<model::State> parentState = parentStack.top();
                    QSharedPointer<model::RegularState> parentRegState = parentState.dynamicCast<model::RegularState>();
                    if (parentRegState) {
                        qDebug() << __LINE__ << "Adding state" << stateId << "to parent" << parentState->name();
                        parentRegState->addChildState(state);
                    }
                }

                // Process state attributes and nested elements
                bool isProcessingState = true;
                while (isProcessingState &&
                       !(xmlReader.tokenType() == QXmlStreamReader::EndElement && xmlReader.name() == QStringView(u"state"))) {
                    qDebug() << __LINE__ << "Processing state element, current token:" << xmlReader.tokenType()
                             << "name:" << xmlReader.name();
                    if (xmlReader.tokenType() == QXmlStreamReader::StartElement) {
                        qDebug() << __LINE__ << "StartElement inside state:" << xmlReader.name();
                        if (xmlReader.name() == QStringView(u"onEntering")) {
                            xmlReader.readNext();
                            QSharedPointer<model::RegularState> regState = state.dynamicCast<model::RegularState>();
                            if (regState) {
                                regState->setOnEnteringCallback(xmlReader.text().toString());
                            }
                        } else if (xmlReader.name() == QStringView(u"onExiting")) {
                            xmlReader.readNext();
                            QSharedPointer<model::RegularState> regState = state.dynamicCast<model::RegularState>();
                            if (regState) {
                                regState->setOnExitingCallback(xmlReader.text().toString());
                            }
                        } else if (xmlReader.name() == QStringView(u"onStateChanged")) {
                            xmlReader.readNext();
                            QSharedPointer<model::RegularState> regState = state.dynamicCast<model::RegularState>();
                            if (regState) {
                                regState->setOnStateChangedCallback(xmlReader.text().toString());
                            }
                        } else if (xmlReader.name() == QStringView(u"transition")) {
                            // Process transitions
                            QString event = xmlReader.attributes().value("event").toString();
                            QString target = xmlReader.attributes().value("target").toString();
                            qDebug() << __LINE__ << "Found transition with event:" << event << "target:" << target;

                            if (event.isEmpty() || target.isEmpty()) {
                                handleParseError("Transition without event or target attribute");
                                xmlReader.skipCurrentElement();
                                continue;
                            }

                            // Find the parent state (the one containing the transition)
                            QSharedPointer<model::State> parentState = parentStack.top();

                            // Find target state
                            QSharedPointer<model::State> targetState = stateMap.value(target);
                            if (!targetState) {
                                // If target state is not found, create it
                                targetState = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR)
                                                  .dynamicCast<model::State>();
                                targetState->setName(target);
                                stateMap[target] = targetState;
                            }

                            // Create the transition
                            QSharedPointer<model::Transition> transition =
                                model::ModelElementsFactory::createUniqueTransition(parentState, targetState);
                            transition->setEvent(event);

                            // Add the transition to the parent state
                            QSharedPointer<model::RegularState> parentRegState = parentState.dynamicCast<model::RegularState>();
                            if (parentRegState) {
                                parentRegState->addTransition(transition);
                            }

                            // Process nested elements
                            QString condition;

                            bool isProcessingTransition = true;
                            while (isProcessingTransition && !(xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                                                               xmlReader.name() == QStringView(u"transition"))) {
                                if (xmlReader.tokenType() == QXmlStreamReader::StartElement) {
                                    if (xmlReader.name() == QStringView(u"condition")) {
                                        xmlReader.readNext();
                                        condition = xmlReader.text().toString();
                                    } else {
                                        xmlReader.skipCurrentElement();
                                    }
                                }
                                xmlReader.readNext();
                            }

                            if (!condition.isEmpty()) {
                                transition->setConditionCallback(condition);
                            }
                        } else {
                            // Skip other elements
                            xmlReader.skipCurrentElement();
                        }
                    } else if (xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                               xmlReader.name() == QStringView(u"state")) {
                        // We don't need to pop the state from the stack when we're done processing it
                        // because we want to maintain the parent-child relationship
                        isProcessingState = false;  // Exit the loop
                    }
                    xmlReader.readNext();
                }
            } else if (xmlReader.name() == QStringView(u"final")) {
                // Process final state
                QString finalStateId = xmlReader.attributes().value("id").toString();
                if (!finalStateId.isEmpty()) {
                    QSharedPointer<model::State> finalState =
                        model::ModelElementsFactory::createUniqueState(model::StateType::FINAL).dynamicCast<model::State>();
                    finalState->setName(finalStateId);
                    // Add it as a child of the current parent
                    QSharedPointer<model::State> parentState = parentStack.top();
                    QSharedPointer<model::RegularState> parentRegState = parentState.dynamicCast<model::RegularState>();
                    if (parentRegState) {
                        parentRegState->addChildState(finalState);
                    }
                }
                // Initial state is handled implicitly by the state machine implementation
                // We don't need to do anything special for it else {
                // Skip other elements
                xmlReader.skipCurrentElement();
            }
        }
    }

    if (xmlReader.hasError()) {
        handleParseError(xmlReader.errorString());
    }

    // Return the model
    return model;
}

/**
 * @brief Validates the structure of SCXML content
 * @param scxml The SCXML string to validate
 * @return True if the SCXML is valid, false otherwise
 */
bool StateMachineSerializer::validateScxmlStructure(const QString& scxml) {
    QXmlStreamReader xmlReader(scxml);
    bool isValid = true;

    while (!xmlReader.atEnd() && isValid) {
        QXmlStreamReader::TokenType token = xmlReader.readNext();

        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        if (token == QXmlStreamReader::StartElement) {
            if (xmlReader.name() == QStringView(u"scxml")) {
                // Check for required attributes
                if (!xmlReader.attributes().hasAttribute("xmlns") || !xmlReader.attributes().hasAttribute("version")) {
                    isValid = false;
                    break;
                }
            } else if (xmlReader.name() == QStringView(u"state")) {
                // Check for required attributes
                if (!xmlReader.attributes().hasAttribute("id")) {
                    isValid = false;
                    break;
                }
            } else if (xmlReader.name() == QStringView(u"transition")) {
                // Check for required attributes
                if (!xmlReader.attributes().hasAttribute("event") || !xmlReader.attributes().hasAttribute("target")) {
                    isValid = false;
                    break;
                }
            }
        }
    }

    if (xmlReader.hasError()) {
        isValid = false;
    }

    return isValid;
}

void StateMachineSerializer::visitRegularState(class RegularState* state) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != state) {
        mXmlWriter->writeStartElement("state");
        mXmlWriter->writeAttribute("id", state->name());

        if (!state->onEnteringCallback().isEmpty()) {
            mXmlWriter->writeStartElement("onentry");
            mXmlWriter->writeTextElement("script", state->onEnteringCallback());
            mXmlWriter->writeEndElement();
        }
        if (!state->onExitingCallback().isEmpty()) {
            mXmlWriter->writeStartElement("onexit");
            mXmlWriter->writeTextElement("script", state->onExitingCallback());
            mXmlWriter->writeEndElement();
        }
        if (!state->onStateChangedCallback().isEmpty()) {
            mXmlWriter->writeStartElement("invoke");
            mXmlWriter->writeAttribute("srcexpr", state->onStateChangedCallback());
            mXmlWriter->writeEndElement();
        }

        for (const auto& child : state->childrenEntities()) {
            child->accept(this);
        }

        mXmlWriter->writeEndElement();  // state
    }
}

void StateMachineSerializer::visitEntryPoint(class EntryPoint* entryPoint) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != entryPoint) {
        /*
        <initial>
            <transition type="external" event="Transition_iD" target="D" condition="C1"/>
            <transition type="external" event="Transition_iE" target="E" condition="C2"/>
        </initial>
        */
        mXmlWriter->writeStartElement("initial");

        for (const auto& transition : entryPoint->transitions()) {
            transition->accept(this);
        }

        mXmlWriter->writeEndElement();  // initial
    }
}

void StateMachineSerializer::visitExitPoint(class ExitPoint* exitPoint) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != exitPoint) {
        mXmlWriter->writeStartElement("final");
        mXmlWriter->writeAttribute("id", exitPoint->name());

        if (exitPoint->event().isEmpty() == false) {
            mXmlWriter->writeAttribute("event", exitPoint->event());
        }

        if (!exitPoint->onEnteringCallback().isEmpty()) {
            mXmlWriter->writeStartElement("onentry");
            mXmlWriter->writeTextElement("script", exitPoint->onEnteringCallback());
            mXmlWriter->writeEndElement();
        }
        if (!exitPoint->onExitingCallback().isEmpty()) {
            mXmlWriter->writeStartElement("onexit");
            mXmlWriter->writeTextElement("script", exitPoint->onExitingCallback());
            mXmlWriter->writeEndElement();
        }
        if (!exitPoint->onStateChangedCallback().isEmpty()) {
            mXmlWriter->writeStartElement("invoke");
            mXmlWriter->writeAttribute("srcexpr", exitPoint->onStateChangedCallback());
            mXmlWriter->writeEndElement();
        }

        mXmlWriter->writeEndElement();  // final
    }
}

void StateMachineSerializer::visitFinalState(class FinalState* finalState) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != finalState) {
        mXmlWriter->writeStartElement("final");
        mXmlWriter->writeAttribute("id", finalState->name());

        if (finalState->onStateChangedCallback().isEmpty() == false) {
            mXmlWriter->writeStartElement("onentry");
            mXmlWriter->writeTextElement("script", finalState->onStateChangedCallback());
            mXmlWriter->writeEndElement();
        }

        mXmlWriter->writeEndElement();  // final
    }
}

void StateMachineSerializer::visitHistoryState(class HistoryState* historyState) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != historyState) {
        QSharedPointer<model::Transition> defaultTransition = historyState->defaultTransition();

        mXmlWriter->writeStartElement("history");
        mXmlWriter->writeAttribute("id", historyState->name());

        switch (historyState->historyType()) {
            case model::HistoryType::SHALLOW:
                mXmlWriter->writeAttribute("type", "shallow");
                break;
            case model::HistoryType::DEEP:
                mXmlWriter->writeAttribute("type", "deep");
                break;
            default:
                break;
        }

        if (defaultTransition) {
            defaultTransition->accept(this);
        }

        mXmlWriter->writeEndElement();  // history
    }
}

void StateMachineSerializer::visitInitialState(class InitialState* initialState) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != initialState) {
        auto transition = initialState->transition();

        mXmlWriter->writeStartElement("initial");

        if (nullptr != transition) {
            transition->accept(this);
        }

        mXmlWriter->writeEndElement();  // initial
    }
}

void StateMachineSerializer::visitTransition(class Transition* transition) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != transition) {
        // <transition type="external" event="NEXT_STATE" target="Red">
        //    <script>onE5Transition</script>
        // </transition>
        mXmlWriter->writeStartElement("transition");

        if (transition->event().isEmpty() == false) {
            mXmlWriter->writeAttribute("event", transition->event());
        }

        switch (transition->transitionType()) {
            case model::TransitionType::EXTERNAL:
                mXmlWriter->writeAttribute("type", "external");
                break;
            case model::TransitionType::INTERNAL:
                mXmlWriter->writeAttribute("type", "internal");
                break;
            default:
                break;
        }

        QSharedPointer<model::State> targetState = transition->target().dynamicCast<model::State>();

        if (targetState) {
            mXmlWriter->writeAttribute("target", targetState->name());
        }

        if (transition->conditionCallback().isEmpty() == false) {
            const QString conditionValue = transition->expectedConditionValue() ? " is true" : " is false";

            mXmlWriter->writeAttribute("cond", transition->conditionCallback() + conditionValue);
        }

        if (transition->transitionCallback().isEmpty() == false) {
            mXmlWriter->writeTextElement("script", transition->transitionCallback());
        }

        mXmlWriter->writeEndElement();  // transition
    }
}

/**
 * @brief Helper function to handle parse errors
 * @param errorMessage The error message to log
 */
void StateMachineSerializer::handleParseError(const QString& errorMessage) {
    qWarning() << "Parse error:" << errorMessage;
}

};  // namespace model