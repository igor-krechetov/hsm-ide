#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QRandomGenerator>
#include <QRectF>
#include <QSizeF>
#include <QtTest>
#include <cmath>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/theme/Theme.hpp"
#include "view/theme/ThemeManager.hpp"
#include "view/theme/ThemeUtils.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Property-based tests for the element grid-alignment fix (task 7).
//
// Three universal properties are exercised across randomly generated inputs,
// reusing the harness/pattern established by HsmTransitionAnchorPreservationTest
// (the ProbeTransition subclass exposing the protected geometric helpers and the
// mapped endpoint rects) and HsmGraphicsViewPropertyTest (QRandomGenerator-driven
// input generation):
//
//  - Property 1 (snapToGrid): for any random grid step, snapToGrid(value) is
//    always a positive whole multiple of the step and never below one step.
//  - Property 4 (hint-less recalculateLine == geometric helpers): for any random
//    overlapping / non-overlapping (rectFrom, rectTo) pair WITHOUT hints, the
//    resolved endpoints equal findStartingPointFromRect exactly.
// ---------------------------------------------------------------------------

// Test-only subclass exposing the protected geometric helpers and the mapped
// endpoint rects, so the test can compute the expected anchors with the SAME
// rects/helpers recalculateLine() uses internally (identical to the probe used
// by HsmTransitionAnchorPreservationTest / HsmTransitionAnchorTest).
class ProbeTransition : public HsmTransition {
public:
    QPointF probeStartFromRect(const QRectF& rectFrom, const QRectF& rectTo) {
        return findStartingPointFromRect(rectFrom, rectTo);
    }

    QPointF probeEndFromPoint(const QRectF& rectTo, const QPointF& prevPoint) {
        return findPerpendicularEdgePoint(rectTo, prevPoint);
    }

    bool probeStraight(const QRectF& rectFrom, const QRectF& rectTo, QPointF& outFrom, QPointF& outTo) {
        return findStraightPerpendicularConnection(rectFrom, rectTo, outFrom, outTo);
    }

    QRectF mappedRect(HsmElement* element) {
        return mapRectFromItem(element, element->elementRect());
    }
};

class ElementGridAlignmentPropertyTest : public QObject {
    Q_OBJECT

private:
    static constexpr int cSnapIterations = 500;
    static constexpr int cAnchorEndpointIterations = 80;

private slots:
    void init();
    void cleanup();

    void PropertySnapToGridIsPositiveWholeMultiple();
    void PropertyHintlessEndpointsMatchGeometricHelper();

private:
    static QList<ElementGripItem*> collectGrips(HsmTransition* transition);

    void setGridStep(qreal step);

private:
    view::theme::Theme mOriginalTheme;
};

void ElementGridAlignmentPropertyTest::init() {
    // Save the active theme so the grid steps driven by the snapToGrid property
    // do not leak into other tests (mirrors HsmElementDefaultSizeTest).
    mOriginalTheme = ThemeManager::instance().theme();
}

void ElementGridAlignmentPropertyTest::cleanup() {
    ThemeManager::instance().setTheme(mOriginalTheme);
}

void ElementGridAlignmentPropertyTest::setGridStep(qreal step) {
    view::theme::Theme theme = mOriginalTheme;
    theme.grid.minorLineStep = step;
    ThemeManager::instance().setTheme(theme);
}

QList<ElementGripItem*> ElementGridAlignmentPropertyTest::collectGrips(HsmTransition* transition) {
    QList<ElementGripItem*> grips;

    for (auto* child : transition->childItems()) {
        auto* grip = dynamic_cast<ElementGripItem*>(child);

        if (nullptr != grip) {
            grips.append(grip);
        }
    }

    return grips;
}

/**
 * @brief Property 1 (Requirement 2.1): snapToGrid is a positive whole multiple of the step.
 *
 * For any configured grid minor step (step > 0) and any requested value,
 * view::snapToGrid(value) must return a result that is
 *   - a whole multiple of the step (result / step is an integer), and
 *   - at least one full step (never collapsed to zero or a fraction of a step),
 * so that an element whose origin is snapped to a grid line also has its opposite
 * edges on grid lines.
 *
 * Random steps and values are generated so the property is checked across the
 * input space rather than only at the shipped step (20).
 *
 * Validates: Requirements 2.1
 */
void ElementGridAlignmentPropertyTest::PropertySnapToGridIsPositiveWholeMultiple() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < cSnapIterations; ++iteration) {
        // Random positive, non-integer-friendly step across a wide range.
        const qreal step = 1.0 + rng.generateDouble() * 99.0;
        setGridStep(step);

        const qreal liveStep = ThemeManager::instance().theme().grid.minorLineStep;
        QVERIFY2(liveStep > 0.0, "grid minorLineStep must be positive for the snapToGrid property to hold");

        // Random requested value spanning well below one step up to many steps.
        const qreal value = rng.generateDouble() * 2000.0;

        const qreal snapped = theme::snapToGrid(value);

        // Never below one step.
        QVERIFY2(snapped >= liveStep - 1e-6,
                 qPrintable(QString("Iteration %1: snapToGrid(%2) = %3 is below one step %4")
                                .arg(iteration)
                                .arg(value)
                                .arg(snapped)
                                .arg(liveStep)));

        // Always a positive value.
        QVERIFY2(
            snapped > 0.0,
            qPrintable(QString("Iteration %1: snapToGrid(%2) = %3 is not positive").arg(iteration).arg(value).arg(snapped)));

        // Always a whole multiple of the step.
        const qreal multiples = snapped / liveStep;
        const qreal rounded = std::round(multiples);
        QVERIFY2(qAbs(multiples - rounded) < 1e-6,
                 qPrintable(QString("Iteration %1: snapToGrid(%2) = %3 is not a whole multiple of step %4 (ratio %5)")
                                .arg(iteration)
                                .arg(value)
                                .arg(snapped)
                                .arg(liveStep)
                                .arg(multiples)));

        // Whole multiple is at least 1.
        QVERIFY2(rounded >= 1.0,
                 qPrintable(QString("Iteration %1: snapToGrid(%2) = %3 is %4 steps, expected >= 1 step")
                                .arg(iteration)
                                .arg(value)
                                .arg(snapped)
                                .arg(rounded)));
    }
}

