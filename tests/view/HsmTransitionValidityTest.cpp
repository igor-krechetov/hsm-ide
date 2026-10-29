#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtTest>

#include "model/elements/HistoryState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmHistoryElement.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"

using namespace view;

// End-to-end validity colouring: HsmTransition::isValid() must reflect the hierarchy rule
// (siblings only, with the history-target exception) once the endpoints are connected and
// recalculateLine() has run. These tests build real view elements so they also exercise the
// parent resolution through the model (HsmElement::modelElementPtr).
class HsmTransitionValidityTest : public QObject {
    Q_OBJECT

private slots:
    void SameParentIsValid();
    void DifferentParentIsInvalid();
    void SelfTransitionIsValid();
    void HistoryFromDifferentParentIsValid();
    void HistoryFromSameParentIsInvalid();

private:
    // Creates a state view element bound to a fresh model state and adds it to the scene.
    static HsmStateElement* makeState(QGraphicsScene& scene,
                                      QSharedPointer<model::RegularState>& outModel,
                                      const QString& name);
    static HsmHistoryElement* makeHistory(QGraphicsScene& scene,
                                          QSharedPointer<model::HistoryState>& outModel,
                                          const QString& name);
    // Nests child under parent in BOTH the model and the view hierarchy, mirroring how the
    // editor builds the tree so parent resolution in updateValidity() sees a real parent.
    static void nest(const QSharedPointer<model::RegularState>& parentModel,
                     HsmStateElement* parentElement,
                     const QSharedPointer<model::State>& childModel,
                     HsmElement* childElement);
    static HsmTransition* connect(QGraphicsScene& scene,
                                  HsmElement* from,
                                  HsmElement* to,
                                  const QSharedPointer<model::State>& fromModel,
                                  const QSharedPointer<model::State>& toModel);
};

HsmStateElement* HsmTransitionValidityTest::makeState(QGraphicsScene& scene,
                                                      QSharedPointer<model::RegularState>& outModel,
                                                      const QString& name) {
    outModel = QSharedPointer<model::RegularState>::create(name);
    auto* element = new HsmStateElement(QSizeF(120.0, 80.0));
    element->init(outModel);
    scene.addItem(element);
    return element;
}

HsmHistoryElement* HsmTransitionValidityTest::makeHistory(QGraphicsScene& scene,
                                                          QSharedPointer<model::HistoryState>& outModel,
                                                          const QString& name) {
    outModel = QSharedPointer<model::HistoryState>::create(name, model::HistoryType::SHALLOW);
    auto* element = new HsmHistoryElement();
    element->init(outModel);
    scene.addItem(element);
    return element;
}

void HsmTransitionValidityTest::nest(const QSharedPointer<model::RegularState>& parentModel,
                                     HsmStateElement* parentElement,
                                     const QSharedPointer<model::State>& childModel,
                                     HsmElement* childElement) {
    parentModel->addChildState(childModel);
    childElement->setHsmParentItem(parentElement);
}

HsmTransition* HsmTransitionValidityTest::connect(QGraphicsScene& scene,
                                                  HsmElement* from,
                                                  HsmElement* to,
                                                  const QSharedPointer<model::State>& fromModel,
                                                  const QSharedPointer<model::State>& toModel) {
    auto transitionModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");
    auto* transition = new HsmTransition();
    transition->init(transitionModel);
    transition->connectElements(from, to);
    transition->recalculateLine();
    Q_UNUSED(scene);
    return transition;
}

/** Two sibling states under the same parent: a transition between them is valid. */
void HsmTransitionValidityTest::SameParentIsValid() {
    QGraphicsScene scene;

    QSharedPointer<model::RegularState> parentModel;
    auto* parent = makeState(scene, parentModel, "P");

    QSharedPointer<model::RegularState> aModel;
    QSharedPointer<model::RegularState> bModel;
    auto* a = makeState(scene, aModel, "A");
    auto* b = makeState(scene, bModel, "B");
    nest(parentModel, parent, aModel, a);
    nest(parentModel, parent, bModel, b);

    auto* transition = connect(scene, a, b, aModel, bModel);
    QVERIFY(transition->isValid());
}

/** Endpoints under different parents: the transition is invalid (highlighted red). */
void HsmTransitionValidityTest::DifferentParentIsInvalid() {
    QGraphicsScene scene;

    QSharedPointer<model::RegularState> p1Model;
    QSharedPointer<model::RegularState> p2Model;
    auto* p1 = makeState(scene, p1Model, "P1");
    auto* p2 = makeState(scene, p2Model, "P2");

    QSharedPointer<model::RegularState> aModel;
    QSharedPointer<model::RegularState> bModel;
    auto* a = makeState(scene, aModel, "A");
    auto* b = makeState(scene, bModel, "B");
    nest(p1Model, p1, aModel, a);
    nest(p2Model, p2, bModel, b);

    auto* transition = connect(scene, a, b, aModel, bModel);
    QVERIFY(false == transition->isValid());
}

/** A self-transition is trivially within one parent and always valid. */
void HsmTransitionValidityTest::SelfTransitionIsValid() {
    QGraphicsScene scene;

    QSharedPointer<model::RegularState> parentModel;
    auto* parent = makeState(scene, parentModel, "P");

    QSharedPointer<model::RegularState> aModel;
    auto* a = makeState(scene, aModel, "A");
    nest(parentModel, parent, aModel, a);

    auto* transition = connect(scene, a, a, aModel, aModel);
    QVERIFY(transition->isValid());
}

/** History target reached from a different parent (higher level): valid. */
void HsmTransitionValidityTest::HistoryFromDifferentParentIsValid() {
    QGraphicsScene scene;

    // Source sits at top level (no parent); the history state is nested inside P.
    QSharedPointer<model::RegularState> srcModel;
    auto* src = makeState(scene, srcModel, "Src");

    QSharedPointer<model::RegularState> parentModel;
    auto* parent = makeState(scene, parentModel, "P");

    QSharedPointer<model::HistoryState> histModel;
    auto* hist = makeHistory(scene, histModel, "H");
    nest(parentModel, parent, histModel, hist);

    auto* transition = connect(scene, src, hist, srcModel, histModel);
    QVERIFY(transition->isValid());
}

/** History target reached from within its own parent (same level): invalid. */
void HsmTransitionValidityTest::HistoryFromSameParentIsInvalid() {
    QGraphicsScene scene;

    QSharedPointer<model::RegularState> parentModel;
    auto* parent = makeState(scene, parentModel, "P");

    QSharedPointer<model::RegularState> srcModel;
    auto* src = makeState(scene, srcModel, "Src");
    QSharedPointer<model::HistoryState> histModel;
    auto* hist = makeHistory(scene, histModel, "H");
    nest(parentModel, parent, srcModel, src);
    nest(parentModel, parent, histModel, hist);

    auto* transition = connect(scene, src, hist, srcModel, histModel);
    QVERIFY(false == transition->isValid());
}

int runHsmTransitionValidityTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionValidityTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionValidityTest.moc"
