#include "StateMachineSerializer.hpp"

#include <QDebug>
#include <QFile>
#include <QPolygonF>
#include <QStack>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ModelElementsFactory.hpp"
#include "ModelUtils.hpp"
#include "StateHierarchyRules.hpp"
#include "StateMachineModel.hpp"
#include "actions/ModelActionFactory.hpp"
#include "elements/EntryPoint.hpp"
#include "elements/ExitPoint.hpp"
#include "elements/FinalState.hpp"
#include "elements/HistoryState.hpp"
#include "elements/IncludeEntity.hpp"
#include "elements/InitialState.hpp"
#include "elements/ModelRootState.hpp"
#include "elements/RegularState.hpp"
#include "elements/Transition.hpp"

namespace model {

StateMachineSerializer::StateMachineSerializer()
    : mActiveStrategy(&mHsmStrategy) {}

IMetadataSerializer* StateMachineSerializer::selectStrategy(SerializationFormat format) {
    IMetadataSerializer* result = &mHsmStrategy;

    if (format == SerializationFormat::QtCreator) {
        result = &mQtStrategy;
    }

    return result;
}

IMetadataSerializer* StateMachineSerializer::selectStrategyForDetectedFormat(DetectedFormat detected) {
    IMetadataSerializer* result = &mQtStrategy;  // default for PlainSCXML and QtCreator

    if (detected == DetectedFormat::HSM) {
        result = &mHsmStrategy;
    }

    return result;
}

// ============================================================================
// Serialization
// ============================================================================

QString StateMachineSerializer::serializeToScxml(const QSharedPointer<model::StateMachineModel>& modelPtr,
                                                 const SerializationFormat format,
                                                 const bool addScxmlTag) {
    QString scxml;

    mActiveStrategy = selectStrategy(format);
    mXmlWriter = QSharedPointer<QXmlStreamWriter>::create(&scxml);

    mXmlWriter->setAutoFormatting(true);

    if (addScxmlTag) {
        mXmlWriter->writeStartDocument();
    }

    if (addScxmlTag) {
        mXmlWriter->writeStartElement("scxml");
        mXmlWriter->writeAttribute("encoding", "UTF-8");
        mXmlWriter->writeAttribute("version", "1.0");
        mXmlWriter->writeAttribute("xmlns", "http://www.w3.org/2005/07/scxml");
        mXmlWriter->writeAttribute("xmlns:xi", "http://www.w3.org/2001/XInclude");

        mActiveStrategy->beginSerialization(*mXmlWriter, modelPtr);

        if (!modelPtr->name().isEmpty()) {
            mXmlWriter->writeAttribute("name", modelPtr->name());
        }
    }

    QSharedPointer<model::ModelRootState> rootState = modelPtr->root();

    if (rootState) {
        for (const auto& child : rootState->childrenEntities()) {
            child->accept(this);
        }
    }

    if (addScxmlTag) {
        mActiveStrategy->endSerialization(*mXmlWriter, modelPtr);
        mXmlWriter->writeEndElement();  // scxml
    }

    if (addScxmlTag) {
        mXmlWriter->writeEndDocument();
    }

    mXmlWriter.reset();

    return scxml;
}

// ============================================================================
// Deserialization
// ============================================================================

QSharedPointer<model::StateMachineModel> StateMachineSerializer::deserializeFromScxml(const QString& scxml) {
    QSharedPointer<model::StateMachineModel> resModel =
        QSharedPointer<model::StateMachineModel>(new model::StateMachineModel("DeserializedModel"));

    if (false == deserializeFromScxml(scxml, resModel)) {
        resModel.clear();
    }

    return resModel;
}

bool StateMachineSerializer::deserializeFromUnwrapperScxml(const QString& unwrappedScxml,
                                                           const QString& stateWrapper,
                                                           QSharedPointer<model::StateMachineModel>& outModel) {
    QString wrappedScxml = unwrappedScxml.trimmed();

    if ((wrappedScxml.isEmpty() == false) && (wrappedScxml.contains("<scxml") == false)) {
        wrappedScxml = QString(
                           "<scxml version=\"1.0\" xmlns=\"http://www.w3.org/2005/07/scxml\" "
                           "xmlns:xi=\"http://www.w3.org/2001/XInclude\" xmlns:qt = \"http://www.qt.io/2015/02/scxml-ext\">"
                           "<state id=\"%1\">%2</state>"
                           "</scxml>")
                           .arg(stateWrapper)
                           .arg(wrappedScxml);
    }

    return deserializeFromScxml(wrappedScxml, outModel);
}

bool StateMachineSerializer::deserializeFromScxml(const QString& scxml, QSharedPointer<model::StateMachineModel>& outModel) {
    // NOTE: block all signals because it's a full rebuild of the model
    QSignalBlocker bloker(outModel.get());

    mModel = outModel;
    mModel->clearModel();
    mTransitionTargets.clear();
    mActiveStrategy = &mQtStrategy;  // default until format detected
    mXmlReader = QSharedPointer<QXmlStreamReader>::create(scxml);

    while (!mXmlReader->atEnd() && !mXmlReader->hasError()) {
        QXmlStreamReader::TokenType token = mXmlReader->readNext();
        qDebug() << __LINE__ << "Token type:" << mXmlReader->tokenString() << "Name:" << mXmlReader->name();

        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        if (token == QXmlStreamReader::StartElement) {
            qDebug() << __LINE__ << "StartElement:" << mXmlReader->name();

            if (mXmlReader->name() == QStringView(u"scxml")) {
                QString name = mXmlReader->attributes().value("name").toString();
                QString initialTarget = mXmlReader->attributes().value("initial").toString();
                qDebug() << __LINE__ << "SCXML name:" << name;

                // Detect format from namespace declarations and select strategy
                DetectedFormat detected = mHsmStrategy.detectFormat(*mXmlReader);

                if (detected == DetectedFormat::PlainSCXML) {
                    detected = mQtStrategy.detectFormat(*mXmlReader);
                }

                mActiveStrategy = selectStrategyForDetectedFormat(detected);

                if (!name.isEmpty()) {
                    mModel->setName(name);
                }

                if (!initialTarget.isEmpty()) {
                    mInitialTargetFromAttribute = initialTarget;
                }
            } else if (mActiveStrategy->parseTopLevelElement(*mXmlReader, mModel)) {
                // Strategy consumed the element (e.g., <hsm:editor>)
            } else {
                QSharedPointer<StateMachineEntity> entity = parseChildEntity(mModel->root());

                if (!entity) {
                    qWarning() << "Failed to parse entity at line" << mXmlReader->lineNumber();
                }
            }
        }
    }

    if (mXmlReader->hasError()) {
        handleParseError(mXmlReader->errorString());
    }

    // Create InitialState from scxml initial="" attribute if no explicit <initial> was parsed at root level
    if (!mInitialTargetFromAttribute.isEmpty()) {
        bool hasRootInitial = false;

        mModel->root()->forEachChildElement(
            [&hasRootInitial](QSharedPointer<StateMachineEntity> /*parent*/, QSharedPointer<StateMachineEntity> child) {
                bool continueTraversal = true;

                if (child->type() == StateMachineEntity::Type::State) {
                    auto state = child.dynamicCast<State>();

                    if (state && state->stateType() == StateType::INITIAL) {
                        hasRootInitial = true;
                        continueTraversal = false;
                    }
                }

                return continueTraversal;
            },
            1,
            false);

        if (!hasRootInitial) {
            QSharedPointer<State> targetState = mModel->root()->findChildStateByName(mInitialTargetFromAttribute);

            if (targetState) {
                auto initialState = ModelElementsFactory::createUniqueState(StateType::INITIAL, mModel->idGenerator());
                auto transition =
                    ModelElementsFactory::createUniqueTransition(initialState, targetState, mModel->idGenerator());

                if (initialState && transition) {
                    mModel->root()->addChild(initialState);
                }
            }
        }
    }

    mActiveStrategy->endDeserialization(mModel);

    // Set targets for parsed transitions. We do this after all states are created
    // because transitions can reference states that are defined later in the SCXML.
    for (const EntityID_t transitionId : mTransitionTargets.keys()) {
        QString targetStateId = mTransitionTargets.value(transitionId);
        QSharedPointer<model::State> targetState = mModel->root()->findChildStateByName(targetStateId);
        QSharedPointer<model::Transition> transition = mModel->root()->findTransition(transitionId);

        if (transition && targetState) {
            transition->setTarget(targetState);
        } else {
            qWarning() << "Failed to find target (" << targetStateId << ") for transition" << transitionId;
            // TODO: handleParseError
        }
    }

    mInitialTargetFromAttribute.clear();
    mModel.clear();
    // TODO: handle parsing errors
    return true;
}

// ============================================================================
// Validation
// ============================================================================

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

// ============================================================================
// Visitor methods (format-agnostic SCXML structure)
// ============================================================================

#define SCXML_SERIALIZE_ACTION(_object, _hasAction, _actionGetter, _element, _attr)     \
    if ((_object)->_hasAction()) {                                                      \
        mXmlWriter->writeStartElement(_element);                                        \
        mXmlWriter->writeTextElement((_attr), (_object)->_actionGetter()->serialize()); \
        mXmlWriter->writeEndElement();                                                  \
    }

#define SCXML_SERIALIZE_ACTION_ATTR(_object, _hasAction, _actionGetter, _element, _attr) \
    if ((_object)->_hasAction()) {                                                       \
        mXmlWriter->writeStartElement(_element);                                         \
        mXmlWriter->writeAttribute((_attr), (_object)->_actionGetter()->serialize());    \
        mXmlWriter->writeEndElement();                                                   \
    }

#define SCXML_SERIALIZE_STATE_ACTIONS(_object)                                                                       \
    serializeActionListElement((_object)->onEnteringActions(), QStringLiteral("onentry"), QStringLiteral("script")); \
    serializeActionListElement((_object)->onExitingActions(), QStringLiteral("onexit"), QStringLiteral("script"));   \
    SCXML_SERIALIZE_ACTION_ATTR(_object, hasOnStateChangedAction, onStateChangedAction, "invoke", "srcexpr")

void StateMachineSerializer::serializeActionListElement(const ModelActionList& actions,
                                                        const QString& wrapper,
                                                        const QString& childTag) {
    if (actions.isEmpty() == false) {
        mXmlWriter->writeStartElement(wrapper);

        for (const auto& action : actions) {
            if (action && (action->type() != ModelAction::NONE)) {
                mXmlWriter->writeTextElement(childTag, action->serialize());
            }
        }

        mXmlWriter->writeEndElement();
    }
}

void StateMachineSerializer::visitRegularState(const RegularState* state) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != state) {
        mXmlWriter->writeStartElement("state");
        mXmlWriter->writeAttribute("id", state->name());

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, state);
        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, state);
        SCXML_SERIALIZE_STATE_ACTIONS(state);

        mActiveStrategy->notifyEnterState(state);

        for (const auto& child : state->childrenEntities()) {
            child->accept(this);
        }

        mActiveStrategy->notifyExitState(state);
        mXmlWriter->writeEndElement();  // state
    }
}

