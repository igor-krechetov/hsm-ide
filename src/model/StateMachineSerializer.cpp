#include "StateMachineSerializer.hpp"

#include <QDebug>
#include <QFile>
#include <QPolygonF>
#include <QStack>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "EntryPoint.hpp"
#include "ExitPoint.hpp"
#include "FinalState.hpp"
#include "HistoryState.hpp"
#include "IncludeEntity.hpp"
#include "InitialState.hpp"
#include "ModelElementsFactory.hpp"
#include "ModelRootState.hpp"
#include "ModelUtils.hpp"
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
    mXmlWriter->writeAttribute("encoding", "UTF-8");
    mXmlWriter->writeAttribute("version", "1.0");
    mXmlWriter->writeAttribute("xmlns", "http://www.w3.org/2005/07/scxml");
    mXmlWriter->writeAttribute("xmlns:xi", "http://www.w3.org/2001/XInclude");
    mXmlWriter->writeAttribute("xmlns:qt", "http://www.qt.io/2015/02/scxml-ext");

    if (!modelPtr->name().isEmpty()) {
        mXmlWriter->writeAttribute("name", modelPtr->name());
    }

    // Get the root state
    QSharedPointer<model::ModelRootState> rootState = modelPtr->root();

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
    QSharedPointer<model::StateMachineModel> resModel =
        QSharedPointer<model::StateMachineModel>(new model::StateMachineModel("DeserializedModel"));

    if (false == deserializeFromScxml(scxml, resModel)) {
        resModel.clear();
    }

    return resModel;
}

bool StateMachineSerializer::deserializeFromScxml(const QString& scxml, QSharedPointer<model::StateMachineModel>& outModel) {
    // NOTE: block all signals because it's a full rebuild of the model
    QSignalBlocker bloker(outModel.get());

    mModel = outModel;
    mModel->clearModel();
    mXmlReader = QSharedPointer<QXmlStreamReader>::create(scxml);

    while (!mXmlReader->atEnd() && !mXmlReader->hasError()) {
        QXmlStreamReader::TokenType token = mXmlReader->readNext();
        qDebug() << __LINE__ << "Token type:" << token << "Name:" << mXmlReader->name();

        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        if (token == QXmlStreamReader::StartElement) {
            qDebug() << __LINE__ << "StartElement:" << mXmlReader->name();
            if (mXmlReader->name() == QStringView(u"scxml")) {
                // TODO: check if "name" attribute exists
                QString name = mXmlReader->attributes().value("name").toString();
                qDebug() << __LINE__ << "SCXML name:" << name;

                if (!name.isEmpty()) {
                    mModel->setName(name);
                }
            } else {
                QSharedPointer<StateMachineEntity> entity = parseChildEntity(mModel->root());

                if (!entity) {
                    qWarning() << "Failed to parse entity at line" << mXmlReader->lineNumber();
                    mXmlReader->skipCurrentElement();  // DO we need this?
                }
            }
        }
    }

    if (mXmlReader->hasError()) {
        handleParseError(mXmlReader->errorString());
    }

    // Set targets for parsed transitions. We do this after all states are created.
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

    mModel.clear();
    // TODO: handle parsing errors
    return true;
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

#define SCXML_SERIALIZE_CALLBACK(_object, _cbGetter, _element, _attr) \
  if (!(_object)->_cbGetter().isEmpty()) {                            \
    mXmlWriter->writeStartElement(_element);                          \
    mXmlWriter->writeTextElement((_attr), (_object)->_cbGetter());    \
    mXmlWriter->writeEndElement();                                    \
  }

#define SCXML_SERIALIZE_STATE_CALLBACKS(_object)                             \
  SCXML_SERIALIZE_CALLBACK(_object, onEnteringCallback, "onentry", "script") \
  SCXML_SERIALIZE_CALLBACK(_object, onExitingCallback, "onexit", "script")   \
  SCXML_SERIALIZE_CALLBACK(_object, onStateChangedCallback, "invoke", "srcexpr")

void StateMachineSerializer::visitRegularState(const RegularState* state) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != state) {
        mXmlWriter->writeStartElement("state");
        mXmlWriter->writeAttribute("id", state->name());

        serializeEntryMetadata(state);
        SCXML_SERIALIZE_STATE_CALLBACKS(state);

        for (const auto& child : state->childrenEntities()) {
            child->accept(this);
        }

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
        serializeEntryMetadata(entryPoint);

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
        serializeEntryMetadata(exitPoint);

        if (exitPoint->event().isEmpty() == false) {
            mXmlWriter->writeAttribute("event", exitPoint->event());
        }

        SCXML_SERIALIZE_STATE_CALLBACKS(exitPoint);

        mXmlWriter->writeEndElement();  // final
    }
}

