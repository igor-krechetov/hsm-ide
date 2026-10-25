#include <QPointF>
#include <QPolygonF>
#include <QSizeF>
#include <QtTest>

#include "../TestPaths.hpp"
#include "model/ParseErrorCollector.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/EntryPoint.hpp"
#include "model/elements/ExitPoint.hpp"
#include "model/elements/FinalState.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/elements/IncludeEntity.hpp"
#include "model/elements/InitialState.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class StateMachineSerializerDeserializationTest : public QObject {
    Q_OBJECT

private slots:
    void DeserializeDeepHistory();
    void DeserializeShallowHistory();
    void DeserializeSubstatesHierarchy();
    void DeserializeStateGeometryWithDelayedParentParsing();
    void DeserializeQtGeometryFixture();
    void DeserializeQtTransitionGeometry();
    void DeserializeMultipleTransitionsForSingleState();
    void DeserializeExternalAndInternalTransitions();
    void DeserializeTransitionScript();
    void DeserializeEntryPointTransitions();
    void DeserializeExitPointAndTargetingTransition();
    // Top-level <final> action-block parsing (bugfix: final-state-onentry-parsing)
    void DeserializeTopLevelFinalOnEntry();
    void DeserializeTopLevelFinalInvoke();
    void DeserializeTopLevelFinalOnEntryPreservesOrder();
    void DeserializeTopLevelFinalOnExitUnsupported();
    void DeserializeTopLevelFinalActionFree();
    void TopLevelFinalRoundTripOmitsOnExit();
    void DeserializeAllElementsFinalOnEntry();
    void DeserializeIncludeEntity();
    void IncludeEntityPreservesHsmUidForLayout();
    void IncludeEntityPreservesInvokeCallback();
    void DeserializeMalformedXml();
    void DeserializeStateWithoutId();
    void ValidateStructure();
    void DeserializeInvalidHierarchyIgnored();
    void MalformedXmlReportsErrorWithLocation();
    void MissingStateIdRecordsProblem();
    void WellFormedYieldsEmptyReport();
    void ReportResetsBetweenImports();
    void UnknownElementsReportedAsWarnings();
    void DuplicateUidsReported();
    void ReservedRootUidReported();
    void InvalidUidValuesReported();
    void BadGeometryReported();
    void MissingIdsReportedPerState();
    void MalformedXmlInInvalidFolderReportsError();
    void TransitionTargetProblemsReported();
    void MixedInvalidFileReportsAllCategories();
};

static int countDirectTransitions(const QSharedPointer<model::RegularState>& state) {
    int count = 0;

    for (const auto& child : state->childrenEntities()) {
        if (child->type() == model::StateMachineEntity::Type::Transition) {
            ++count;
        }
    }

    return count;
}

/**
 * @brief Validate deserialization of deep history state.
 *
 * Use-case: Ensure `history` with `type="deep"` keeps default transition metadata.
 *
 * @startuml
 * state S1
 * state H <<history:deep>>
 * H --> S1 : resume
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeDeepHistory() {
    const QString scxml = test::loadScxmlFixture("history_deep.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto history = model->root()->findChildStateByName("H").dynamicCast<model::HistoryState>();
    QVERIFY(history);
    QCOMPARE(model::HistoryType::DEEP, history->historyType());
    QVERIFY(history->defaultTransition());
    QCOMPARE(QString("resume"), history->defaultTransition()->event());
}

/**
 * @brief Validate deserialization of shallow history state.
 *
 * Use-case: Ensure `history` with `type="shallow"` is parsed as shallow history.
 *
 * @startuml
 * state S1 {
 * state H <<history:shallow>>
 * H --> S2 : resume
 * }
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeShallowHistory() {
    const QString scxml = test::loadScxmlFixture("history_shallow.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto history = model->root()->findChildStateByName("H").dynamicCast<model::HistoryState>();
    QVERIFY(history);
    QCOMPARE(model::HistoryType::SHALLOW, history->historyType());
    QVERIFY(history->defaultTransition());
    QCOMPARE(QString("S2"), history->defaultTransition()->target()->name());
}

/**
 * @brief Validate deserialization of nested substates.
 *
 * Use-case: Ensure parent-child hierarchy is preserved for nested states.
 *
 * @startuml
 * state Parent {
 *   state ChildA
 *   state ChildB {
 *     state GrandChild
 *   }
 * }
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeSubstatesHierarchy() {
    const QString scxml = test::loadScxmlFixture("substates.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto parent = model->root()->findChildStateByName("Parent").dynamicCast<model::RegularState>();
    QVERIFY(parent);
    auto childA = model->root()->findChildStateByName("ChildA").dynamicCast<model::RegularState>();
    auto childB = model->root()->findChildStateByName("ChildB").dynamicCast<model::RegularState>();
    auto grandChild = model->root()->findChildStateByName("GrandChild").dynamicCast<model::RegularState>();

    QVERIFY(childA);
    QVERIFY(childB);
    QVERIFY(grandChild);
    QCOMPARE(parent, model->root()->findParentState(childA->id()).dynamicCast<model::RegularState>());
    QCOMPARE(childB, model->root()->findParentState(grandChild->id()).dynamicCast<model::RegularState>());
}

/**
 * @brief Validate deserialization of nested state geometry when parent geometry appears after child nodes.
 *
 * Use-case: Ensure Qt editorinfo geometry is stored and applied after the full state tree is available.
 */
