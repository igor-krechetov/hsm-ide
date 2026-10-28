#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QSharedPointer>
#include <QSizeF>
#include <QtTest>

#include "model/elements/RegularState.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/private/ElementBoundaryGripItem.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/elements/private/HsmElement.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// BUG (logs/hsm-ide-06.log): when a child state's edge lies EXACTLY on the
// parent's edge (left / top / right / bottom), it becomes impossible to resize
// the parent AT ALL - including GROWING it, which can never shrink the usable
// interior and must always be allowed.
//
// Reproduced scenario from the log:
//   newOuterRect  = QRectF(0,0 304x212)   (parent, growing wider via the East grip)
//   childrenSize  = QRectF(0,64 240x140)  (child flush against the LEFT edge: left==0)
//   -> "can't resize because child items wont fit"  -> CAN RESIZE = false
//
// ROOT CAUSE: HsmResizableElement::onGripMoved() gates the resize on
//
//   childrenSize.top()  > newOuterRect.top()  && childrenSize.left()  > newOuterRect.left() &&
//   childrenSize.bottom() < newOuterRect.bottom() && childrenSize.right() < newOuterRect.right()
//
// Those are STRICT inequalities, so a child whose edge merely TOUCHES the parent
// edge (left==0, i.e. childrenSize.left() > newOuterRect.left() is 0 > 0 == false)
// fails the predicate and the resize is rejected in EVERY direction - even a pure
// grow that moves the opposite edge outward.
//
// Expected behaviour: a child edge touching the parent edge is a valid (flush)
// layout; growing the parent must be allowed, and shrinking must be allowed down
// to the point where the children still fit (edges may touch).
// ---------------------------------------------------------------------------

namespace {

// Exposes the boundary grips and onGripMoved() so a resize can be driven exactly
// the way an ElementBoundaryGripItem drag does.
class ProbeCompositeState : public HsmStateElement {
public:
    explicit ProbeCompositeState(const QSizeF& size)
        : HsmStateElement(size) {}

    ElementBoundaryGripItem* boundaryGrip(const GripDirection direction) {
        for (QGraphicsItem* child : childItems()) {
            auto* grip = dynamic_cast<ElementBoundaryGripItem*>(child);

            if ((nullptr != grip) && (grip->direction() == direction)) {
                return grip;
            }
        }

        return nullptr;
    }

    bool resizeFromGrip(const GripDirection direction, const QPointF& delta) {
        ElementBoundaryGripItem* grip = boundaryGrip(direction);

        return (nullptr != grip) ? onGripMoved(grip, delta) : false;
    }

    QRectF outerRect() const {
        return elementRect();
    }
};

struct Composite {
    QGraphicsScene scene;
    QGraphicsView view;
    ProbeCompositeState* parent = nullptr;
    HsmStateElement* child = nullptr;
    QSharedPointer<model::RegularState> parentModel;
    QSharedPointer<model::RegularState> childModel;
};

// Builds a composite "Active" state containing one child "Idle" state, then aligns
// the child so its bounding box (in parent coordinates) is FLUSH with the requested
// parent edge (the condition the bug needs). The child is parented to the state's
// inner body section, so its scene offset relative to the parent outer rect is not
// known up front - we therefore MEASURE the child rect after an initial placement
// and correct setPos() so the requested edge lands exactly on the parent edge.
enum class FlushEdge { Left, Top, Right, Bottom };

static QRectF childRectInParent(HsmStateElement* parent, HsmStateElement* child) {
    return parent->mapRectFromItem(child, child->elementRect());
}

void buildFlushComposite(Composite& c, const QSizeF& parentSize, const QSizeF& childSize, const FlushEdge edge) {
    c.view.setScene(&c.scene);

    c.parentModel = QSharedPointer<model::RegularState>::create("Active");
    c.childModel = QSharedPointer<model::RegularState>::create("Idle");
    c.parentModel->addChildState(c.childModel);

    c.parent = new ProbeCompositeState(parentSize);
    c.parent->init(c.parentModel);
    c.scene.addItem(c.parent);
    c.parent->setPos(0.0, 0.0);

    c.child = new HsmStateElement(childSize);
    c.child->init(c.childModel);
    c.parent->addChildItem(c.child);

    // Place the child roughly centred, then correct so the requested edge is flush.
    c.child->setPos(QPointF(90.0, 90.0));

    const QRectF parentRect = c.parent->outerRect();
    const QRectF childRect = childRectInParent(c.parent, c.child);
    QPointF correction(0.0, 0.0);

    switch (edge) {
        case FlushEdge::Left:
            correction.setX(parentRect.left() - childRect.left());
            break;
        case FlushEdge::Top:
            correction.setY(parentRect.top() - childRect.top());
            break;
        case FlushEdge::Right:
            correction.setX(parentRect.right() - childRect.right());
            break;
        case FlushEdge::Bottom:
            correction.setY(parentRect.bottom() - childRect.bottom());
            break;
    }

    c.child->setPos(c.child->pos() + correction);
}

}  // namespace