void StateMachineSerializer::visitFinalState(const FinalState* finalState) {
    qDebug() << Q_FUNC_INFO;
    if (nullptr != finalState) {
        mXmlWriter->writeStartElement("final");
        mXmlWriter->writeAttribute("id", finalState->name());
        serializeEntryMetadata(finalState);

        SCXML_SERIALIZE_CALLBACK(finalState, onStateChangedCallback, "onentry", "script");

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

        serializeEntryMetadata(historyState);

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
        serializeEntryMetadata(initialState);

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

        SCXML_SERIALIZE_STATE_CALLBACKS(include);

        for (const auto& child : include->childrenEntities()) {
            child->accept(this);
        }

        mXmlWriter->writeStartElement("xi:include");
        mXmlWriter->writeAttribute("href", include->path());
        mXmlWriter->writeAttribute("parse", "xml");
        serializeEntryMetadata(include);
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

        serializeEntryMetadata(transition);

        if (transition->transitionCallback().isEmpty() == false) {
            mXmlWriter->writeTextElement("script", transition->transitionCallback());
        }

        mXmlWriter->writeEndElement();  // transition
    }
}

void StateMachineSerializer::serializeEntryMetadata(const StateMachineEntity* entity) {
    qDebug() << "serializeEntryMetadata" << entity->id() << entity->getPos();

    if (entity->type() == StateMachineEntity::Type::Transition) {
        // for transitions
        // <qt:editorinfo localGeometry="0;117.63;238.86;117.63"/>
        QVariant geometryData = entity->getMetadata(StateMachineEntity::MetadataKey::GEOMETRY);

        if (geometryData.isValid()) {
            QPolygonF linePath = geometryData.value<QPolygonF>();
            QStringList pointStrings;

            // remove first and last points
            if (linePath.size() >= 2) {
                linePath.removeFirst();
                linePath.removeLast();
            }

            for (const QPointF& point : linePath) {
                pointStrings.append(
                    QString("%1;%2").arg(QString::number(point.x(), 'f', 2)).arg(QString::number(point.y(), 'f', 2)));
            }

            if (pointStrings.isEmpty() == false) {
                mXmlWriter->writeStartElement("qt:editorinfo");
                mXmlWriter->writeAttribute("localGeometry", pointStrings.join(';'));
                mXmlWriter->writeEndElement();
            }
        }
    } else {
        // for states
        // Store as string with 2 decimal places
        QString geometryValue =
            QString("%1;%2;0;0;%3;%4")
                .arg(QString::number(entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 'f', 2))
                .arg(QString::number(entity->getMetadata(StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 'f', 2))
                .arg(QString::number(entity->getMetadata(StateMachineEntity::MetadataKey::WIDTH).toDouble(), 'f', 2))
                .arg(QString::number(entity->getMetadata(StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 'f', 2));
        mXmlWriter->writeStartElement("qt:editorinfo");
        mXmlWriter->writeAttribute("geometry", geometryValue);
        mXmlWriter->writeEndElement();
    }
}

void StateMachineSerializer::deserializeEntryMetadata(StateMachineEntity* entity) {
    qDebug() << "deserializeEntryMetadata" << mXmlReader->name();
    if (nullptr != entity) {
        if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"editorinfo"))) {
            QString localGeometryValue = tryGetElementAttribute("localGeometry");

            if (localGeometryValue.isEmpty() == false) {
                const QStringList pointParts = localGeometryValue.split(';');
                QPolygonF linePath;

                // start and end points are recalculated by the HsmTransition element so we set them to 0
                linePath.append(QPointF(0, 0));  // start point

                for (int i = 0; i + 1 < pointParts.size(); i += 2) {
                    bool okX = false;
                    bool okY = false;
                    const double x = pointParts[i].toDouble(&okX);
                    const double y = pointParts[i + 1].toDouble(&okY);

                    if (okX && okY) {
                        linePath.append(QPointF(x, y));
                    } else {
                        handleParseError("Invalid point format in qt:editorinfo localGeometry");
                    }
                }

                linePath.append(QPointF(0, 0));  // end point
                entity->setMetadata(StateMachineEntity::MetadataKey::GEOMETRY, linePath);
            } else {
                QString geometryValue = tryGetElementAttribute("geometry");
                QStringList geometryParts = geometryValue.split(';');

                if (geometryParts.size() == 6) {
                    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_X, geometryParts[0].toDouble());
                    entity->setMetadata(StateMachineEntity::MetadataKey::POSITION_Y, geometryParts[1].toDouble());
                    entity->setMetadata(StateMachineEntity::MetadataKey::WIDTH, geometryParts[4].toDouble());
                    entity->setMetadata(StateMachineEntity::MetadataKey::HEIGHT, geometryParts[5].toDouble());
                } else {
                    handleParseError("Invalid geometry format in qt:editorinfo");
                }
            }

            mXmlReader->readNext();
        }
    } else {
        qCritical() << "entity is null";
    }
}