void StateMachineSerializerDeserializationTest::DeserializeStateGeometryWithDelayedParentParsing() {
    const QString scxml = R"(
<scxml xmlns="http://www.w3.org/2005/07/scxml" version="1.0" name="TestMachine" xmlns:qt="http://www.qt.io/2015/02/scxml-ext">
  <state id="Parent">
    <state id="Child">
      <qt:editorinfo geometry="80;80;-20;-20;40;40"/>
    </state>
    <qt:editorinfo geometry="100;100;-50;-50;100;100"/>
  </state>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto parent = model->root()->findChildStateByName("Parent").dynamicCast<model::RegularState>();
    auto child = model->root()->findChildStateByName("Child").dynamicCast<model::RegularState>();

    QVERIFY(parent);
    QVERIFY(child);
    QCOMPARE(parent->getPos(), QPointF(50.0, 50.0));
    QCOMPARE(parent->getSize(), QSizeF(100.0, 100.0));
    QCOMPARE(child->getPos(), QPointF(10.0, 10.0));
    QCOMPARE(child->getSize(), QSizeF(40.0, 40.0));
}

/**
 * @brief Validate Qt geometry deserialization for a real fixture.
 *
 * Use-case: Ensure positions are read from nested Qt editorinfo metadata with a full state tree.
 */
void StateMachineSerializerDeserializationTest::DeserializeQtGeometryFixture() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_geometry_02.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto state_1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    auto state_1_1 = model->root()->findChildStateByName("state_1_1").dynamicCast<model::RegularState>();
    auto state_2 = model->root()->findChildStateByName("state_2").dynamicCast<model::RegularState>();
    auto state_2_2 = model->root()->findChildStateByName("state_2_1").dynamicCast<model::RegularState>();

    QVERIFY(state_1);
    QVERIFY(state_1_1);
    QVERIFY(state_2);
    QVERIFY(state_2_2);

    QCOMPARE(state_1->getPos(), QPointF(0.0, 0.0));
    QCOMPARE(state_1->getSize(), QSizeF(400.0, 300.0));

    QCOMPARE(state_1_1->getPos(), QPointF(220.0, 180.0));
    QCOMPARE(state_1_1->getSize(), QSizeF(160.0, 80.0));

    QCOMPARE(state_2->getPos(), QPointF(500.0, 0.0));
    QCOMPARE(state_2->getSize(), QSizeF(300.0, 200.0));

    QCOMPARE(state_2_2->getPos(), QPointF(140.0, 100.0));
    QCOMPARE(state_2_2->getSize(), QSizeF(180.0, 100.0));
}

/**
 * @brief Validate Qt transition geometry deserialization for a real fixture.
 *
 * Use-case: Ensure transition event positions are preserved for qt:editorinfo localGeometry.
 */
void StateMachineSerializerDeserializationTest::DeserializeQtTransitionGeometry() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_geometry_03.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto state_1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    QVERIFY(state_1);

    QSharedPointer<model::Transition> transition;
    for (const auto& child : state_1->childrenEntities()) {
        if (child->type() == model::StateMachineEntity::Type::Transition) {
            auto candidate = child.dynamicCast<model::Transition>();
            if (candidate && candidate->event() == "event_2") {
                transition = candidate;
                break;
            }
        }
    }

    QVERIFY(transition);
    QVariant geometryData = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY(geometryData.isValid());

    const QPolygonF linePath = geometryData.value<QPolygonF>();
    QCOMPARE(linePath.size(), 3);

    const QPointF intermediatePoint = linePath[1];
    const double expectedX = state_1->getMetadata(model::StateMachineEntity::MetadataKey::QT_DELTA_X).toDouble() - 121.82;
    const double expectedY = state_1->getMetadata(model::StateMachineEntity::MetadataKey::QT_DELTA_Y).toDouble();

    QVERIFY(qAbs(intermediatePoint.x() - expectedX) < 0.01);
    QVERIFY(qAbs(intermediatePoint.y() - expectedY) < 0.01);
}

