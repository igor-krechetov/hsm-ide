#include <QApplication>
#include <QGraphicsScene>
#include <QtTest>
#include <cmath>

#include "model/elements/InitialState.hpp"
#include "model/elements/RegularState.hpp"
#include "view/elements/HsmFinalElement.hpp"
#include "view/elements/HsmInitialElement.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/theme/Theme.hpp"
#include "view/theme/ThemeManager.hpp"

using namespace view;

/**
 * @brief Bug condition exploration test for Bug 1 (default element size not grid-aligned).
 *
 * These tests encode the EXPECTED behavior and are written BEFORE the fix.
 * They MUST FAIL on the unfixed code: the default/minimum sizes are hardcoded
 * literals (states 240x140, min 200x100, round 40x40) that are decoupled from the
 * configured grid minor step. The assertions are driven by the step itself (not the
 * literal 20), so they fail as soon as the default size is not expressed as a whole
 * multiple of the configured step.
 */
class HsmElementDefaultSizeTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void RectangularDefaultSizeIsGridMultiple();
    void RectangularMinSizeIsGridMultiple();
    void RoundDefaultSizeIsGridMultiple();

    void RectangularDefaultSizeIsGridMultipleAtLiveStep();
    void RectangularMinSizeIsGridMultipleAtLiveStep();
    void RoundDefaultSizeIsGridMultipleAtLiveStep();
    void FinalRoundDefaultSizeIsGridMultiple();

    void RectangularOuterRectIsGridMultipleAfterInit();
    void RoundOuterRectIsGridMultipleAfterInitAndBorderNotClipped();

private:
    bool isWholeMultiple(qreal value, qreal step) const;
    void setGridStep(qreal step);

private:
    qreal mStep = 0.0;
    view::theme::Theme mOriginalTheme;
};

void HsmElementDefaultSizeTest::init() {
    // Save the active theme so the awkward grid step does not leak into other tests.
    mOriginalTheme = ThemeManager::instance().theme();

    // Drive the fail-first from the configured grid step directly. Today's defaults
    // (states 240x140, min 200x100, round 40x40) happen to be multiples of the default
    // step 20, so asserting against step 20 would pass even on the unfixed code. Choosing
    // a step the hardcoded literals are NOT whole multiples of exposes the real defect:
    // the default/min sizes are decoupled from minorLineStep. 140, 200, 100 and 40 are
    // all NOT whole multiples of 30, so an element whose size tracked the step would be
    // snapped to multiples of 30, while the unfixed hardcoded literals are not.
    setGridStep(30.0);
    QVERIFY2(mStep > 0.0, "grid minorLineStep must be positive for the exploration test to be meaningful");
}

/**
 * @brief Apply a grid minor step to the active theme and cache it in mStep.
 *
 * Element default/min sizes are derived from the configured minorLineStep at
 * construction time, so a slot must install the step it wants to exercise before
 * constructing the element under test.
 */
void HsmElementDefaultSizeTest::setGridStep(qreal step) {
    view::theme::Theme theme = mOriginalTheme;
    theme.grid.minorLineStep = step;
    ThemeManager::instance().setTheme(theme);

    mStep = ThemeManager::instance().theme().grid.minorLineStep;
}

void HsmElementDefaultSizeTest::cleanup() {
    ThemeManager::instance().setTheme(mOriginalTheme);
}

bool HsmElementDefaultSizeTest::isWholeMultiple(qreal value, qreal step) const {
    const qreal ratio = value / step;
    const qreal rounded = std::round(ratio);

    return qAbs(ratio - rounded) < 1e-9;
}

/**
 * @brief Design Test Case 1: a rectangular state element's default width/height must be
 *        whole multiples of the grid minor step.
 *
 * EXPECTED (unfixed): FAIL whenever the hardcoded 240x140 is not a multiple of the
 * configured step (the sizes are not derived from minorLineStep).
 */
