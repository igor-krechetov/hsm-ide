#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QtTest>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/ElementGripItem.hpp"

using namespace view;

static bool nearPoint(const QPointF& a, const QPointF& b, qreal eps = 1e-6) {
    return qAbs(a.x() - b.x()) < eps && qAbs(a.y() - b.y()) < eps;
}

// ---------------------------------------------------------------------------
// Transition endpoints are computed purely geometrically. With no intermediate
// grips the source anchor comes from findStartingPointFromRect and the
// destination is the perpendicular foot dropped from that source anchor onto the
// nearest target edge. (Any Qt Creator startTargetFactors/endTargetFactors hints
// in an imported file are ignored.)
// ---------------------------------------------------------------------------

// Test-only subclass exposing the protected geometric helpers and the mapped
// endpoint rects, so the test can compute the expected anchors with the SAME
// rects/helpers recalculateLine() uses internally.
class ProbeTransition : public HsmTransition {
public:
    QPointF probeStartFromRect(const QRectF& rectFrom, const QRectF& rectTo) {
        return findStartingPointFromRect(rectFrom, rectTo);
    }

    QPointF probeEndFromPoint(const QRectF& rectTo, const QPointF& prevPoint) {
        return findPerpendicularEdgePoint(rectTo, prevPoint);
    }

    QRectF mappedRect(HsmElement* element) {
        return mapRectFromItem(element, element->elementRect());
    }
};

class HsmTransitionAnchorTest : public QObject {
    Q_OBJECT

private slots:
    void HintLessEndpointsUseGeometricHelpers();

private:
    static QList<ElementGripItem*> collectGrips(HsmTransition* transition);
};

QList<ElementGripItem*> HsmTransitionAnchorTest::collectGrips(HsmTransition* transition) {
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
 * @brief No-intermediate endpoints: source via findStartingPointFromRect, destination via
 *        the perpendicular foot from the source anchor.
 *
 * Two disjoint states with no intermediate grips. The source anchor must equal
 * findStartingPointFromRect(rectFrom, rectTo); the destination must equal the perpendicular
 * foot dropped from that source anchor onto the target. The expected values are produced by
 * the same helpers recalculateLine() uses, so this pins the geometric routing.
 */
void HsmTransitionAnchorTest::HintLessEndpointsUseGeometricHelpers() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto transitionModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "event");

    HsmStateElement* fromElement = new HsmStateElement(QSizeF(240.0, 140.0));
    fromElement->init(fromModel);
    scene.addItem(fromElement);
    fromElement->setPos(0, 0);

    // Disjoint placement so the geometric helper returns an edge anchor rather than a
    // center collapse, exercising the non-overlapping fallback branch.
    HsmStateElement* toElement = new HsmStateElement(QSizeF(200.0, 100.0));
    toElement->init(toModel);
    scene.addItem(toElement);
    toElement->setPos(600, 400);

    ProbeTransition* transition = new ProbeTransition();
    transition->init(transitionModel);
    transition->connectElements(fromElement, toElement);
    transition->recalculateLine();

    const QRectF rectFrom = transition->mappedRect(fromElement);
    const QRectF rectTo = transition->mappedRect(toElement);
    const QPointF expectedFrom = transition->probeStartFromRect(rectFrom, rectTo);
    const QPointF expectedTo = transition->probeEndFromPoint(rectTo, expectedFrom);

    const QList<ElementGripItem*> grips = collectGrips(transition);
    QVERIFY2(grips.size() == 2, qPrintable(QString("expected exactly source and destination grips, got %1").arg(grips.size())));

    const QPointF actualFrom = grips.first()->pos();
    const QPointF actualTo = grips.last()->pos();

    QVERIFY2(nearPoint(actualFrom, expectedFrom),
             qPrintable(QString("start anchor (%1,%2) != findStartingPointFromRect (%3,%4)")
                            .arg(actualFrom.x())
                            .arg(actualFrom.y())
                            .arg(expectedFrom.x())
                            .arg(expectedFrom.y())));
    QVERIFY2(nearPoint(actualTo, expectedTo),
             qPrintable(QString("end anchor (%1,%2) != perpendicular foot (%3,%4)")
                            .arg(actualTo.x())
                            .arg(actualTo.y())
                            .arg(expectedTo.x())
                            .arg(expectedTo.y())));
}

int runHsmTransitionAnchorTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionAnchorTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionAnchorTest.moc"