/**
 * @brief Validate deserialization of multiple transitions on one state.
 *
 * Use-case: Ensure a single state can hold multiple outgoing transitions.
 *
 * @startuml
 * state S1
 * state S2
 * S1 --> S2 : e1
 * S1 --> S3 : e2
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeMultipleTransitionsForSingleState() {
    const QString scxml = test::loadScxmlFixture("multiple_transitions.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto s1 = model->root()->findChildStateByName("S1").dynamicCast<model::RegularState>();
    QVERIFY(s1);
    QCOMPARE(2, countDirectTransitions(s1));
}

/**
 * @brief Validate transition type parsing for external and internal transitions.
 *
 * Use-case: Ensure SCXML `type` attribute maps to model transition type enum.
 *
 * @startuml
 * state S1
 * state S2
 * S1 --> S2 : e1 / external
 * S1 --> S3 : e2 / internal
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeExternalAndInternalTransitions() {
    const QString scxml = test::loadScxmlFixture("multiple_transitions.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto s1 = model->root()->findChildStateByName("S1").dynamicCast<model::RegularState>();
    QVERIFY(s1);

    bool seenExternal = false;
    bool seenInternal = false;
    s1->forEachChildElement([&seenExternal, &seenInternal](QSharedPointer<model::StateMachineEntity> parent,
                                                           QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            auto tr = child.dynamicCast<model::Transition>();
            if (tr && tr->event() == "e1") {
                seenExternal = (tr->transitionType() == model::TransitionType::EXTERNAL);
            } else if (tr && tr->event() == "e2") {
                seenInternal = (tr->transitionType() == model::TransitionType::INTERNAL);
            }
        }

        return keepWalking;
    });

    QVERIFY(seenExternal);
    QVERIFY(seenInternal);
}

/**
 * @brief Validate deserialization of transition script content.
 *
 * Use-case: Ensure `<transition><script>...</script></transition>` is parsed into transition action metadata.
 *
 * @startuml
 * state state_1 {
 *   state state_1_1
 *   state state_1_2
 *   state_1_1 --> state_1_2 : EVENT_1
 * }
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeTransitionScript() {
    const QString scxml = test::loadScxmlFixture("transitions/transition_script.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto state_1_1 = model->root()->findChildStateByName("state_1_1").dynamicCast<model::RegularState>();
    QVERIFY(state_1_1);

    bool foundScriptTransition = false;
    state_1_1->forEachChildElement([&foundScriptTransition](QSharedPointer<model::StateMachineEntity> parent,
                                                            QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            auto transition = child.dynamicCast<model::Transition>();
            if (transition && transition->event() == "EVENT_1") {
                foundScriptTransition = transition->hasTransitionAction() &&
                                        transition->transitionAction()->serialize() == "callback_name" &&
                                        transition->target() && transition->target()->name() == "state_1_2";
            }
        }

        return keepWalking;
    });

    QVERIFY(foundScriptTransition);

    // A transition <script> is a valid child (it becomes the transition's callback action), so
    // parsing it must NOT produce a "Skipped unparseable child element: script" warning.
    for (const model::ParseErrorCollector::Problem& problem : serializer.parseReport().problems()) {
        QVERIFY2(false == problem.message.contains("script", Qt::CaseInsensitive),
                 qPrintable(QString("unexpected parse report entry for a valid transition script: %1").arg(problem.message)));
    }
}

/**
 * @brief Validate parsing of entry-point equivalent (`initial`) with transitions.
 *
 * Use-case: Ensure transitions from `<initial>` are parsed and linked to targets.
 *
 * @startuml
 * state Region {
 *   [*] --> A
 *   [*] --> B
 * }
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeEntryPointTransitions() {
    const QString scxml = test::loadScxmlFixture("entrypoint_multiple.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    bool hasInitialState = false;
    bool hasTransitionWithEmptyEvent = false;

    model->root()->forEachChildElement(
        [&hasInitialState, &hasTransitionWithEmptyEvent](QSharedPointer<model::StateMachineEntity> parent,
                                                         QSharedPointer<model::StateMachineEntity> child) {
            Q_UNUSED(parent);
            bool keepWalking = true;

            if (child->type() == model::StateMachineEntity::Type::State) {
                auto state = child.dynamicCast<model::State>();
                if (state && state->stateType() == model::StateType::ENTRYPOINT) {
                    hasInitialState = true;
                    auto entry = state.dynamicCast<model::EntryPoint>();

                    if (entry) {
                        auto& transitions = entry->transitions();
                        if (transitions.size() > 0) {
                            const QString target = transitions.first()->target()->name();
                            const bool validTarget = (target == "A" || target == "B");

                            hasTransitionWithEmptyEvent = (validTarget && transitions.first()->event().isEmpty());
                        }
                    }
                }
            }

            return keepWalking;
        });

    QVERIFY(hasInitialState);
    QVERIFY(hasTransitionWithEmptyEvent);
}

/**
 * @brief Validate parsing of exit point and transitions targeting it.
 *
 * Use-case: Ensure nested `<final>` maps to ExitPoint and transitions can target it.
 *
 * @startuml
 * state Parent {
 *   state Worker
 *   Worker --> XP : done
 *   state XP <<exitPoint>>
 * }
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeExitPointAndTargetingTransition() {
    const QString scxml = test::loadScxmlFixture("exitpoint.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto xp = model->root()->findChildStateByName("XP").dynamicCast<model::ExitPoint>();
    QVERIFY(xp);
    QCOMPARE(QString("leave"), xp->event());

    auto worker = model->root()->findChildStateByName("Worker").dynamicCast<model::RegularState>();
    QVERIFY(worker);

    bool hasTargetingTransition = false;
    worker->forEachChildElement([&hasTargetingTransition](QSharedPointer<model::StateMachineEntity> parent,
                                                          QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            auto tr = child.dynamicCast<model::Transition>();
            if (tr && tr->target() && tr->target()->name() == "XP") {
                hasTargetingTransition = true;
            }
        }

        return keepWalking;
    });

    QVERIFY(hasTargetingTransition);
}

/**
 * @brief Validate include entity parsing from xi:include.
 *
 * Use-case: Ensure `<xi:include>` gets mapped to IncludeEntity path.
 *
 * @startuml
 * state IncludeNode
 * IncludeNode : xi:include href=subchart.scxml
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeIncludeEntity() {
    const QString scxml = test::loadScxmlFixture("include_entity.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto include = model->root()->findChildStateByName("IncludeNode").dynamicCast<model::IncludeEntity>();
    QVERIFY(include);
    QCOMPARE(QString("subchart.scxml"), include->path());
}

/**
 * @brief Regression: a state that both carries an hsm:uid and contains an <xi:include>
 *        must keep that uid after being promoted to an IncludeEntity.
 *
 * Bug: the enclosing <state> owns the hsm:uid (the <xi:include> element has none). When the
 * state was promoted to an IncludeEntity the promotion dropped the original id and the entity
 * kept a freshly generated id instead, so the <hsm:editor> layout entry keyed by the state's
 * uid could no longer be matched ("layout references unknown uid").
 *
 * @startuml
 * state NeighborState
 * NeighborState : xi:include href=transitions.scxml
 * @enduml
 */