/**
 * @brief Helper function to handle parse errors
 * @param errorMessage The error message to log
 */
void StateMachineSerializer::handleParseError(const QString& errorMessage) {
    qWarning() << "Parse error:" << errorMessage;
}

bool StateMachineSerializer::parseAllChildEntities(const QSharedPointer<StateMachineEntity>& parent) {
    bool res = false;

    QXmlStreamReader::TokenType token = mXmlReader->readNext();

    while (!mXmlReader->atEnd() && !mXmlReader->hasError() && (QXmlStreamReader::EndElement != token)) {
        if (QXmlStreamReader::StartElement == token) {
            // if (mXmlReader->name() == QStringView(u"editorinfo")) {
            //     deserializeEntryMetadata(parent.get());
            // } else {
            // TODO: handle errors if can't parse child entities
            parseChildEntity(parent);
            // }
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

        QSharedPointer<State> state = entity.dynamicCast<State>();
    } else if (mXmlReader->name() == QStringView(u"initial")) {
        entity = parseInitialState();
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
                ptrParent->setTransitionCallback(parseInvoke());
            } else {
                qWarning() << "script elements outside of transition nodes are not supported. skipping";
            }
        }
    } else if (mXmlReader->name() == QStringView(u"editorinfo")) {
        deserializeEntryMetadata(parent.get());
    }
    // TODO: parallel

    if (entity && ((nullptr == outNewParent) || outNewParent->isNull())) {
        QSharedPointer<Transition> transition = hsmDynamicCast<Transition>(entity);

        if (transition) {
            transition->setSource(parent.dynamicCast<State>());
        }

        parent->addChild(entity);
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
        entity = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
        entity->setName(stateId);

        QXmlStreamReader::TokenType token = mXmlReader->readNext();

        while (!mXmlReader->atEnd() && !mXmlReader->hasError() && (QXmlStreamReader::EndElement != token)) {
            if (QXmlStreamReader::StartElement == token) {
                qDebug() << __LINE__ << "Found" << mXmlReader->name();
                if (mXmlReader->name() == QStringView(u"onentry")) {
                    entity->setOnEnteringCallback(parseOnEntry());
                } else if (mXmlReader->name() == QStringView(u"onexit")) {
                    entity->setOnEnteringCallback(parseOnExit());
                } else if (mXmlReader->name() == QStringView(u"invoke")) {
                    entity->setOnEnteringCallback(parseInvoke());
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
    // TODO
    mXmlReader->skipCurrentElement();
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

        QXmlStreamReader::TokenType token = mXmlReader->readNext();

        while (!mXmlReader->atEnd() && !mXmlReader->hasError() && (QXmlStreamReader::EndElement != token)) {
            if (QXmlStreamReader::StartElement == token) {
                if (mXmlReader->name() == QStringView(u"onentry")) {
                    entity->setOnEnteringCallback(parseOnEntry());
                } else if (mXmlReader->name() == QStringView(u"onexit")) {
                    entity->setOnEnteringCallback(parseOnExit());
                } else if (mXmlReader->name() == QStringView(u"invoke")) {
                    entity->setOnEnteringCallback(parseInvoke());
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

        entity = QSharedPointer<FinalState>::create(stateId);

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

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    return entity;
}

QSharedPointer<IncludeEntity> StateMachineSerializer::parseIncludeEntity() {
    QSharedPointer<IncludeEntity> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"include"))) {
        entity = QSharedPointer<IncludeEntity>::create("");  // name will be overwritten in parseRegularState

        entity->setPath(tryGetElementAttribute("href"));

        // TODO: handle errors
        parseAllChildEntities(entity);
    }

    return entity;
}

QSharedPointer<Transition> StateMachineSerializer::parseTransition() {
    qDebug() << Q_FUNC_INFO;
    QSharedPointer<Transition> entity;

    if ((mXmlReader->tokenType() == QXmlStreamReader::StartElement) && (mXmlReader->name() == QStringView(u"transition"))) {
        // Process transitions
        QString event = tryGetElementAttribute("event");
        QString target = tryGetElementAttribute("target");

        qDebug() << __LINE__ << "Found transition with event:" << event << "target:" << target;

        if (event.isEmpty() || target.isEmpty()) {
            handleParseError("Transition without event or target attribute");
            mXmlReader->skipCurrentElement();
            return nullptr;
        }

        // TODO: transition can reference a state that is not yet created
        // Need to parse all staes first before parsing transitions

        // // Find the parent state (the one containing the transition)
        // QSharedPointer<model::State> parentState = parentStack.top();

        // // Find target state
        // QSharedPointer<model::State> targetState = stateMap.value(target);
        // if (!targetState) {
        //     // If target state is not found, create it
        //     targetState = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR)
        //                     .dynamicCast<model::State>();
        //     targetState->setName(target);
        //     stateMap[target] = targetState;
        // }

        // entity = model::ModelElementsFactory::createUniqueTransition(parentState, targetState);
        // TODO: fixme
        // entity = model::ModelElementsFactory::createUniqueTransition(nullptr, nullptr);
        entity = QSharedPointer<Transition>::create(nullptr, nullptr, event);

        mTransitionTargets.insert(entity->id(), target);

        // Add the transition to the parent state
        // QSharedPointer<model::RegularState> parentRegState = parentState.dynamicCast<model::RegularState>();
        // if (parentRegState) {
        //     parentRegState->addTransition(transition);
        // }

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

QString StateMachineSerializer::parseOnEntry() {
    qDebug() << Q_FUNC_INFO;
    QString content;

    // Keep reading until we reach the closing </onentry> tag
    while (!(mXmlReader->tokenType() == QXmlStreamReader::EndElement && mXmlReader->name() == QStringView(u"onentry"))) {
        mXmlReader->readNext();

        if (mXmlReader->tokenType() == QXmlStreamReader::StartElement) {
            if (mXmlReader->name() == QStringView(u"script")) {
                content = mXmlReader->readElementText();
            } else {
                // Skip entire element (and its children) that isn’t <script>
                mXmlReader->skipCurrentElement();
            }
        }

        if (mXmlReader->atEnd() || mXmlReader->hasError()) {
            break;
        }
    }

    if (content.isEmpty()) {
        handleParseError("onentry element without script or with invalid content");
    }

    return content;
}

// TODO: unify with parseOnEntry
QString StateMachineSerializer::parseOnExit() {
    qDebug() << Q_FUNC_INFO;
    QString content;

    // Keep reading until we reach the closing </onexit> tag
    while (!(mXmlReader->tokenType() == QXmlStreamReader::EndElement && mXmlReader->name() == QStringView(u"onexit"))) {
        mXmlReader->readNext();

        if (mXmlReader->tokenType() == QXmlStreamReader::StartElement) {
            if (mXmlReader->name() == QStringView(u"script")) {
                content = mXmlReader->readElementText();
            } else {
                // Skip entire element (and its children) that isn’t <script>
                mXmlReader->skipCurrentElement();
            }
        }

        if (mXmlReader->atEnd() || mXmlReader->hasError()) {
            break;
        }
    }

    if (content.isEmpty()) {
        handleParseError("onexit element without script or with invalid content");
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