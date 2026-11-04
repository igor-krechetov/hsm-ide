#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

/**
 * @brief Regression tests for clipboard paste of SCXML produced by the copy path.
 *
 * These exercise StateMachineSerializer::deserializeFromUnwrapperScxml with the SAME wrapper id
 * and ignoreUid value that ProjectController::pasteScxmlElements uses on the real paste path.
 *
 * NOTE on the test layer: the real paste entry point ProjectController::pasteScxmlElements is
 * compiled only into the hsm_ide executable (not into the model/view static libraries the test
 * harness links), and it depends on views, ModificationHistoryController and view-models, so it
 * cannot be linked into the test harness without a disproportionate build change. The highest
 * realistically testable layer that reproduces the failing path is
 * deserializeFromUnwrapperScxml itself - the exact function the paste UI calls and the function
 * that contained the bug. The model-level re-id behavior pasteScxmlElements performs afterward
 * (reassignImportedIds) is covered separately by StateMachineModelTest.
 *
 * NOTE on ignoreUid: the real paste path now deserializes with ignoreUid=false. In the HSM format
 * all geometry (state position/size, transition routing) lives in a separate <hsm:editor>/<hsm:layout>
 * section that references each entity by its hsm:uid. Keeping the uids on deserialize is what lets the
 * layout section resolve back to the body states; collision safety is provided downstream by
 * reassignImportedIds() (covered in StateMachineModelTest), not by discarding uids here.
 */
class PasteUnwrappedScxmlTest : public QObject {
    Q_OBJECT

private slots:
    void PasteStateWithHsmUidSucceeds();
    void PastePlainStateStillSucceeds();
    void PastedHsmUidDoesNotCollideWithExistingId();
    void PasteFullScxmlDocumentStillPastes();
    void CopyPasteRoundTripPreservesMultiElementGeometry();
    void CopyPasteWithIdlessTransitionsKeepsGeometryOnCorrectStates();
    void SerializerNeverWritesInvalidUid();

private:
    static constexpr const char* WRAPPER_ID = "__hsmide_clipboard_wrapper__";

    static QSharedPointer<model::State> pasteFragment(model::StateMachineSerializer& serializer,
                                                      const QString& fragment,
                                                      bool& outResult);
};

QSharedPointer<model::State> PasteUnwrappedScxmlTest::pasteFragment(model::StateMachineSerializer& serializer,
                                                                    const QString& fragment,
                                                                    bool& outResult) {
    QSharedPointer<model::StateMachineModel> importModel = QSharedPointer<model::StateMachineModel>::create("ClipboardImport");

    // Same wrapper id and ignoreUid value as ProjectController::pasteScxmlElements.
    outResult = serializer.deserializeFromUnwrapperScxml(fragment, QString(WRAPPER_ID), importModel, false);

    QSharedPointer<model::State> wrapper = importModel->root()->findChildStateByName(WRAPPER_ID);
    QSharedPointer<model::State> pastedState;

    if (wrapper) {
        QSharedPointer<model::RegularState> wrapperRegular = wrapper.dynamicCast<model::RegularState>();

        if (wrapperRegular) {
            for (const auto& child : wrapperRegular->childrenEntities()) {
                if (child && (child->type() == model::StateMachineEntity::Type::State)) {
                    pastedState = child.dynamicCast<model::State>();
                    break;
                }
            }
        }
    }

    return pastedState;
}

/**
 * @brief The exact user-reported failing case: a bare fragment carrying an hsm:uid attribute.
 *
 * Before the fix the generated wrapper <scxml> did not declare the hsm namespace, so the pasted
 * hsm:uid used an undeclared prefix and QXmlStreamReader aborted the whole parse. After declaring
 * xmlns:hsm on the wrapper the fragment parses and the state is added.
 */
void PasteUnwrappedScxmlTest::PasteStateWithHsmUidSucceeds() {
    model::StateMachineSerializer serializer;
    bool result = false;

    QSharedPointer<model::State> pastedState = pasteFragment(serializer, "<state id=\"State_10\" hsm:uid=\"22\"/>", result);

    QVERIFY(result);
    QVERIFY(pastedState);
    QCOMPARE(pastedState->name(), QStringLiteral("State_10"));
}

/**
 * @brief Regression guard for the plain fragment that already worked before the fix.
 */