void StateMachineSerializer::visitEntryPoint(const EntryPoint* entryPoint) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != entryPoint) {
        /*
        <initial>
            <transition type="external" event="Transition_iD" target="D" condition="C1"/>
            <transition type="external" event="Transition_iE" target="E" condition="C2"/>
        </initial>
        */
        mXmlWriter->writeStartElement("initial");

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, entryPoint);
        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, entryPoint);

        for (const auto& transition : entryPoint->transitions()) {
            transition->accept(this);
        }

        mXmlWriter->writeEndElement();  // initial
    }
}

void StateMachineSerializer::visitExitPoint(const ExitPoint* exitPoint) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != exitPoint) {
        mXmlWriter->writeStartElement("final");
        mXmlWriter->writeAttribute("id", exitPoint->name());

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, exitPoint);

        if (exitPoint->event().isEmpty() == false) {
            mXmlWriter->writeAttribute("event", exitPoint->event());
        }

        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, exitPoint);
        SCXML_SERIALIZE_STATE_ACTIONS(exitPoint);

        mXmlWriter->writeEndElement();  // final
    }
}

void StateMachineSerializer::visitFinalState(const FinalState* finalState) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != finalState) {
        mXmlWriter->writeStartElement("final");
        mXmlWriter->writeAttribute("id", finalState->name());

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, finalState);

        if (finalState->event().isEmpty() == false) {
            mXmlWriter->writeAttribute("event", finalState->event());
        }

        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, finalState);
        SCXML_SERIALIZE_ACTION(finalState, hasOnStateChangedAction, onStateChangedAction, "onentry", "script");

        mXmlWriter->writeEndElement();  // final
    }
}