void HsmElementDefaultSizeTest::RectangularDefaultSizeIsGridMultiple() {
    HsmStateElement element;
    const QRectF rect = element.elementRect();

    QVERIFY2(isWholeMultiple(rect.width(), mStep),
             qPrintable(QString("default width %1 is not a whole multiple of grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(isWholeMultiple(rect.height(), mStep),
             qPrintable(QString("default height %1 is not a whole multiple of grid step %2").arg(rect.height()).arg(mStep)));
}

/**
 * @brief Design Test Case 2: a rectangular state element's minimum size must be
 *        whole multiples of the grid minor step.
 *
 * EXPECTED (unfixed): FAIL whenever the hardcoded 200x100 min size is not a multiple
 * of the configured step.
 */
void HsmElementDefaultSizeTest::RectangularMinSizeIsGridMultiple() {
    HsmStateElement element;
    const QSizeF minSize = element.minSize();

    QVERIFY2(isWholeMultiple(minSize.width(), mStep),
             qPrintable(QString("min width %1 is not a whole multiple of grid step %2").arg(minSize.width()).arg(mStep)));
    QVERIFY2(isWholeMultiple(minSize.height(), mStep),
             qPrintable(QString("min height %1 is not a whole multiple of grid step %2").arg(minSize.height()).arg(mStep)));
}

/**
 * @brief Design Test Case 3: a round pseudo-state element's default width/height must be
 *        whole multiples of the grid minor step.
 *
 * EXPECTED (unfixed): FAIL whenever the hardcoded 40x40 is not a multiple of the
 * configured step.
 */
void HsmElementDefaultSizeTest::RoundDefaultSizeIsGridMultiple() {
    HsmInitialElement element;
    const QRectF rect = element.elementRect();

    QVERIFY2(
        isWholeMultiple(rect.width(), mStep),
        qPrintable(QString("round default width %1 is not a whole multiple of grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(
        isWholeMultiple(rect.height(), mStep),
        qPrintable(QString("round default height %1 is not a whole multiple of grid step %2").arg(rect.height()).arg(mStep)));
}

/**
 * @brief Requirement 2.1 at the live default step: a rectangular state element's default
 *        width/height must be whole multiples of the production grid minor step (20).
 *
 * The forced-step slots above prove the default is derived from minorLineStep rather than
 * a hardcoded literal. This slot additionally pins the behavior at the step the IDE ships
 * with, so a regression that restores a literal happening to divide 30 but not 20 (or vice
 * versa) is still caught in the real configuration.
 */
void HsmElementDefaultSizeTest::RectangularDefaultSizeIsGridMultipleAtLiveStep() {
    setGridStep(20.0);

    HsmStateElement element;
    const QRectF rect = element.elementRect();

    QVERIFY2(isWholeMultiple(rect.width(), mStep),
             qPrintable(QString("default width %1 is not a whole multiple of live grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(
        isWholeMultiple(rect.height(), mStep),
        qPrintable(QString("default height %1 is not a whole multiple of live grid step %2").arg(rect.height()).arg(mStep)));
}

/**
 * @brief Requirement 2.1/2.3 at the live default step: a rectangular state element's
 *        minimum size must be whole multiples of the production grid minor step (20).
 */
void HsmElementDefaultSizeTest::RectangularMinSizeIsGridMultipleAtLiveStep() {
    setGridStep(20.0);

    HsmStateElement element;
    const QSizeF minSize = element.minSize();

    QVERIFY2(isWholeMultiple(minSize.width(), mStep),
             qPrintable(QString("min width %1 is not a whole multiple of live grid step %2").arg(minSize.width()).arg(mStep)));
    QVERIFY2(
        isWholeMultiple(minSize.height(), mStep),
        qPrintable(QString("min height %1 is not a whole multiple of live grid step %2").arg(minSize.height()).arg(mStep)));
}

/**
 * @brief Requirement 2.2 at the live default step: a round pseudo-state element's default
 *        width/height must be whole multiples of the production grid minor step (20).
 */
void HsmElementDefaultSizeTest::RoundDefaultSizeIsGridMultipleAtLiveStep() {
    setGridStep(20.0);

    HsmInitialElement element;
    const QRectF rect = element.elementRect();

    QVERIFY2(isWholeMultiple(rect.width(), mStep),
             qPrintable(
                 QString("round default width %1 is not a whole multiple of live grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(
        isWholeMultiple(rect.height(), mStep),
        qPrintable(
            QString("round default height %1 is not a whole multiple of live grid step %2").arg(rect.height()).arg(mStep)));
}

/**
 * @brief Requirement 2.2: a second round pseudo-state subtype (final) must also derive its
 *        default size from the grid step.
 *
 * All round pseudo-states (initial/final/history/entry/exit) share HsmRoundElement, so a
 * single additional subtype guards against a per-subtype override that reintroduces a
 * hardcoded size. Verified at the forced step (30) installed by init() so the assertion
 * fails for a literal that is not a multiple of the configured step.
 */
void HsmElementDefaultSizeTest::FinalRoundDefaultSizeIsGridMultiple() {
    HsmFinalElement element;
    const QRectF rect = element.elementRect();

    QVERIFY2(isWholeMultiple(rect.width(), mStep),
             qPrintable(
                 QString("final round default width %1 is not a whole multiple of grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(
        isWholeMultiple(rect.height(), mStep),
        qPrintable(
            QString("final round default height %1 is not a whole multiple of grid step %2").arg(rect.height()).arg(mStep)));
}

/**
 * @brief Requirement 2.1/2.3 at the RENDERED geometry: a rectangular state element's outer
 *        rect (elementRect()) must be a whole multiple of the grid step AFTER init().
 *
 * Reading elementRect() right after construction is INSUFFICIENT and masked the real Bug 1:
 * the constructor sets mOuterRect to the (grid-multiple) mSize, while init() is what populates
 * the rendered rect. Before the fix init() inflated mOuterRect by the border pen width
 * (penWidth*2), so a 240x140 state rendered as 246x146 - off a step-30 grid. This slot init()s
 * the element (with a model entity, like production) and asserts the post-init() outer rect.
 */
void HsmElementDefaultSizeTest::RectangularOuterRectIsGridMultipleAfterInit() {
    QGraphicsScene scene;
    auto stateModel = QSharedPointer<model::RegularState>::create("State_init");
    HsmStateElement element;
    element.init(stateModel);
    scene.addItem(&element);

    const QRectF rect = element.elementRect();

    QVERIFY2(
        isWholeMultiple(rect.width(), mStep),
        qPrintable(QString("post-init() outer width %1 is not a whole multiple of grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(isWholeMultiple(rect.height(), mStep),
             qPrintable(
                 QString("post-init() outer height %1 is not a whole multiple of grid step %2").arg(rect.height()).arg(mStep)));
}

/**
 * @brief Requirement 2.2/2.3: a round element's outer rect must be a grid multiple AFTER init(),
 *        and its drawn circle (radius) must stay inside the outer rect (border not clipped).
 *
 * The round element exhibited the user-visible off-grid rendering (46x46 at step 20). The fix
 * removes the pen-width inflation (outer rect = grid-multiple size) and insets radius() by half
 * the pen width so the stroke is not clipped by the un-inflated rect.
 */
void HsmElementDefaultSizeTest::RoundOuterRectIsGridMultipleAfterInitAndBorderNotClipped() {
    QGraphicsScene scene;
    auto initialModel = QSharedPointer<model::InitialState>::create();
    HsmInitialElement element;
    element.init(initialModel);
    scene.addItem(&element);

    const QRectF rect = element.elementRect();

    QVERIFY2(
        isWholeMultiple(rect.width(), mStep),
        qPrintable(
            QString("post-init() round outer width %1 is not a whole multiple of grid step %2").arg(rect.width()).arg(mStep)));
    QVERIFY2(isWholeMultiple(rect.height(), mStep),
             qPrintable(QString("post-init() round outer height %1 is not a whole multiple of grid step %2")
                            .arg(rect.height())
                            .arg(mStep)));

    // The circle must fit inside the (un-inflated) outer rect so the stroke is not clipped.
    // radius() is protected, so recompute it from the production formula
    // (min(w, h) / 2 - penWidth / 2) and assert it stays within half the min dimension.
    const qreal penWidth = ThemeManager::instance().theme().node.selectedBorderPen.widthF();
    const qreal maxRadius = qMin(rect.width(), rect.height()) / 2.0;
    const qreal drawnRadius = maxRadius - penWidth / 2.0;
    QVERIFY2(drawnRadius <= maxRadius + 1e-9 && drawnRadius > 0.0,
             qPrintable(QString("round drawn radius %1 does not fit inside half the outer rect min-dimension %2 "
                                "(border would be clipped)")
                            .arg(drawnRadius)
                            .arg(maxRadius)));
}

int runHsmElementDefaultSizeTest(int argc, char** argv) {
    QApplication app(argc, argv);
    HsmElementDefaultSizeTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmElementDefaultSizeTest.moc"