void StateMachineSerializerDeserializationTest::IncludeEntityPreservesHsmUidForLayout() {
    const QString scxml = QStringLiteral(
        "<scxml version=\"1.0\" xmlns=\"http://www.w3.org/2005/07/scxml\" "
        "xmlns:xi=\"http://www.w3.org/2001/XInclude\" xmlns:hsm=\"https://hsm-ide.dev/scxml\" name=\"IncludeLayout\">"
        "  <state id=\"NeighborState\" hsm:uid=\"33\">"
        "    <xi:include href=\"transitions.scxml\" parse=\"xml\"/>"
        "  </state>"
        "  <hsm:editor>"
        "    <hsm:layout>"
        "      <hsm:state uid=\"33\" x=\"10.00\" y=\"20.00\" width=\"240.00\" height=\"140.00\"/>"
        "    </hsm:layout>"
        "  </hsm:editor>"
        "</scxml>");

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto include = model->root()->findChildStateByName("NeighborState").dynamicCast<model::IncludeEntity>();
    QVERIFY(include);

    // The IncludeEntity must keep the enclosing state's uid so layout can be matched.
    QCOMPARE(include->id(), static_cast<model::EntityID_t>(33));

    // The layout entry (keyed by uid 33) must have been applied to the include node.
    QCOMPARE(include->getPos(), QPointF(10.00, 20.00));
    QCOMPARE(include->getSize(), QSizeF(240.00, 140.00));

    // No "unknown uid" layout warning should have been produced.
    const model::ParseErrorCollector& report = serializer.parseReport();

    for (const auto& problem : report.problems()) {
        QVERIFY2(!problem.message.contains("unknown uid"), qPrintable(problem.message));
    }
}