void StateMachineSerializer::visitHistoryState(const HistoryState* historyState) {
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

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, historyState);
        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, historyState);

        if (defaultTransition) {
            defaultTransition->accept(this);
        }

        mXmlWriter->writeEndElement();  // history
    }
}

void StateMachineSerializer::visitInitialState(const InitialState* initialState) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != initialState) {
        auto transition = initialState->transition();

        mXmlWriter->writeStartElement("initial");

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, initialState);
        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, initialState);

        if (nullptr != transition) {
            transition->accept(this);
        }

        mXmlWriter->writeEndElement();  // initial
    }
}

void StateMachineSerializer::visitIncludeEntity(const IncludeEntity* include) {
    qDebug() << Q_FUNC_INFO;

    if (nullptr != include) {
        mXmlWriter->writeStartElement("state");
        mXmlWriter->writeAttribute("id", include->name());

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, include);

        SCXML_SERIALIZE_STATE_ACTIONS(include);

        for (const auto& child : include->childrenEntities()) {
            child->accept(this);
        }

        mXmlWriter->writeStartElement("xi:include");
        mXmlWriter->writeAttribute("href", include->path());
        mXmlWriter->writeAttribute("parse", "xml");
        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, include);
        mXmlWriter->writeEndElement();  // xi:include

        mXmlWriter->writeEndElement();  // state
    }
}

