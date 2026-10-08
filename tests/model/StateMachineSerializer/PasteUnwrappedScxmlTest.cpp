#include <QtTest>

#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"

/**
 * @brief Regression tests for clipboard paste of bare SCXML fragments.
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
 */
class PasteUnwrappedScxmlTest : public QObject {
    Q_OBJECT

private slots:
    void PasteStateWithHsmUidSucceeds();
    void PastePlainStateStillSucceeds();
    void PastedHsmUidDoesNotCollideWithExistingId();
    void PasteFullScxmlDocumentStillPastes();

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
    outResult = serializer.deserializeFromUnwrapperScxml(fragment, QString(WRAPPER_ID), importModel, true);

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
 * @brief The pasted hsm:uid must NOT be honored (ignoreUid=true), so no collision is possible.
 *
 * The pasted state must get a freshly generated id, distinct from the raw pasted uid 22 and
 * within the valid id range.
 */
void PasteUnwrappedScxmlTest::PastedHsmUidDoesNotCollideWithExistingId() {
    model::StateMachineSerializer serializer;
    bool result = false;

    QSharedPointer<model::State> pastedState = pasteFragment(serializer, "<state id=\"State_10\" hsm:uid=\"22\"/>", result);

    QVERIFY(result);
    QVERIFY(pastedState);

    // ignoreUid=true forces a fresh id, so the raw pasted uid 22 must NOT be honored.
    QVERIFY(pastedState->id() != static_cast<model::EntityID_t>(22));
    QVERIFY(pastedState->id() != model::INVALID_MODEL_ID);
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

    const bool result = serializer.deserializeFromUnwrapperScxml(fragment, QString(WRAPPER_ID), importModel, true);

    QVERIFY(result);

    QSharedPointer<model::State> fullState = importModel->root()->findChildStateByName("Full1");
    QVERIFY(fullState);
}

int runPasteUnwrappedScxmlTest(int argc, char** argv) {
    PasteUnwrappedScxmlTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "PasteUnwrappedScxmlTest.moc"