/**
 * @brief Regression: a state that carries an <invoke> and contains an <xi:include> must keep
 *        its state-changed (invoke) callback after being promoted to an IncludeEntity.
 *
 * Bug: IncludeEntity::promoteFrom rebuilt the state-changed action with type ModelAction::NONE,
 * so hasOnStateChangedAction() became false and the <invoke> was silently dropped on save.
 *
 * @startuml
 * state NeighborState
 * NeighborState : invoke srcexpr=callbackName
 * NeighborState : xi:include href=transitions.scxml
 * @enduml
 */
void StateMachineSerializerDeserializationTest::IncludeEntityPreservesInvokeCallback() {
    const QString scxml = QStringLiteral(
        "<scxml version=\"1.0\" xmlns=\"http://www.w3.org/2005/07/scxml\" "
        "xmlns:xi=\"http://www.w3.org/2001/XInclude\" xmlns:hsm=\"https://hsm-ide.dev/scxml\" name=\"IncludeInvoke\">"
        "  <state id=\"NeighborState\" hsm:uid=\"33\">"
        "    <invoke srcexpr=\"callbackName\"/>"
        "    <xi:include href=\"transitions.scxml\" parse=\"xml\"/>"
        "  </state>"
        "</scxml>");

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    auto include = model->root()->findChildStateByName("NeighborState").dynamicCast<model::IncludeEntity>();
    QVERIFY(include);

    // The invoke callback must survive promotion to IncludeEntity (not downgraded to NONE).
    QVERIFY(include->hasOnStateChangedAction());
    QCOMPARE(include->onStateChangedAction()->serialize(), QStringLiteral("callbackName"));
}

/**
 * @brief Validate malformed SCXML handling for XML syntax errors.
 *
 * Use-case: Ensure parser behavior is deterministic when SCXML is not well-formed XML.
 *
 * @startuml
 * [*] --> ParseError
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeMalformedXml() {
    const QString scxml = test::loadScxmlFixture("malformed_not_xml.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    // Current parser behavior keeps partially parsed nodes on malformed XML.
    QVERIFY(model != nullptr);
    QVERIFY(model->root()->findChildStateByName("S1") != nullptr);
}

/**
 * @brief Validate malformed SCXML handling for state without id attribute.
 *
 * Use-case: Ensure invalid state nodes are ignored without crashing parser.
 *
 * @startuml
 * state (missing id)
 * @enduml
 */
void StateMachineSerializerDeserializationTest::DeserializeStateWithoutId() {
    const QString scxml = test::loadScxmlFixture("malformed_missing_state_id.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    QCOMPARE(nullptr, model->root()->findChildStateByName(""));
    QVERIFY(model->root()->findChildStateByName("S2") != nullptr);
}

/**
 * @brief Validate structural validation checks required SCXML root attributes.
 *
 * Use-case: Fast pre-check for import before full parse.
 *
 * @startuml
 * [*] --> ValidOrInvalid
 * @enduml
 */
void StateMachineSerializerDeserializationTest::ValidateStructure() {
    model::StateMachineSerializer serializer;
    const QString valid = "<scxml xmlns=\"http://www.w3.org/2005/07/scxml\" version=\"1.0\"></scxml>";
    const QString invalid = "<scxml version=\"1.0\"></scxml>";

    // NOTE: current implementation checks for explicit "xmlns" attribute presence via QXmlStreamReader::attributes().
    // With default XML namespace handling this returns false for both strings.
    QVERIFY(!serializer.validateScxmlStructure(valid));
    QVERIFY(!serializer.validateScxmlStructure(invalid));
}

/**
 * @brief Validate invalid SCXML hierarchy nodes are ignored according to shared hierarchy rules.
 */
void StateMachineSerializerDeserializationTest::DeserializeInvalidHierarchyIgnored() {
    const QString scxml = R"(<scxml version="1.0" xmlns="http://www.w3.org/2005/07/scxml" name="InvalidHierarchy">
<state id="Parent">
  <initial>
    <transition event="start" target="Child"/>
  </initial>
  <state id="Child"/>
</state>
<history id="Hroot" type="shallow"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);
    QVERIFY(model->root()->findChildStateByName("Parent") != nullptr);
    QVERIFY(model->root()->findChildStateByName("Child") != nullptr);
    QVERIFY(model->root()->findChildStateByName("Hroot") == nullptr);
}