void StateMachineSerializer::visitTransition(const Transition* transition) {
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

        AttributeWriter attrWriter(*mXmlWriter);
        mActiveStrategy->writeEntityAttributes(attrWriter, transition);
        mActiveStrategy->writeEntityChildMetadata(*mXmlWriter, transition);

        if (transition->hasTransitionAction()) {
            for (const auto& action : transition->transitionActions()) {
                if (action && (action->type() != ModelAction::NONE)) {
                    mXmlWriter->writeTextElement("script", action->serialize());
                }
            }
        }

        mXmlWriter->writeEndElement();  // transition
    }
}

// ============================================================================
// Deserialization helpers
// ============================================================================

void StateMachineSerializer::handleParseError(const QString& errorMessage) {
    qWarning() << "Parse error:" << errorMessage;
}

bool StateMachineSerializer::parseAllChildEntities(const QSharedPointer<StateMachineEntity>& parent) {
    bool res = false;

    QXmlStreamReader::TokenType token = mXmlReader->readNext();

    while (!mXmlReader->atEnd() && !mXmlReader->hasError() && (QXmlStreamReader::EndElement != token)) {
        if (QXmlStreamReader::StartElement == token) {
            // TODO: handle errors if can't parse child entities
            parseChildEntity(parent);
        }

        token = mXmlReader->readNext();
    }

    return res;
}

