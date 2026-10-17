#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPolygonF>
#include <QtTest>
#include <cmath>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// boundingRect() must enclose EVERYTHING paint() draws - the line, the stroke, and
// the arrowhead - otherwise moving a transition leaves stale pixels behind, because
// QGraphicsScene only repaints the area the item reports as its bounding rect.
// This reproduces the "visual artifact" defect: before the fix the margin was only
// SELECTION_DISTANCE (2px), far smaller than the 10px arrowhead.
// ---------------------------------------------------------------------------

class ProbeTransition : public HsmTransition {
public:
    QRectF mappedRect(HsmElement* element) {
        return mapRectFromItem(element, element->elementRect());
    }

    QRectF probeBoundingRect() const {
        return boundingRect();
    }

    QRectF probeShapeBounds() const {
        return shape().boundingRect();
    }
};

class HsmTransitionBoundingRectTest : public QObject {
    Q_OBJECT

private slots:
    void BoundingRectContainsArrowHead();
};

/**
 * @brief The transition bounding rect contains the arrowhead triangle at the destination.
 *
 * Rebuilds the exact arrowhead geometry paint() computes from the last segment and asserts
 * every arrowhead vertex lies within boundingRect(). A too-tight bounding rect (the artifact
 * bug) would leave the arrow tip/base outside and cause stale-pixel trails on move.
 */
void HsmTransitionBoundingRectTest::BoundingRectContainsArrowHead() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto trModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");

    // Diagonal arrangement so the final segment is slanted and the arrowhead extends in both x
    // and y - the worst case for a too-tight bounding rect.
    auto* fromE = new HsmStateElement(QSizeF(200.0, 100.0));
    fromE->init(fromModel);
    scene.addItem(fromE);
    fromE->setPos(0.0, 0.0);

    auto* toE = new HsmStateElement(QSizeF(200.0, 150.0));
    toE->init(toModel);
    scene.addItem(toE);
    toE->setPos(600.0, 500.0);

    ProbeTransition* t = new ProbeTransition();
    t->init(trModel);
    t->connectElements(fromE, toE);
    t->recalculateLine();

    const QPolygonF path = trModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(path.size() >= 2, "expected a resolved path");

    // Reproduce paint()'s arrowhead computation.
    const QPointF p1 = path[path.size() - 2];
    const QPointF p2 = path.last();
    const double angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
    const double arrowSize = HsmTransition::ARROW_SIZE;
    const double arrowAngle = M_PI / 6.0;
    const QPointF a1 = p2 - QPointF(std::cos(angle - arrowAngle) * arrowSize, std::sin(angle - arrowAngle) * arrowSize);
    const QPointF a2 = p2 - QPointF(std::cos(angle + arrowAngle) * arrowSize, std::sin(angle + arrowAngle) * arrowSize);

    const QRectF bounds = t->probeBoundingRect();

    QVERIFY2(bounds.contains(p2),
             qPrintable(QString("boundingRect %1,%2 %3x%4 does not contain arrow tip (%5,%6)")
                            .arg(bounds.x())
                            .arg(bounds.y())
                            .arg(bounds.width())
                            .arg(bounds.height())
                            .arg(p2.x())
                            .arg(p2.y())));
    QVERIFY2(bounds.contains(a1),
             qPrintable(QString("boundingRect does not contain arrow base a1 (%1,%2)").arg(a1.x()).arg(a1.y())));
    QVERIFY2(bounds.contains(a2),
             qPrintable(QString("boundingRect does not contain arrow base a2 (%1,%2)").arg(a2.x()).arg(a2.y())));

    // boundingRect() must also fully contain shape() (the selection polygon, offset 5px).
    // A boundingRect smaller than shape() violates the QGraphicsItem contract and leaves
    // stale selection pixels behind on move - the original artifact (margin was only 2px).
    const QRectF shapeBounds = t->probeShapeBounds();
    QVERIFY2(bounds.contains(shapeBounds),
             qPrintable(QString("boundingRect (%1,%2 %3x%4) does not contain shape bounds (%5,%6 %7x%8)")
                            .arg(bounds.x())
                            .arg(bounds.y())
                            .arg(bounds.width())
                            .arg(bounds.height())
                            .arg(shapeBounds.x())
                            .arg(shapeBounds.y())
                            .arg(shapeBounds.width())
                            .arg(shapeBounds.height())));
}

int runHsmTransitionBoundingRectTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionBoundingRectTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionBoundingRectTest.moc"