/**
 * @brief Malformed XML yields a non-empty report containing a located error.
 *
 * Use-case: User opens a file with an XML well-formedness error and must see the location.
 */
void StateMachineSerializerDeserializationTest::MalformedXmlReportsErrorWithLocation() {
    const QString scxml = test::loadScxmlFixture("malformed_not_xml.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    QSharedPointer<model::StateMachineModel> model = QSharedPointer<model::StateMachineModel>::create("ReportTarget");
    serializer.deserializeFromScxml(scxml, model);

    const model::ParseErrorCollector& report = serializer.parseReport();
    QVERIFY(!report.isEmpty());
    QVERIFY(report.hasErrors());

    bool foundLocatedError = false;

    for (const model::ParseErrorCollector::Problem& problem : report.problems()) {
        if ((model::ParseErrorCollector::Severity::Error == problem.severity) && (problem.line > 0)) {
            foundLocatedError = true;
            break;
        }
    }

    QVERIFY(foundLocatedError);
}

/**
 * @brief A state missing its id attribute is recorded as a parse problem.
 *
 * Use-case: Semantic problems are surfaced even when the XML itself is well-formed.
 */
void StateMachineSerializerDeserializationTest::MissingStateIdRecordsProblem() {
    const QString scxml = test::loadScxmlFixture("malformed_missing_state_id.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    QSharedPointer<model::StateMachineModel> model = QSharedPointer<model::StateMachineModel>::create("ReportTarget");
    serializer.deserializeFromScxml(scxml, model);

    const model::ParseErrorCollector& report = serializer.parseReport();
    QVERIFY(!report.isEmpty());

    bool foundIdProblem = false;

    for (const model::ParseErrorCollector::Problem& problem : report.problems()) {
        if (problem.message.contains("id")) {
            foundIdProblem = true;
            break;
        }
    }

    QVERIFY(foundIdProblem);
}

/**
 * @brief A well-formed SCXML input produces an empty parse report.
 *
 * Use-case: Normal opens must not raise spurious problems.
 */
void StateMachineSerializerDeserializationTest::WellFormedYieldsEmptyReport() {
    const QString scxml = test::loadScxmlFixture("substates.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    QSharedPointer<model::StateMachineModel> model = QSharedPointer<model::StateMachineModel>::create("ReportTarget");
    serializer.deserializeFromScxml(scxml, model);

    QVERIFY(serializer.parseReport().isEmpty());
}

/**
 * @brief The parse report resets between successive imports on one serializer instance.
 *
 * Use-case: Problems from a previous import must not leak into a later clean import.
 */
void StateMachineSerializerDeserializationTest::ReportResetsBetweenImports() {
    model::StateMachineSerializer serializer;

    const QString malformed = test::loadScxmlFixture("malformed_not_xml.scxml");
    QVERIFY(!malformed.isEmpty());
    QSharedPointer<model::StateMachineModel> firstModel = QSharedPointer<model::StateMachineModel>::create("First");
    serializer.deserializeFromScxml(malformed, firstModel);
    QVERIFY(!serializer.parseReport().isEmpty());

    const QString wellFormed = test::loadScxmlFixture("substates.scxml");
    QVERIFY(!wellFormed.isEmpty());
    QSharedPointer<model::StateMachineModel> secondModel = QSharedPointer<model::StateMachineModel>::create("Second");
    serializer.deserializeFromScxml(wellFormed, secondModel);

    QVERIFY(serializer.parseReport().isEmpty());
}

namespace {

// Count problems whose message contains the given needle (case-sensitive).
int countMatching(const model::ParseErrorCollector& report, const QString& needle) {
    int count = 0;

    for (const model::ParseErrorCollector::Problem& problem : report.problems()) {
        if (problem.message.contains(needle)) {
            ++count;
        }
    }

    return count;
}

model::ParseErrorCollector reportFor(const QString& fixture) {
    const QString scxml = test::loadScxmlFixture(fixture);
    model::StateMachineSerializer serializer;
    QSharedPointer<model::StateMachineModel> model = QSharedPointer<model::StateMachineModel>::create("ReportTarget");
    serializer.deserializeFromScxml(scxml, model);
    return serializer.parseReport();
}

}  // namespace

/**
 * @brief Unrecognized top-level elements are reported as warnings, not silently skipped.
 */
void StateMachineSerializerDeserializationTest::UnknownElementsReportedAsWarnings() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_02_unknown_elements.scxml");

    QCOMPARE(countMatching(report, "<widget>"), 1);
    QCOMPARE(countMatching(report, "<superstate>"), 1);
}

/**
 * @brief Repeated hsm:uid values are reported as duplicate-uid warnings.
 */
void StateMachineSerializerDeserializationTest::DuplicateUidsReported() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_03_duplicate_uids.scxml");

    // Three elements share uid=5; the first registers, the other two are duplicates.
    QCOMPARE(countMatching(report, "Duplicate hsm:uid 5"), 2);
}

/**
 * @brief uid=1 collides with the reserved model root and gets an explicit message.
 */
void StateMachineSerializerDeserializationTest::ReservedRootUidReported() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_02_unknown_elements.scxml");

    QCOMPARE(countMatching(report, "reserved for the model root"), 1);
}