QSharedPointer<StateMachineEntity> StateMachineSerializer::parseChildEntity(const QSharedPointer<StateMachineEntity>& parent,
                                                                            QSharedPointer<StateMachineEntity>* outNewParent) {
    qDebug() << "parseChildEntity:" << mXmlReader->name();
    QSharedPointer<StateMachineEntity> entity;

    if (mXmlReader->name() == QStringView(u"state")) {
        entity = parseRegularState();
    } else if (mXmlReader->name() == QStringView(u"initial")) {
        if (parent == mModel->root()) {
            // Root final state
            entity = parseInitialState();
        } else {
            entity = parseEntryPoint();
        }
    } else if (mXmlReader->name() == QStringView(u"history")) {
        entity = parseHistoryState();
    } else if (mXmlReader->name() == QStringView(u"transition")) {
        entity = parseTransition();
    } else if (mXmlReader->name() == QStringView(u"final")) {
        if (parent == mModel->root()) {
            // Root final state
            entity = parseFinalState();
        } else {
            entity = parseExitPoint();
        }
    } else if (mXmlReader->name() == QStringView(u"include")) {
        entity = parseIncludeEntity();

        QSharedPointer<RegularState> ptrParentState = hsmDynamicCast<RegularState>(parent, StateType::REGULAR);
        QSharedPointer<IncludeEntity> ptrInclude = hsmDynamicCast<IncludeEntity>(entity, StateType::INCLUDE);

        // since Include elements are defined as children of a State, we need to merge them
        if (ptrInclude && ptrParentState) {
            // promote existing RegularState element to IncludeEntity
            ptrInclude->promoteFrom(ptrParentState);

            // release RegularState instance and replace pointer with IncludeEntity
            *outNewParent = ptrInclude;
        }
    } else if (mXmlReader->name() == QStringView(u"script")) {
        if (parent->type() == StateMachineEntity::Type::Transition) {
            QSharedPointer<Transition> ptrParent = parent.dynamicCast<Transition>();

            if (ptrParent) {
                ptrParent->addTransitionAction(
                    ModelActionFactory::createModelActionFromData(parseScript(), ModelAction::CALLBACK));
            } else {
                qWarning() << "script elements outside of transition nodes are not supported. skipping";
            }
        }
    } else if (mActiveStrategy->parseMetadataElement(*mXmlReader, parent.get(), mModel)) {
        // Strategy consumed the element (e.g., qt:editorinfo)
    } else {
        // Unknown element — skip it to keep the parser advancing
        mXmlReader->skipCurrentElement();
    }
    // TODO: parallel

    if (entity && ((nullptr == outNewParent) || outNewParent->isNull())) {
        QSharedPointer<Transition> transition = hsmDynamicCast<Transition>(entity);

        if (transition) {
            transition->setSource(parent.dynamicCast<State>());
        }

        if (StateHierarchyRules::canAddEntityToParent(parent, entity)) {
            parent->addChild(entity);
        } else {
            qWarning() << "Ignoring invalid hierarchy. Parent type=" << static_cast<int>(parent->type())
                       << "child type=" << static_cast<int>(entity->type());
        }
    }

    return entity;
}

QSharedPointer<RegularState> StateMachineSerializer::parseRegularState() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<RegularState> entity;

    // NOTE: if a state contains xi:include it should be parsed as IncludeEntity

    // Process state
    QString stateId = tryGetElementAttribute("id");
    qDebug() << __LINE__ << "Found state with id:" << stateId;

    if (stateId.isEmpty() == false) {
        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity =
            model::ModelElementsFactory::createStateWithId(model::StateType::REGULAR, uid).dynamicCast<model::RegularState>();
        entity->setName(stateId);

        QXmlStreamReader::TokenType token = mXmlReader->readNext();

        while (!mXmlReader->atEnd() && !mXmlReader->hasError() && (QXmlStreamReader::EndElement != token)) {
            if (QXmlStreamReader::StartElement == token) {
                qDebug() << __LINE__ << "Found" << mXmlReader->name();
                if (mXmlReader->name() == QStringView(u"onentry")) {
                    entity->setOnEnteringActions(parseOnEntry());
                } else if (mXmlReader->name() == QStringView(u"onexit")) {
                    entity->setOnExitingActions(parseOnExit());
                } else if (mXmlReader->name() == QStringView(u"invoke")) {
                    entity->setOnStateChangedAction(parseInvoke());
                } else {
                    // TODO: handle errors if can't parse child entities
                    QSharedPointer<StateMachineEntity> newParent;
                    QSharedPointer<StateMachineEntity> childEntity = parseChildEntity(entity, &newParent);

                    if (newParent) {
                        // TODO: better casting
                        entity = newParent.dynamicCast<model::RegularState>();
                    }
                }
            }

            token = mXmlReader->readNext();
        }
    } else {
        handleParseError("State element without id attribute");
    }

    return entity;
}

QSharedPointer<EntryPoint> StateMachineSerializer::parseEntryPoint() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<EntryPoint> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"initial"))) {
        entity = QSharedPointer<EntryPoint>::create();

        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity->setId(uid);

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    return entity;
}