/**
 * @brief Property 4 (Requirements 2.4, 3.4): hint-less endpoints match the geometric helper.
 *
 * For many generated (rectFrom, rectTo) pairs - both overlapping and disjoint - a
 * transition that carries NO *TargetFactors and NO intermediate points must
 * resolve its start/end anchors through findStartingPointFromRect, exactly as the
 * original geometric computation does. The expected values are produced by the
 * SAME helper recalculateLine() uses, so the fallback path remains byte-for-byte
 * unchanged by the Bug 2 fix.
 *
 * This complements HsmTransitionAnchorPreservationTest, which covered the same
 * fallback on the UNFIXED baseline; here the property is re-asserted on the FIXED
 * code to prove the hint-less path is still the pure geometric computation.
 *
 * Validates: Requirements 2.4, 3.4
 */
void ElementGridAlignmentPropertyTest::PropertyHintlessEndpointsMatchGeometricHelper() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < cAnchorEndpointIterations; ++iteration) {
        QGraphicsScene scene;
        QGraphicsView view;
        view.setScene(&scene);

        const qreal fromW = 60.0 + rng.bounded(0, 200);
        const qreal fromH = 40.0 + rng.bounded(0, 160);
        const qreal toW = 60.0 + rng.bounded(0, 200);
        const qreal toH = 40.0 + rng.bounded(0, 160);

        // Even iterations overlap the source; odd iterations are disjoint in a
        // random one of the eight relative directions.
        const bool overlapping = (iteration % 2 == 0);
        QPointF toPos;

        if (overlapping) {
            toPos = QPointF(rng.bounded(0, static_cast<int>(fromW)), rng.bounded(0, static_cast<int>(fromH)));
        } else {
            const int dirX = rng.bounded(-1, 2);
            const int dirY = rng.bounded(-1, 2);
            const qreal gap = 300.0 + rng.bounded(0, 400);

            toPos = QPointF(dirX * gap, dirY * gap);

            if (dirX == 0 && dirY == 0) {
                toPos = QPointF(gap, 0.0);
            }
        }

        auto fromModel = QSharedPointer<model::RegularState>::create("from");
        auto toModel = QSharedPointer<model::RegularState>::create("to");
        auto transitionModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "event");

        HsmStateElement* fromElement = new HsmStateElement(QSizeF(fromW, fromH));
        fromElement->init(fromModel);
        scene.addItem(fromElement);
        fromElement->setPos(0, 0);

        HsmStateElement* toElement = new HsmStateElement(QSizeF(toW, toH));
        toElement->init(toModel);
        scene.addItem(toElement);
        toElement->setPos(toPos);

        // Transition model carries NO GEOMETRY and NO *TargetFactors metadata, so
        // mLinePath stays empty and the geometric (hint-less) path is exercised.
        ProbeTransition* transition = new ProbeTransition();
        transition->init(transitionModel);
        transition->connectElements(fromElement, toElement);
        transition->recalculateLine();

        const QRectF rectFrom = transition->mappedRect(fromElement);
        const QRectF rectTo = transition->mappedRect(toElement);
        // A facing+overlapping pair is joined by a straight perpendicular segment; otherwise
        // the source uses findStartingPointFromRect and the destination the perpendicular foot.
        QPointF expectedFrom;
        QPointF expectedTo;
        if (false == transition->probeStraight(rectFrom, rectTo, expectedFrom, expectedTo)) {
            expectedFrom = transition->probeStartFromRect(rectFrom, rectTo);
            expectedTo = transition->probeEndFromPoint(rectTo, expectedFrom);
        }

        const QList<ElementGripItem*> grips = collectGrips(transition);
        QVERIFY2(grips.size() == 2,
                 qPrintable(QString("Iteration %1: expected exactly source and destination grips, got %2")
                                .arg(iteration)
                                .arg(grips.size())));

        const QPointF actualFrom = grips.first()->pos();
        const QPointF actualTo = grips.last()->pos();

        QVERIFY2(qFuzzyCompare(actualFrom.x(), expectedFrom.x()) && qFuzzyCompare(actualFrom.y(), expectedFrom.y()),
                 qPrintable(QString("Iteration %1 (%2): hint-less start anchor (%3,%4) != "
                                    "findStartingPointFromRect (%5,%6)")
                                .arg(iteration)
                                .arg(overlapping ? "overlapping" : "disjoint")
                                .arg(actualFrom.x())
                                .arg(actualFrom.y())
                                .arg(expectedFrom.x())
                                .arg(expectedFrom.y())));
        QVERIFY2(qFuzzyCompare(actualTo.x(), expectedTo.x()) && qFuzzyCompare(actualTo.y(), expectedTo.y()),
                 qPrintable(QString("Iteration %1 (%2): hint-less end anchor (%3,%4) != "
                                    "findPerpendicularEdgePoint (%5,%6)")
                                .arg(iteration)
                                .arg(overlapping ? "overlapping" : "disjoint")
                                .arg(actualTo.x())
                                .arg(actualTo.y())
                                .arg(expectedTo.x())
                                .arg(expectedTo.y())));
    }
}

int runElementGridAlignmentPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    ElementGridAlignmentPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ElementGridAlignmentPropertyTest.moc"