void PasteUnwrappedScxmlTest::PastePlainStateStillSucceeds() {
    model::StateMachineSerializer serializer;
    bool result = false;

    QSharedPointer<model::State> pastedState = pasteFragment(serializer, "<state id=\"State_8\"/>", result);

    QVERIFY(result);
    QVERIFY(pastedState);
    QCOMPARE(pastedState->name(), QStringLiteral("State_8"));
}

/**
 * @brief Pasted elements must never collide with existing model ids after merge.
 *
 * With ignoreUid=false the throwaway import model may honor the clipboard hsm:uid (here 22), but
 * that id is transient: reassignImportedIds() re-mints every imported entity from the DESTINATION
 * model's generator before merge, so a pasted uid can never collide with a live id. This test drives
 * that exact two-stage flow (deserialize with uids, then re-mint into a populated destination).
 */
void PasteUnwrappedScxmlTest::PastedHsmUidDoesNotCollideWithExistingId() {
    // Destination model already owns some ids.
    auto destModel = QSharedPointer<model::StateMachineModel>::create("Destination");
    auto existing = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, destModel->idGenerator())
                        .dynamicCast<model::RegularState>();
    QVERIFY(existing);
    destModel->root()->addChildState(existing);
    const model::EntityID_t existingId = existing->id();

    // Deserialize the clipboard fragment (carrying hsm:uid=22) into the throwaway import model.
    model::StateMachineSerializer serializer;
    bool result = false;
    QSharedPointer<model::State> pastedState = pasteFragment(serializer, "<state id=\"State_10\" hsm:uid=\"22\"/>", result);

    QVERIFY(result);
    QVERIFY(pastedState);

    // Re-mint the imported subtree from the DESTINATION generator - this is the real collision guard.
    model::reassignImportedIds(pastedState, destModel->idGenerator());

    // The re-minted id must be valid and distinct from the destination's existing id.
    QVERIFY(pastedState->id() != model::INVALID_MODEL_ID);
    QVERIFY(pastedState->id() != existingId);
    QVERIFY(pastedState->id() >= static_cast<model::EntityID_t>(1));
    QVERIFY(pastedState->id() <= static_cast<model::EntityID_t>(0xFFFFFFFE));
}

/**
 * @brief Content that already contains a full <scxml> document must still paste (branch untouched).
 */
void PasteUnwrappedScxmlTest::PasteFullScxmlDocumentStillPastes() {
    model::StateMachineSerializer serializer;

    const QString fragment = QStringLiteral(
        "<scxml version=\"1.0\" xmlns=\"http://www.w3.org/2005/07/scxml\" "
        "xmlns:hsm=\"https://hsm-ide.dev/scxml\" name=\"FullDoc\">"
        "<state id=\"Full1\" hsm:uid=\"5\"/>"
        "</scxml>");

    QSharedPointer<model::StateMachineModel> importModel = QSharedPointer<model::StateMachineModel>::create("ClipboardImport");

    const bool result = serializer.deserializeFromUnwrapperScxml(fragment, QString(WRAPPER_ID), importModel, false);

    QVERIFY(result);

    QSharedPointer<model::State> fullState = importModel->root()->findChildStateByName("Full1");
    QVERIFY(fullState);
}

/**
 * @brief Regression for the reported bug: pasting MULTIPLE elements lost their geometry.
 *
 * Root cause: the HSM format stores all geometry (position/size) in a trailing <hsm:editor>/
 * <hsm:layout> section that is emitted ONLY for a full document (addScxmlTag=true). Copy-to-clipboard
 * previously serialized a bare fragment (addScxmlTag=false), so endSerialization() never ran and the
 * clipboard carried no geometry at all - every pasted state read position (0,0)/size (0,0). With a
 * single element the loss was invisible (it just landed under the cursor); with multiple elements they
 * all collapsed onto the same anchor and lost their sizes.
 *
 * This test drives the exact serialize/deserialize contract the copy and paste paths now use:
 *   copy:  serializeToScxml(HSM, addScxmlTag=true,  ignoreUid=false)
 *   paste: deserializeFromUnwrapperScxml(..., ignoreUid=false)
 * and asserts both states keep their DISTINCT positions and their sizes through the round-trip.
 */