QSharedPointer<ExitPoint> StateMachineSerializer::parseExitPoint() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<ExitPoint> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"final"))) {
        const QString stateId = mXmlReader->attributes().value("id").toString();
        const QString eventAttr = tryGetElementAttribute("event");

        entity = QSharedPointer<ExitPoint>::create(stateId);
        entity->setEvent(eventAttr);

        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity->setId(uid);

        QXmlStreamReader::TokenType token = mXmlReader->readNext();

        while (!mXmlReader->atEnd() && !mXmlReader->hasError() && (QXmlStreamReader::EndElement != token)) {
            if (QXmlStreamReader::StartElement == token) {
                if (mXmlReader->name() == QStringView(u"onentry")) {
                    entity->setOnEnteringActions(parseOnEntry());
                } else if (mXmlReader->name() == QStringView(u"onexit")) {
                    entity->setOnExitingActions(parseOnExit());
                } else if (mXmlReader->name() == QStringView(u"invoke")) {
                    entity->setOnStateChangedAction(parseInvoke());
                } else {
                    // TODO: handle errors if can't parse child entities
                    parseChildEntity(entity);
                }
            }

            token = mXmlReader->readNext();
        }
    }

    return entity;
}

QSharedPointer<FinalState> StateMachineSerializer::parseFinalState() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<FinalState> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"final"))) {
        const QString stateId = mXmlReader->attributes().value("id").toString();
        const QString eventAttr = tryGetElementAttribute("event");

        entity = QSharedPointer<FinalState>::create(stateId);
        entity->setEvent(eventAttr);

        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity->setId(uid);

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    return entity;
}

QSharedPointer<HistoryState> StateMachineSerializer::parseHistoryState() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<HistoryState> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"history"))) {
        const QString stateId = mXmlReader->attributes().value("id").toString();
        HistoryType historyType = HistoryType::INVALID;
        const QString typeAttr = mXmlReader->attributes().value("type").toString();

        if (typeAttr == "deep") {
            historyType = HistoryType::DEEP;
        } else if (typeAttr == "shallow") {
            historyType = HistoryType::SHALLOW;
        } else {
            handleParseError("Invalid history state type attribute: " + typeAttr);
        }

        if (historyType != HistoryType::INVALID) {
            entity = QSharedPointer<HistoryState>::create(stateId, historyType);

            EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
            entity->setId(uid);

            // TODO: handle errors
            parseAllChildEntities(entity);
        }
    }

    return entity;
}

QSharedPointer<InitialState> StateMachineSerializer::parseInitialState() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<InitialState> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"initial"))) {
        entity = QSharedPointer<InitialState>::create();

        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity->setId(uid);

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    return entity;
}

QSharedPointer<IncludeEntity> StateMachineSerializer::parseIncludeEntity() {
    QSharedPointer<IncludeEntity> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"include"))) {
        entity = QSharedPointer<IncludeEntity>::create("");

        entity->setPath(tryGetElementAttribute("href"));

        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity->setId(uid);

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    return entity;
}

QSharedPointer<Transition> StateMachineSerializer::parseTransition() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<Transition> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"transition"))) {
        QString event = tryGetElementAttribute("event");
        QString target = tryGetElementAttribute("target");

        qDebug() << __LINE__ << "Found transition with event:" << event << "target:" << target;

        if (target.isEmpty()) {
            handleParseError("Transition without target attribute");
            mXmlReader->skipCurrentElement();
            return nullptr;
        }

        entity = QSharedPointer<Transition>::create(nullptr, nullptr, event);

        EntityID_t uid = mActiveStrategy->resolveEntityId(*mXmlReader, mModel);
        entity->setId(uid);

        mTransitionTargets.insert(entity->id(), target);

        // condition is defined as "callback is true|false"
        QString condition = tryGetElementAttribute("cond");
        QString type = tryGetElementAttribute("type");

        if (type == "external") {
            entity->setTransitionType(TransitionType::EXTERNAL);
        } else if (type == "internal") {
            entity->setTransitionType(TransitionType::INTERNAL);
        }

        if (condition.isEmpty() == false) {
            // TODO: account for multiple spaces and formatting issues
            if (condition.endsWith(" is true")) {
                condition.chop(QString(" is true").length());
                entity->setExpectedConditionValue(true);
            } else if (condition.endsWith(" is false")) {
                condition.chop(QString(" is false").length());
                entity->setExpectedConditionValue(false);
            }

            entity->setConditionCallback(condition);
        }

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    qDebug() << "current element" << mXmlReader->name() << tryGetElementAttribute("id");

    if (mXmlReader->tokenType() != QXmlStreamReader::EndElement) {
        qDebug() << "skip the rest of <transition> node";
        mXmlReader->skipCurrentElement();  // skip the rest of <transition> node
    }

    qDebug() << "current element" << mXmlReader->name() << tryGetElementAttribute("id");

    return entity;
}