class HsmResizableElementChildOnEdgeTest : public QObject {
    Q_OBJECT

private slots:
    void GrowAllowedWhenChildFlushWithLeftEdge();
    void GrowAllowedWhenChildFlushWithTopEdge();
    void GrowAllowedWhenChildFlushWithRightEdge();
    void GrowAllowedWhenChildFlushWithBottomEdge();
    void ShrinkRejectedWhenItWouldClipChild();
};

/**
 * @brief Child flush with the LEFT edge - growing the parent wider (East) must be allowed.
 *
 * This is the exact log scenario: child at left==0, parent grown via the East grip.
 * On the unfixed code onGripMoved() returns false.
 */
void HsmResizableElementChildOnEdgeTest::GrowAllowedWhenChildFlushWithLeftEdge() {
    Composite c;
    const QSizeF parentSize(300.0, 220.0);
    const QSizeF childSize(120.0, 80.0);
    // Child flush against the LEFT edge so its left edge == parent left edge.
    buildFlushComposite(c, parentSize, childSize, FlushEdge::Left);

    const QRectF childRect = childRectInParent(c.parent, c.child);
    QVERIFY2(qFuzzyCompare(childRect.left() + 1.0, c.parent->outerRect().left() + 1.0),
             qPrintable(QString("precondition: child left %1 not flush with parent left %2")
                            .arg(childRect.left())
                            .arg(c.parent->outerRect().left())));

    const qreal widthBefore = c.parent->outerRect().width();

    // Grow wider from the East grip (origin does not move, opposite edge extends out).
    const bool resized = c.parent->resizeFromGrip(GripDirection::East, QPointF(60.0, 0.0));

    QVERIFY2(resized, "growing the parent was rejected while a child was flush with the left edge");
    QVERIFY2(c.parent->outerRect().width() > widthBefore,
             qPrintable(
                 QString("parent width did not grow: before %1 after %2").arg(widthBefore).arg(c.parent->outerRect().width())));
}

/**
 * @brief Child flush with the TOP edge - growing the parent taller (South) must be allowed.
 */
void HsmResizableElementChildOnEdgeTest::GrowAllowedWhenChildFlushWithTopEdge() {
    Composite c;
    const QSizeF parentSize(300.0, 220.0);
    const QSizeF childSize(120.0, 80.0);
    // Child flush against the TOP edge so its top edge == parent top edge.
    buildFlushComposite(c, parentSize, childSize, FlushEdge::Top);

    const QRectF childRect = childRectInParent(c.parent, c.child);
    QVERIFY2(qFuzzyCompare(childRect.top() + 1.0, c.parent->outerRect().top() + 1.0),
             qPrintable(QString("precondition: child top %1 not flush with parent top %2")
                            .arg(childRect.top())
                            .arg(c.parent->outerRect().top())));

    const qreal heightBefore = c.parent->outerRect().height();

    const bool resized = c.parent->resizeFromGrip(GripDirection::South, QPointF(0.0, 60.0));

    QVERIFY2(resized, "growing the parent was rejected while a child was flush with the top edge");
    QVERIFY2(
        c.parent->outerRect().height() > heightBefore,
        qPrintable(
            QString("parent height did not grow: before %1 after %2").arg(heightBefore).arg(c.parent->outerRect().height())));
}

/**
 * @brief Child flush with the RIGHT edge - growing the parent wider (East) must be allowed.
 */