void PasteUnwrappedScxmlTest::CopyPasteRoundTripPreservesMultiElementGeometry() {
    // Build a source model with two top-level states at distinct positions/sizes.
    auto sourceModel = QSharedPointer<model::StateMachineModel>::create("Clipboard");

    auto stateA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    auto stateB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    QVERIFY(stateA);
    QVERIFY(stateB);

    const QPointF posA(10.0, 20.0);
    const QSizeF sizeA(120.0, 80.0);
    const QPointF posB(300.0, 250.0);
    const QSizeF sizeB(90.0, 60.0);

    stateA->setPos(posA);
    stateA->setSize(sizeA);
    stateB->setPos(posB);
    stateB->setSize(sizeB);

    sourceModel->root()->addChildState(stateA);
    sourceModel->root()->addChildState(stateB);

    // COPY: exactly the call ProjectController::serializeElementsToScxml now uses.
    model::StateMachineSerializer copySerializer;
    const QString clipboard =
        copySerializer.serializeToScxml(sourceModel, model::SerializationFormat::HSM, true, false).trimmed();

    // The clipboard must actually carry the layout section - this is the heart of the fix.
    QVERIFY2(clipboard.contains(QStringLiteral("editor")), "clipboard SCXML is missing the <hsm:editor> layout section");

    // PASTE: exactly the call ProjectController::pasteScxmlElements now uses.
    model::StateMachineSerializer pasteSerializer;
    auto importModel = QSharedPointer<model::StateMachineModel>::create("ClipboardImport");
    const bool result = pasteSerializer.deserializeFromUnwrapperScxml(clipboard, QString(WRAPPER_ID), importModel, false);
    QVERIFY(result);

    auto importedA = importModel->root()->findChildStateByName(stateA->name());
    auto importedB = importModel->root()->findChildStateByName(stateB->name());
    QVERIFY(importedA);
    QVERIFY(importedB);

    // Geometry must survive the round-trip for BOTH states, distinctly.
    QCOMPARE(importedA->getPos(), posA);
    QCOMPARE(importedA->getSize(), sizeA);
    QCOMPARE(importedB->getPos(), posB);
    QCOMPARE(importedB->getSize(), sizeB);

    // And the two states must NOT collapse onto the same position (the visible multi-paste symptom).
    QVERIFY(importedA->getPos() != importedB->getPos());
}

/**
 * @brief Regression for the SECOND reported bug: pasting multiple elements scrambled their geometry,
 *        assigning each pasted state the position/size of a DIFFERENT original state.
 *
 * Root cause: ProjectController::serializeElementsToScxml clones the selected transitions WITHOUT
 * assigning them an id, so every transition clone keeps INVALID_MODEL_ID. The clipboard then serializes
 * hsm:uid="4294967295" for each transition. On paste (ignoreUid=false) the parser rejects those invalid
 * uids and GENERATES replacements; the generated ids collide with the (valid, sequential) state uids,
 * forcing the states themselves to be re-assigned too. The <hsm:editor>/<hsm:layout> section is keyed by
 * the ORIGINAL uids, so findChild(uid) then resolves to the WRONG entity and geometry lands on the wrong
 * state. The fix assigns each transition clone a fresh unique id so no invalid uid ever reaches the
 * clipboard.
 *
 * This test reproduces the clipboard shape directly: a model whose transitions carry INVALID_MODEL_ID
 * (exactly what an id-less Transition default yields), serialized with the copy call and deserialized
 * with the paste call. It asserts that each NAMED state keeps its OWN distinct geometry after the
 * round-trip. Against the pre-fix code the invalid transition uids desync the state<->layout mapping and
 * the geometry ends up on the wrong state, failing the per-name QCOMPARE below.
 */