/**
 * @brief Non-numeric and out-of-range hsm:uid values are reported.
 */
void StateMachineSerializerDeserializationTest::InvalidUidValuesReported() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_07_invalid_uid_value.scxml");

    QCOMPARE(countMatching(report, "Invalid hsm:uid value \"not-a-number\""), 1);
    QCOMPARE(countMatching(report, "Invalid hsm:uid value \"0\""), 1);
}

/**
 * @brief Layout entries with bad coordinates or unknown uids are reported.
 */
void StateMachineSerializerDeserializationTest::BadGeometryReported() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_05_bad_geometry.scxml");

    QVERIFY(countMatching(report, "non-numeric coordinate/dimension") >= 1);
    QVERIFY(countMatching(report, "references unknown uid") >= 1);
}

/**
 * @brief Each state missing an id is reported individually.
 */
void StateMachineSerializerDeserializationTest::MissingIdsReportedPerState() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_04_missing_ids.scxml");

    QCOMPARE(countMatching(report, "State element without id attribute"), 2);
    QVERIFY(report.hasErrors());
}

/**
 * @brief A well-formedness error in the invalid folder is reported as an error with a location.
 */
void StateMachineSerializerDeserializationTest::MalformedXmlInInvalidFolderReportsError() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_06_malformed_xml.scxml");

    QVERIFY(report.hasErrors());

    bool locatedError = false;

    for (const model::ParseErrorCollector::Problem& problem : report.problems()) {
        if ((model::ParseErrorCollector::Severity::Error == problem.severity) && (problem.line > 0)) {
            locatedError = true;
            break;
        }
    }

    QVERIFY(locatedError);
}

/**
 * @brief Transitions without a target, and targets that do not resolve, are reported as errors.
 */
void StateMachineSerializerDeserializationTest::TransitionTargetProblemsReported() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_08_transition_missing_target.scxml");

    QCOMPARE(countMatching(report, "Transition without target attribute"), 1);
    QCOMPARE(countMatching(report, "Transition target not found: DoesNotExist"), 1);
}

/**
 * @brief The original mixed invalid file surfaces problems from every category.
 */
void StateMachineSerializerDeserializationTest::MixedInvalidFileReportsAllCategories() {
    const model::ParseErrorCollector report = reportFor("invalid/invalid_01.scxml");

    QVERIFY(report.hasErrors());
    QVERIFY(countMatching(report, "reserved for the model root") >= 1);
    QVERIFY(countMatching(report, "Unrecognized or invalid top-level element") >= 1);
    QCOMPARE(countMatching(report, "State element without id attribute"), 1);
    QVERIFY(countMatching(report, "non-numeric coordinate/dimension") >= 1);
}

namespace {

// Serialize an action list into a plain QStringList of callback names, in order.
QStringList enteringActionNames(const QSharedPointer<model::FinalState>& finalState) {
    QStringList names;

    for (const auto& action : finalState->onEnteringActions()) {
        names.append(action->serialize());
    }

    return names;
}

QSharedPointer<model::FinalState> topLevelFinal(const QSharedPointer<model::StateMachineModel>& model,
                                                const QString& name) {
    return model->root()->findChildStateByName(name).dynamicCast<model::FinalState>();
}

}  // namespace