ModelActionList StateMachineSerializer::parseOnEntry() {
    qDebug() << Q_FUNC_INFO;
    ModelActionList actions;

    // Keep reading until we reach the closing </onentry> tag, collecting every <script>
    while (!(mXmlReader->tokenType() == QXmlStreamReader::EndElement && mXmlReader->name() == QStringView(u"onentry"))) {
        mXmlReader->readNext();

        if (mXmlReader->tokenType() == QXmlStreamReader::StartElement) {
            if (mXmlReader->name() == QStringView(u"script")) {
                const QString content = mXmlReader->readElementText();

                if (content.isEmpty() == false) {
                    actions.append(ModelActionFactory::createModelActionFromData(content, ModelAction::CALLBACK));
                }
            } else {
                // Skip entire element (and its children) that isn't <script>
                mXmlReader->skipCurrentElement();
            }
        }

        if (mXmlReader->atEnd() || mXmlReader->hasError()) {
            break;
        }
    }

    if (actions.isEmpty()) {
        handleParseError("onentry element without script or with invalid content");
    }

    return actions;
}

// TODO: unify with parseOnEntry
ModelActionList StateMachineSerializer::parseOnExit() {
    qDebug() << Q_FUNC_INFO;
    ModelActionList actions;

    // Keep reading until we reach the closing </onexit> tag, collecting every <script>
    while (!(mXmlReader->tokenType() == QXmlStreamReader::EndElement && mXmlReader->name() == QStringView(u"onexit"))) {
        mXmlReader->readNext();

        if (mXmlReader->tokenType() == QXmlStreamReader::StartElement) {
            if (mXmlReader->name() == QStringView(u"script")) {
                const QString content = mXmlReader->readElementText();

                if (content.isEmpty() == false) {
                    actions.append(ModelActionFactory::createModelActionFromData(content, ModelAction::CALLBACK));
                }
            } else {
                // Skip entire element (and its children) that isn't <script>
                mXmlReader->skipCurrentElement();
            }
        }

        if (mXmlReader->atEnd() || mXmlReader->hasError()) {
            break;
        }
    }

    if (actions.isEmpty()) {
        handleParseError("onexit element without script or with invalid content");
    }

    return actions;
}

QString StateMachineSerializer::parseScript() {
    qDebug() << "parseScript:" << mXmlReader->name();
    QString content;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"script"))) {
        content = mXmlReader->readElementText();

        if (content.isEmpty()) {
            handleParseError("script element without content");
        }
    }

    return content;
}

QString StateMachineSerializer::parseInvoke() {
    qDebug() << "parseInvoke:" << mXmlReader->name();
    QString content;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"invoke"))) {
        content = tryGetElementAttribute("srcexpr");

        if (content.isEmpty() == true) {
            content = tryGetElementAttribute("src");
        }

        // Keep reading until we reach the closing </invoke> tag
        while (!(mXmlReader->tokenType() == QXmlStreamReader::EndElement && mXmlReader->name() == QStringView(u"invoke"))) {
            mXmlReader->readNext();

            if (mXmlReader->atEnd() || mXmlReader->hasError()) {
                break;
            }
        }

        if (content.isEmpty() == true) {
            handleParseError("invoke element without script or with invalid content");
        }
    }

    return content;
}

QString StateMachineSerializer::tryGetElementAttribute(const QString& name) {
    QString attributeValue;

    if (mXmlReader->attributes().hasAttribute(name) == true) {
        attributeValue = mXmlReader->attributes().value(name).toString();
    }

    return attributeValue;
}

};  // namespace model