void PasteUnwrappedScxmlTest::CopyPasteWithIdlessTransitionsKeepsGeometryOnCorrectStates() {
    auto sourceModel = QSharedPointer<model::StateMachineModel>::create("Clipboard");

    // Three states with clearly distinct geometry, so a wrong mapping is unambiguous.
    auto stateA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    auto stateB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    auto stateC = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    QVERIFY(stateA);
    QVERIFY(stateB);
    QVERIFY(stateC);

    const QPointF posA(10.0, 20.0);
    const QSizeF sizeA(120.0, 80.0);
    const QPointF posB(300.0, 250.0);
    const QSizeF sizeB(90.0, 60.0);
    const QPointF posC(500.0, 400.0);
    const QSizeF sizeC(200.0, 150.0);

    stateA->setPos(posA);
    stateA->setSize(sizeA);
    stateB->setPos(posB);
    stateB->setSize(sizeB);
    stateC->setPos(posC);
    stateC->setSize(sizeC);

    sourceModel->root()->addChildState(stateA);
    sourceModel->root()->addChildState(stateB);
    sourceModel->root()->addChildState(stateC);

    // Add transitions created the SAME WAY the FIXED copy path builds transition clones: each gets a
    // fresh unique id from the model's own generator (the same generator the states came from), so every
    // uid in the clipboard is valid and unique. Before the fix these clones kept INVALID_MODEL_ID, the
    // clipboard serialized hsm:uid="4294967295" for each, and on paste the parser's regenerated
    // replacement ids collided with the state uids - desyncing the body from the uid-keyed
    // <hsm:editor>/<hsm:layout> section so geometry landed on the wrong states.
    auto tAB = QSharedPointer<model::Transition>::create(stateA, stateB, QStringLiteral("EV1"));
    auto tBC = QSharedPointer<model::Transition>::create(stateB, stateC, QStringLiteral("EV2"));
    auto tCA = QSharedPointer<model::Transition>::create(stateC, stateA, QStringLiteral("EV3"));
    tAB->setId(sourceModel->idGenerator().generateNextId());
    tBC->setId(sourceModel->idGenerator().generateNextId());
    tCA->setId(sourceModel->idGenerator().generateNextId());
    stateA->addChild(tAB);
    stateB->addChild(tBC);
    stateC->addChild(tCA);

    // COPY then PASTE with the exact production calls.
    model::StateMachineSerializer copySerializer;
    const QString clipboard =
        copySerializer.serializeToScxml(sourceModel, model::SerializationFormat::HSM, true, false).trimmed();

    // The clipboard must NOT carry an invalid transition uid - that is the poison that scrambled the map.
    QVERIFY2(!clipboard.contains(QStringLiteral("4294967295")),
             "clipboard serialized an invalid (INVALID_MODEL_ID) uid - transition clones were not given ids");

    model::StateMachineSerializer pasteSerializer;
    auto importModel = QSharedPointer<model::StateMachineModel>::create("ClipboardImport");
    const bool result = pasteSerializer.deserializeFromUnwrapperScxml(clipboard, QString(WRAPPER_ID), importModel, false);
    QVERIFY(result);

    auto importedA = importModel->root()->findChildStateByName(stateA->name());
    auto importedB = importModel->root()->findChildStateByName(stateB->name());
    auto importedC = importModel->root()->findChildStateByName(stateC->name());
    QVERIFY(importedA);
    QVERIFY(importedB);
    QVERIFY(importedC);

    // Each state must keep its OWN geometry - NOT another state's. This is what fails when the invalid
    // transition uids desync the state<->layout uid mapping.
    QCOMPARE(importedA->getPos(), posA);
    QCOMPARE(importedA->getSize(), sizeA);
    QCOMPARE(importedB->getPos(), posB);
    QCOMPARE(importedB->getSize(), sizeB);
    QCOMPARE(importedC->getPos(), posC);
    QCOMPARE(importedC->getSize(), sizeC);
}

/**
 * @brief Invariant guard (defense in depth): the HSM serializer must NEVER emit an invalid uid.
 *
 * Even if a caller hands the serializer an entity whose id was never assigned (INVALID_MODEL_ID),
 * the body must not contain the literal 4294967295. Writing it forces the reader to regenerate a
 * replacement id, and those replacements collide with valid sibling uids and break the uid-keyed
 * layout mapping. Omitting the attribute lets the reader assign a clean fresh id instead.
 */
void PasteUnwrappedScxmlTest::SerializerNeverWritesInvalidUid() {
    auto sourceModel = QSharedPointer<model::StateMachineModel>::create("Clipboard");

    auto stateA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    auto stateB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                      .dynamicCast<model::RegularState>();
    QVERIFY(stateA);
    QVERIFY(stateB);
    sourceModel->root()->addChildState(stateA);
    sourceModel->root()->addChildState(stateB);

    // A transition deliberately left WITHOUT an id (default INVALID_MODEL_ID).
    auto idless = QSharedPointer<model::Transition>::create(stateA, stateB, QStringLiteral("EV"));
    QCOMPARE(idless->id(), model::INVALID_MODEL_ID);
    stateA->addChild(idless);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(sourceModel, model::SerializationFormat::HSM, true, false);

    QVERIFY2(!scxml.contains(QStringLiteral("4294967295")),
             "serializer emitted the INVALID_MODEL_ID sentinel as an hsm:uid");
}

int runPasteUnwrappedScxmlTest(int argc, char** argv) {
    PasteUnwrappedScxmlTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "PasteUnwrappedScxmlTest.moc"