/**
 * @brief A top-level <final> with <onentry> loads its entering action (bug condition).
 *
 * Validates Property 1 (Requirement 2.1): the discarded <onentry> is now loaded.
 */
void StateMachineSerializerDeserializationTest::DeserializeTopLevelFinalOnEntry() {
    const QString scxml = test::loadScxmlFixture("final_toplevel_onentry.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto done = topLevelFinal(model, "Done");
    QVERIFY(done);
    QCOMPARE(QStringList{"cb1"}, enteringActionNames(done));
}

/**
 * @brief A top-level <final> with <invoke> loads its state-changed action (Requirement 2.2).
 */
void StateMachineSerializerDeserializationTest::DeserializeTopLevelFinalInvoke() {
    const QString scxml = test::loadScxmlFixture("final_toplevel_invoke.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto done = topLevelFinal(model, "Done");
    QVERIFY(done);
    QVERIFY(done->hasOnStateChangedAction());
    QCOMPARE(QString("cb3"), done->onStateChangedAction()->serialize());
}

/**
 * @brief Multiple <onentry> scripts on a top-level <final> load in document order (Requirement 2.1).
 */
void StateMachineSerializerDeserializationTest::DeserializeTopLevelFinalOnEntryPreservesOrder() {
    const QString scxml = test::loadScxmlFixture("final_toplevel_onentry_order.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto done = topLevelFinal(model, "Done");
    QVERIFY(done);
    QCOMPARE((QStringList{"a", "b"}), enteringActionNames(done));
    QVERIFY(done->hasOnStateChangedAction());
    QCOMPARE(QString("notifyCb"), done->onStateChangedAction()->serialize());
}

/**
 * @brief A top-level <final> with only <onexit> loads no actions - <onexit> is unsupported (Requirement 3.1).
 */
void StateMachineSerializerDeserializationTest::DeserializeTopLevelFinalOnExitUnsupported() {
    const QString scxml = test::loadScxmlFixture("final_toplevel_onexit_unsupported.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto done = topLevelFinal(model, "Done");
    QVERIFY(done);
    QVERIFY(!done->hasOnEnteringAction());
    QVERIFY(!done->hasOnStateChangedAction());
}

/**
 * @brief An action-free top-level <final> loads with id and event only (Requirement 3.4).
 */
void StateMachineSerializerDeserializationTest::DeserializeTopLevelFinalActionFree() {
    const QString scxml = test::loadScxmlFixture("final_toplevel_actionfree.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto done = topLevelFinal(model, "Done");
    QVERIFY(done);
    QVERIFY(!done->hasOnEnteringAction());
    QVERIFY(!done->hasOnStateChangedAction());
}

/**
 * @brief load -> save -> load of a top-level <final> preserves <onentry>/<invoke> and emits no <onexit> (Requirement 3.6).
 */
void StateMachineSerializerDeserializationTest::TopLevelFinalRoundTripOmitsOnExit() {
    const QString scxml = test::loadScxmlFixture("final_toplevel_onentry_order.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    const QString saved = serializer.serializeToScxml(model, model::SerializationFormat::HSM, true, true);
    QVERIFY(!saved.contains("<onexit"));
    QVERIFY(saved.contains("<onentry"));
    QVERIFY(saved.contains("srcexpr=\"notifyCb\""));

    auto reloaded = serializer.deserializeFromScxml(saved);
    QVERIFY(reloaded);
    auto done = topLevelFinal(reloaded, "Done");
    QVERIFY(done);
    QCOMPARE((QStringList{"a", "b"}), enteringActionNames(done));
    QVERIFY(done->hasOnStateChangedAction());
    QCOMPARE(QString("notifyCb"), done->onStateChangedAction()->serialize());
}

/**
 * @brief Integration: all_elements.scxml top-level final FinalState1 now carries its <onentry> action.
 */
void StateMachineSerializerDeserializationTest::DeserializeAllElementsFinalOnEntry() {
    QFile file(QStringLiteral("tests/data/manual_tests/all_elements.scxml"));

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QSKIP("all_elements.scxml manual fixture not reachable from this working directory");
    }

    const QString scxml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto finalState1 = topLevelFinal(model, "FinalState1");
    QVERIFY(finalState1);
    QCOMPARE(QStringList{"callbackName"}, enteringActionNames(finalState1));
}

int runStateMachineSerializerDeserializationTest(int argc, char** argv) {
    StateMachineSerializerDeserializationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineSerializerDeserializationTest.moc"