void HsmResizableElementChildOnEdgeTest::GrowAllowedWhenChildFlushWithRightEdge() {
    Composite c;
    const QSizeF parentSize(300.0, 220.0);
    const QSizeF childSize(120.0, 80.0);
    // Child flush against the RIGHT edge so its right edge == parent right edge.
    buildFlushComposite(c, parentSize, childSize, FlushEdge::Right);

    const QRectF childRect = childRectInParent(c.parent, c.child);
    QVERIFY2(qFuzzyCompare(childRect.right() + 1.0, c.parent->outerRect().right() + 1.0),
             qPrintable(QString("precondition: child right %1 not flush with parent right %2")
                            .arg(childRect.right())
                            .arg(c.parent->outerRect().right())));

    const qreal widthBefore = c.parent->outerRect().width();

    // Grow wider from the West grip: the origin moves left, the right edge (where the
    // child is flush) stays put. This must be allowed - the interior only gets bigger.
    const bool resized = c.parent->resizeFromGrip(GripDirection::West, QPointF(-60.0, 0.0));

    QVERIFY2(resized, "growing the parent was rejected while a child was flush with the right edge");
    QVERIFY2(c.parent->outerRect().width() > widthBefore,
             qPrintable(
                 QString("parent width did not grow: before %1 after %2").arg(widthBefore).arg(c.parent->outerRect().width())));
}

/**
 * @brief Child flush with the BOTTOM edge - growing the parent taller (North) must be allowed.
 */
void HsmResizableElementChildOnEdgeTest::GrowAllowedWhenChildFlushWithBottomEdge() {
    Composite c;
    const QSizeF parentSize(300.0, 220.0);
    const QSizeF childSize(120.0, 80.0);
    // Child flush against the BOTTOM edge so its bottom edge == parent bottom edge.
    buildFlushComposite(c, parentSize, childSize, FlushEdge::Bottom);

    const QRectF childRect = childRectInParent(c.parent, c.child);
    QVERIFY2(qFuzzyCompare(childRect.bottom() + 1.0, c.parent->outerRect().bottom() + 1.0),
             qPrintable(QString("precondition: child bottom %1 not flush with parent bottom %2")
                            .arg(childRect.bottom())
                            .arg(c.parent->outerRect().bottom())));

    const qreal heightBefore = c.parent->outerRect().height();

    // Grow taller from the North grip: the origin moves up, the bottom edge (where the
    // child is flush) stays put. This must be allowed - the interior only gets bigger.
    const bool resized = c.parent->resizeFromGrip(GripDirection::North, QPointF(0.0, -60.0));

    QVERIFY2(resized, "growing the parent was rejected while a child was flush with the bottom edge");
    QVERIFY2(
        c.parent->outerRect().height() > heightBefore,
        qPrintable(
            QString("parent height did not grow: before %1 after %2").arg(heightBefore).arg(c.parent->outerRect().height())));
}

/**
 * @brief Preservation guard: shrinking the parent so the child would be clipped is
 *        still rejected. The fix must only relax the touching-edge case, not disable
 *        the legitimate "children won't fit" protection.
 *
 * The child spans the full interior width; dragging the East grip inward far enough
 * to cut into the child must be refused.
 */
void HsmResizableElementChildOnEdgeTest::ShrinkRejectedWhenItWouldClipChild() {
    Composite c;
    const QSizeF parentSize(300.0, 220.0);
    const QSizeF childSize(260.0, 80.0);
    buildFlushComposite(c, parentSize, childSize, FlushEdge::Left);

    const qreal widthBefore = c.parent->outerRect().width();

    // Drag the East grip far inward - well past the child's right edge.
    const bool resized = c.parent->resizeFromGrip(GripDirection::East, QPointF(-200.0, 0.0));

    QVERIFY2(false == resized, "shrinking the parent to clip the child must be rejected");
    QVERIFY2(qFuzzyCompare(c.parent->outerRect().width(), widthBefore),
             "parent width must be unchanged after a rejected shrink");
}

int runHsmResizableElementChildOnEdgeTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmResizableElementChildOnEdgeTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmResizableElementChildOnEdgeTest.moc"
