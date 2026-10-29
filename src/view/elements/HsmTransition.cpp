#include "HsmTransition.hpp"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTimer>
#include <cmath>

#include "HsmStateElement.hpp"
#include "model/StateHierarchyRules.hpp"
#include "model/elements/Transition.hpp"
#include "private/AutoGroupItem.hpp"
#include "private/ElementGripItem.hpp"
#include "private/HsmStateTextItem.hpp"
#include "view/common/ViewUtils.hpp"
#include "view/theme/ThemeManager.hpp"

/*
TODO: sometimes self-transitions get positioned incorrectly (shifted to the left or right) when moving them around
*/

namespace view {

HsmTransition::HsmTransition()
    : HsmElement(HsmElementType::TRANSITION, QSizeF(1, 1))
    , mFromElement(nullptr)
    , mToElement(nullptr) {
    qDebug() << "CREATE: HsmTransition: " << this;
    // NOTE: there are no grips at the beginning and end of the poly-line
    // TODO: fix me
    // self.lineGrips = [None, None]

    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(DEFAULT_Z);
    setAcceptHoverEvents(true);

    mSrcGrip = new ElementGripItem(this);
    mDestGrip = new ElementGripItem(this);
    mSrcGrip->setVisible(false);
    mDestGrip->setVisible(false);
    mSrcGrip->init();
    mDestGrip->init();
    mLineGrips = {mSrcGrip, mDestGrip};
}

HsmTransition::~HsmTransition() {
    qDebug() << "DELETE: HsmTransition: " << this;
}

void HsmTransition::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    HsmElement::init(modelEntity);

    // Create label
    mLabel = new AutoGroupItem(this);
    mLabelEvent = new HsmStateTextItem(nullptr, this);
    mLabelCondition = new HsmStateTextItem(nullptr, this);

    // configure labels order
    mLabelEvent->setData(static_cast<int>(AutoLayoutDirection::VERTICAL), 1);
    mLabelCondition->setData(static_cast<int>(AutoLayoutDirection::VERTICAL), 0);

    mLabelEvent->setData(static_cast<int>(AutoLayoutDirection::HORIZONTAL), 0);
    mLabelCondition->setData(static_cast<int>(AutoLayoutDirection::HORIZONTAL), 1);

    mLabel->setDirection(AutoLayoutDirection::VERTICAL);
    mLabel->setZValue(-1);
    mLabel->addItem(mLabelEvent);
    mLabel->addItem(mLabelCondition);

    connect(mLabel, &AutoGroupItem::geometryChanged, this, &HsmTransition::recalculateLabelPosition);

    // Pull latest data from the model
    onModelDataChanged();
    // If called directly, labels wont have their geometry updated and label will be incorrectly positioned
    QTimer::singleShot(0, this, SLOT(recalculateLine()));
}

QList<QGraphicsItem*> HsmTransition::hsmChildItems() const {
    // transitions can't have subelements
    return QList<QGraphicsItem*>();
}

void HsmTransition::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    setAllGripsVisibility(true);
}

void HsmTransition::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    if ((false == isSelected()) && (false == mConnecting)) {
        setAllGripsVisibility(false);
    }
}

void HsmTransition::updateBoundingRect(const QRectF& newRect) {
    prepareGeometryChange();
    updateSelectionPolygon();

    // The bounding rect must cover everything paint() draws AND the selection shape, otherwise
    // moving the transition leaves stale pixels behind (the scene only repaints the item's
    // boundingRect(), and QGraphicsItem requires boundingRect() to contain shape()). The line
    // fits in mLinePath.boundingRect(); the arrowhead extends up to ARROW_SIZE beyond the last
    // point and the stroke adds half the pen width on each side, so the margin accounts for
    // both plus the selection halo. We then union with the selection polygon's bounds so the
    // perpendicular 5px selection offset near the endpoints is always enclosed too.
    const qreal penWidth = ThemeManager::instance().theme().transition.selectedPen.widthF();
    const qreal margin = ARROW_SIZE + penWidth + SELECTION_DISTANCE;

    QRectF bounds = mLinePath.boundingRect().normalized().adjusted(-margin, -margin, margin, margin);
    bounds = bounds.united(mSelectionPath.boundingRect().normalized().adjusted(-penWidth, -penWidth, penWidth, penWidth));

    mOuterRect = bounds;
}

QPolygonF HsmTransition::calculateLinePolygon(int selectionOffset, const QLineF& line) {
    const double radAngle = line.angle() * M_PI / 180.0;
    const double dx = selectionOffset * std::sin(radAngle);
    const double dy = selectionOffset * std::cos(radAngle);
    const QPointF offset1(dx, dy);
    const QPointF offset2(-dx, -dy);
    QPolygonF nPolygon;

    nPolygon.append(line.p1() + offset1);
    nPolygon.append(line.p1() + offset2);
    nPolygon.append(line.p2() + offset2);
    nPolygon.append(line.p2() + offset1);
    nPolygon.append(line.p1() + offset1);

    return nPolygon;
}

void HsmTransition::updateSelectionPolygon() {
    constexpr int offset = 5;

    mSelectionPath.clear();

    for (int i = 0; i < mLinePath.size() - 1; ++i) {
        QPointF p1 = mLinePath[i];
        QPointF p2 = mLinePath[i + 1];

        mSelectionPath.addPolygon(calculateLinePolygon(offset, QLineF(p1, p2)));
    }
}

QPainterPath HsmTransition::shape() const {
    return mSelectionPath;
}

void HsmTransition::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
#ifdef DEBUG_RENDERING
    painter->setPen(ThemeManager::instance().theme().transition.debugSelectionPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(mSelectionPath);
#endif

    const auto& theme = ThemeManager::instance().theme();

    if (false == isSelected()) {
        painter->setPen(mIsValid ? theme.transition.pen : theme.transition.invalidPen);
    } else {
        // Keep the selection dash pattern but recolour it red while the transition is invalid.
        if (mIsValid) {
            painter->setPen(theme.transition.selectedPen);
        } else {
            QPen selectedInvalidPen = theme.transition.selectedPen;
            selectedInvalidPen.setColor(theme.transition.invalidPen.color());
            selectedInvalidPen.setWidthF(theme.transition.invalidPen.widthF());
            painter->setPen(selectedInvalidPen);
        }
    }

    // Draw the transition path
    for (int i = 0; i < (mLinePath.size() - 1); ++i) {
        painter->drawLine(mLinePath[i], mLinePath[i + 1]);
    }

    // Draw arrow at the end of the transition as a filled triangle
    if (mLinePath.size() >= 2) {
        const int last = mLinePath.size() - 1;
        QPointF p1 = mLinePath[last - 1];
        QPointF p2 = mLinePath[last];

        // Calculate the angle of the last segment
        double angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());

        // Arrow parameters
        const double arrowSize = ARROW_SIZE;
        double arrowAngle = M_PI / 6.0;  // 30 degrees

        // Calculate arrowhead points
        QPointF arrowP1 = p2 - QPointF(std::cos(angle - arrowAngle) * arrowSize, std::sin(angle - arrowAngle) * arrowSize);
        QPointF arrowP2 = p2 - QPointF(std::cos(angle + arrowAngle) * arrowSize, std::sin(angle + arrowAngle) * arrowSize);

        // Draw arrowhead as a filled triangle
        QPolygonF arrowHead;
        arrowHead << p2 << arrowP1 << arrowP2;

        // Set brush to same color as pen to fill the triangle
        painter->setBrush(painter->pen().color());
        painter->drawPolygon(arrowHead);
    }
}

void HsmTransition::beginConnection(HsmElement* fromElement, const QPointF& pos) {
    disconnectElements();
    mFromElement = fromElement;
    mConnecting = true;
    mDestGrip->setPos(pos);
    recalculateLine();
}

bool HsmTransition::isConnecting() const {
    return mConnecting;
}

void HsmTransition::moveConnectionTo(const QPointF& pos) {
    if (true == isConnecting()) {
        const auto prevPos = mDestGrip->pos();
        mDestGrip->setPos(pos);
        onGripMoved(mDestGrip, pos - prevPos);
    } else {
        qDebug() << "WARNING: unexpected moveConnectionTo call";
    }
}

void HsmTransition::connectElements(HsmElement* fromElement, HsmElement* toElement) {
    disconnectElements();

    if (nullptr != fromElement && nullptr != toElement) {
        mFromElement = fromElement;
        mToElement = toElement;

        // NOTE: we always add transition to the parent state of fromElement
        auto parent = fromElement->hsmParentItem();

        if (nullptr != fromElement && fromElement == toElement) {
            qDebug() << "---- added SELF TRANSITION to " << fromElement;
            fromElement->addChildItem(this);
        } else if (nullptr == parent) {
            qDebug() << "---- added TRANSITION to SCENE" << scene();
            fromElement->scene()->addItem(this);
        } else {
            qDebug() << "---- added TRANSITION to " << parent;
            parent->addChildItem(this);
        }

        if (isSelfTransition() == false) {
            connect(mFromElement, SIGNAL(geometryChanged(HsmElement*)), this, SLOT(recalculateLine()));
            connect(mToElement, SIGNAL(geometryChanged(HsmElement*)), this, SLOT(recalculateLine()));
        }

        // Update geometry if available
        auto ptrElement = modelElement<model::Transition>();

        if (ptrElement) {
            QVariant geometryVar = ptrElement->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);

            if (geometryVar.isValid()) {
                QPolygonF geometry = geometryVar.value<QPolygonF>();
                int gripIndex = 0;

                mLinePath.clear();

                for (const QPointF& gripPos : geometry) {
                    if (gripIndex != 0 && gripIndex != (geometry.size() - 1)) {
                        // First and last grips are source and destination grips
                        ElementGripItem* grip = new ElementGripItem(this);

                        grip->init();
                        grip->setPos(gripPos);
                        grip->setVisible(false);
                        mLineGrips.insert(mLineGrips.begin() + gripIndex, grip);
                    }

                    mLinePath.insert(gripIndex, gripPos);
                    ++gripIndex;
                }
            }
        } else {
            qCritical() << "failed to get model element for transition";
        }

        // Recalculate line in case label position needs to be changed
        recalculateLine();

        if (mLabel) {
            mLabel->makeMovable(isSelfTransition() == false);
            mLabel->setDirection(isSelfTransition() ? AutoLayoutDirection::HORIZONTAL : AutoLayoutDirection::VERTICAL);
        }
    }
}

void HsmTransition::disconnectElements() {
    qDebug() << Q_FUNC_INFO;

    if (isSelfTransition() == false) {
        if (mFromElement != nullptr) {
            try {
                QObject::disconnect(mFromElement, SIGNAL(geometryChanged(HsmElement*)), this, SLOT(recalculateLine()));
            } catch (...) {
                qDebug() << "from not connected";
            }
        }
        if (mToElement != nullptr) {
            try {
                QObject::disconnect(mToElement, SIGNAL(geometryChanged(HsmElement*)), this, SLOT(recalculateLine()));
            } catch (...) {
                qDebug() << "to not connected";
            }
        }
    }

    if (nullptr != mFromElement) {
        mFromElement->removeChildItem(this);
    }

    mFromElement = nullptr;
    mToElement = nullptr;
}

QPointer<HsmElement> HsmTransition::connectionCandidate() const {
    return mLastConnectionTarget;
}

bool HsmTransition::isValid() const {
    return mIsValid;
}

void HsmTransition::updateValidity() {
    bool valid = true;

    // Only a fully connected transition (both endpoints resolved) can be judged. While a
    // transition is being drawn/dragged (one endpoint still floating) we treat it as valid so
    // it isn't flashed red mid-gesture.
    if ((nullptr != mFromElement) && (nullptr != mToElement)) {
        const HsmElement* const fromParent = mFromElement->hsmParentItem();
        const HsmElement* const toParent = mToElement->hsmParentItem();

        // Resolve the model parents for an identity comparison in the hierarchy rule, plus
        // the target entity itself (the rule inspects it to detect a history target).
        // A null view parent (top-level element) maps to a null model parent, which the
        // rule compares as "same parent" only against another null.
        QSharedPointer<model::StateMachineEntity> sourceParentEntity;
        QSharedPointer<model::StateMachineEntity> targetParentEntity;
        QSharedPointer<model::StateMachineEntity> targetEntity = mToElement->modelElementPtr().lock();

        if (nullptr != fromParent) {
            sourceParentEntity = fromParent->modelElementPtr().lock();
        }
        if (nullptr != toParent) {
            targetParentEntity = toParent->modelElementPtr().lock();
        }

        valid = model::StateHierarchyRules::isTransitionValid(sourceParentEntity, targetParentEntity, targetEntity);
    }

    if (valid != mIsValid) {
        mIsValid = valid;
        update();  // repaint with the new pen colour
    }
}

QPointF HsmTransition::findStartingPointFromPoint(const QRectF& rectFrom, const QPointF& pointTo) {
    QPointF pointFrom;

    if (rectFrom.bottom() <= pointTo.y()) {
        if (rectFrom.right() <= pointTo.x()) {
            // cout << "1" << endl;
            pointFrom = QPointF(rectFrom.right(), rectFrom.center().y());
        } else if (rectFrom.left() >= pointTo.x()) {
            // cout << "3" << endl;
            pointFrom = QPointF(rectFrom.left(), rectFrom.center().y());
        } else {
            // cout << "2, 12, 23" << endl;
            pointFrom = QPointF(rectFrom.center().x(), rectFrom.bottom());
        }
    } else if (rectFrom.top() >= pointTo.y()) {
        if (rectFrom.right() <= pointTo.x()) {
            // cout << "6" << endl;
            pointFrom = QPointF(rectFrom.right(), rectFrom.center().y());
        } else if (rectFrom.left() >= pointTo.x()) {
            // cout << "8" << endl;
            pointFrom = QPointF(rectFrom.left(), rectFrom.center().y());
        } else {
            // cout << "7, 67, 78" << endl;
            pointFrom = QPointF(rectFrom.center().x(), rectFrom.top());
        }
    } else if (rectFrom.right() <= pointTo.x()) {
        // cout << "4, 14, 46" << endl;
        pointFrom = QPointF(rectFrom.right(), rectFrom.center().y());
    } else if (rectFrom.left() >= pointTo.x()) {
        // cout << "5, 35, 58" << endl;
        pointFrom = QPointF(rectFrom.left(), rectFrom.center().y());
    } else {
        // cout << "overlapping" << endl;
        pointFrom = rectFrom.center();
    }

    return pointFrom;
}

QPointF HsmTransition::findStartingPointFromRect(const QRectF& rectFrom, const QRectF& rectTo) {
    QPointF pointFrom;
    if (rectFrom.bottom() <= rectTo.top()) {
        if (rectFrom.right() <= rectTo.left()) {
            // cout << "1" << endl;
            pointFrom = QPointF(rectFrom.right(), rectFrom.center().y());
        } else if (rectFrom.left() >= rectTo.right()) {
            // cout << "3" << endl;
            pointFrom = QPointF(rectFrom.left(), rectFrom.center().y());
        } else {
            // cout << "2, 12, 23" << endl;
            pointFrom = QPointF(rectFrom.center().x(), rectFrom.bottom());
        }
    } else if (rectFrom.top() >= rectTo.bottom()) {
        if (rectFrom.right() <= rectTo.left()) {
            // cout << "6" << endl;
            pointFrom = QPointF(rectFrom.right(), rectFrom.center().y());
        } else if (rectFrom.left() >= rectTo.right()) {
            // cout << "8" << endl;
            pointFrom = QPointF(rectFrom.left(), rectFrom.center().y());
        } else {
            // cout << "7, 67, 78" << endl;
            pointFrom = QPointF(rectFrom.center().x(), rectFrom.top());
        }
    } else if (rectFrom.right() <= rectTo.left()) {
        // cout << "4, 14, 46" << endl;
        pointFrom = QPointF(rectFrom.right(), rectFrom.center().y());
    } else if (rectFrom.left() >= rectTo.right()) {
        // cout << "5, 35, 58" << endl;
        pointFrom = QPointF(rectFrom.left(), rectFrom.center().y());
        // QPointF pointTo(rectTo.right(), rectTo.center().y());
    } else {
        // cout << "overlapping" << endl;
        pointFrom = rectFrom.center();
    }

    return pointFrom;
}

QPointF HsmTransition::findPerpendicularEdgePoint(const QRectF& rect, const QPointF& neighbourPoint) {
    // Anchor an endpoint on an element edge by dropping a perpendicular from the neighbouring
    // point onto the nearest edge of that element. This is used for BOTH endpoints:
    //   - the destination, where the neighbour is the last intermediate grip (or the start
    //     point when there are none);
    //   - the source, where the neighbour is the first intermediate grip (or the destination
    //     point when there are none).
    // The segment that touches the element therefore meets its edge at a right angle (see the
    // reference layouts) instead of pointing at the center of the edge.
    //
    // "Nearest edge" is chosen with the same region analysis findStartingPointFromPoint uses.
    // The perpendicular foot is valid only when it lands within the edge's span; if it would
    // fall outside (no perpendicular possible), we fall back to the center of that edge.
    QPointF endPoint;

    // Clamp helpers for the perpendicular foot along an edge.
    const qreal footX = qBound(rect.left(), neighbourPoint.x(), rect.right());
    const qreal footY = qBound(rect.top(), neighbourPoint.y(), rect.bottom());

    // true when the perpendicular foot stays within the edge extent (a real perpendicular).
    const bool xInsideSpan = (neighbourPoint.x() >= rect.left()) && (neighbourPoint.x() <= rect.right());
    const bool yInsideSpan = (neighbourPoint.y() >= rect.top()) && (neighbourPoint.y() <= rect.bottom());

    if (rect.bottom() <= neighbourPoint.y()) {
        if (rect.right() <= neighbourPoint.x()) {
            // target is up-left of neighbourPoint: nearest edges are right and bottom corner region
            endPoint = QPointF(rect.right(), rect.center().y());
        } else if (rect.left() >= neighbourPoint.x()) {
            endPoint = QPointF(rect.left(), rect.center().y());
        } else {
            // directly below neighbourPoint: bottom edge, perpendicular foot at neighbourPoint.x()
            endPoint = QPointF(xInsideSpan ? footX : rect.center().x(), rect.bottom());
        }
    } else if (rect.top() >= neighbourPoint.y()) {
        if (rect.right() <= neighbourPoint.x()) {
            endPoint = QPointF(rect.right(), rect.center().y());
        } else if (rect.left() >= neighbourPoint.x()) {
            endPoint = QPointF(rect.left(), rect.center().y());
        } else {
            // directly above neighbourPoint: top edge, perpendicular foot at neighbourPoint.x()
            endPoint = QPointF(xInsideSpan ? footX : rect.center().x(), rect.top());
        }
    } else if (rect.right() <= neighbourPoint.x()) {
        // directly to the right: right edge, perpendicular foot at neighbourPoint.y()
        endPoint = QPointF(rect.right(), yInsideSpan ? footY : rect.center().y());
    } else if (rect.left() >= neighbourPoint.x()) {
        // directly to the left: left edge, perpendicular foot at neighbourPoint.y()
        endPoint = QPointF(rect.left(), yInsideSpan ? footY : rect.center().y());
    } else {
        // overlapping
        endPoint = rect.center();
    }

    return endPoint;
}

bool HsmTransition::findStraightPerpendicularConnection(const QRectF& rectFrom,
                                                        const QRectF& rectTo,
                                                        QPointF& outFrom,
                                                        QPointF& outTo) {
    // A straight perpendicular segment is possible only when the two rects face each other
    // across a gap AND their edges overlap along the perpendicular axis:
    //   - from is fully above/below to  AND their x-spans overlap  -> vertical segment
    //   - from is fully left/right of to AND their y-spans overlap -> horizontal segment
    // The endpoints are anchored at the CENTER of the overlapping edge span, so the line is
    // perpendicular to both facing edges. Diagonal or intersecting arrangements return false.
    bool result = false;

    const qreal xOverlapLeft = qMax(rectFrom.left(), rectTo.left());
    const qreal xOverlapRight = qMin(rectFrom.right(), rectTo.right());
    const qreal yOverlapTop = qMax(rectFrom.top(), rectTo.top());
    const qreal yOverlapBottom = qMin(rectFrom.bottom(), rectTo.bottom());

    const bool xOverlap = (xOverlapLeft < xOverlapRight);
    const bool yOverlap = (yOverlapTop < yOverlapBottom);

    if (xOverlap && (rectFrom.bottom() <= rectTo.top())) {
        // from is above to: vertical line down the center of the horizontal overlap.
        const qreal cx = (xOverlapLeft + xOverlapRight) / 2.0;
        outFrom = QPointF(cx, rectFrom.bottom());
        outTo = QPointF(cx, rectTo.top());
        result = true;
    } else if (xOverlap && (rectFrom.top() >= rectTo.bottom())) {
        // from is below to: vertical line up the center of the horizontal overlap.
        const qreal cx = (xOverlapLeft + xOverlapRight) / 2.0;
        outFrom = QPointF(cx, rectFrom.top());
        outTo = QPointF(cx, rectTo.bottom());
        result = true;
    } else if (yOverlap && (rectFrom.right() <= rectTo.left())) {
        // from is left of to: horizontal line across the center of the vertical overlap.
        const qreal cy = (yOverlapTop + yOverlapBottom) / 2.0;
        outFrom = QPointF(rectFrom.right(), cy);
        outTo = QPointF(rectTo.left(), cy);
        result = true;
    } else if (yOverlap && (rectFrom.left() >= rectTo.right())) {
        // from is right of to: horizontal line across the center of the vertical overlap.
        const qreal cy = (yOverlapTop + yOverlapBottom) / 2.0;
        outFrom = QPointF(rectFrom.left(), cy);
        outTo = QPointF(rectTo.right(), cy);
        result = true;
    }

    return result;
}

void HsmTransition::onEventEditFinished() {
    auto entityPtr = modelElement<model::Transition>();

    if (mLabelEvent && entityPtr) {
        QStringList transitionConfigParts = mLabelEvent->toPlainText().split('/');

        if (transitionConfigParts.size() == 2) {
            entityPtr->setEvent(transitionConfigParts[0]);
            entityPtr->setTransitionAction(transitionConfigParts[1]);
        } else {
            entityPtr->setEvent(transitionConfigParts[0]);
        }
    }
}

void HsmTransition::onConditionEditFinished() {
    auto entityPtr = modelElement<model::Transition>();

    if (mLabelCondition && entityPtr) {
        QString conditionText = mLabelCondition->toPlainText().trimmed();

        conditionText.remove("<").remove(">");

        // split by =, ==, is using regex
        QRegularExpression re("(==|=|is)");
        QStringList conditionParts = conditionText.split(re);

        if (conditionParts.size() == 2) {
            QString expectedValueStr = conditionParts[1].toLower().trimmed();

            entityPtr->setConditionCallback(conditionParts[0]);
            // NOTE: reset to TRUE by default
            entityPtr->setExpectedConditionValue(expectedValueStr != "false");
        } else {
            entityPtr->setConditionCallback(conditionText);
            entityPtr->setExpectedConditionValue(true);
        }
    }
}

void HsmTransition::onEventLabelPositionChanged() {
    // Update offset when label is moved manually
    if (mLinePath.size() >= 2) {
        QPointF mid;
        int n = mLinePath.size();
        if (n % 2 == 0) {
            int midIdx = n / 2 - 1;
            mid = (mLinePath[midIdx] + mLinePath[midIdx + 1]) / 2.0;
        } else {
            mid = mLinePath[n / 2];
        }
        mLabelOffset = mLabel->pos() - mid;
    }
}

void HsmTransition::onModelDataChanged() {
    auto entityPtr = modelElement<model::Transition>();

    if (mLabelEvent && mLabelCondition && entityPtr) {
        QSignalBlocker block(mLabelEvent);
        QSignalBlocker block2(mLabelCondition);

        const model::ModelActionList& transitionActions = entityPtr->transitionActions();

        if (transitionActions.size() == 1) {
            mLabelEvent->setPlainText(entityPtr->event() + " / " + transitionActions.first()->serialize());
        } else if (transitionActions.size() > 1) {
            mLabelEvent->setPlainText(entityPtr->event() + " / " + tr("<multiple actions>"));
        } else {
            mLabelEvent->setPlainText(entityPtr->event());
        }

        if (entityPtr->conditionCallback().isEmpty() == false) {
            const QString condValue = (entityPtr->expectedConditionValue() == true ? "true" : "false");
            mLabelCondition->setPlainText("<<" + entityPtr->conditionCallback() + " == " + condValue + ">>");
            mLabelCondition->setVisible(true);
        } else {
            mLabelCondition->setPlainText("");
            mLabelCondition->setVisible(false);
        }

        QTimer::singleShot(0, this, SLOT(recalculateLabelPosition()));
    }

    update();  // trigger repaint
}

void HsmTransition::recalculateLine() {
    // TODO: fix connection incorectly attaching to the center of elements when they are too close
    QPointF currentMovePos;

    if (mLinePath.size() >= 2) {
        mLinePath.removeFirst();
        mLinePath.removeLast();
    }

    if ((nullptr != mFromElement) && (nullptr != mToElement)) {
        if (mFromElement != mToElement) {
            QRectF rectFrom = mapRectFromItem(mFromElement, mFromElement->elementRect());
            QRectF rectTo = mapRectFromItem(mToElement, mToElement->elementRect());

            // determine relative position of elements against each other
            // if can be one of these 8 positions or a combination of them
            //     1 2 3
            //     4 @ 5
            //     6 7 8
            //
            //    12 23
            //  14 @@@ 35
            //     @@@
            //  46 @@@ 58
            //    67 78
            //
            // 1, 3, 6, 8: angle line
            // others: Z-line

            QPointF pointFrom;
            QPointF pointTo;

            if (false == mLinePath.isEmpty()) {
                // With intermediate grips both ends use the perpendicular-foot rule from the
                // neighbouring grip (source from the first grip, destination from the last),
                // falling back to the edge centre when no perpendicular is possible.
                pointFrom = findPerpendicularEdgePoint(rectFrom, mLinePath.first());
                pointTo = findPerpendicularEdgePoint(rectTo, mLinePath.last());
            } else if (findStraightPerpendicularConnection(rectFrom, rectTo, pointFrom, pointTo)) {
                // No intermediate grips and the two elements face each other with overlapping
                // edges: connect them with a straight perpendicular segment anchored at the
                // centers of the overlapping edge spans.
            } else {
                // No intermediate grips: the source anchor is computed from the rect-to-rect
                // relationship, the destination is the perpendicular foot dropped from it.
                pointFrom = findStartingPointFromRect(rectFrom, rectTo);
                pointTo = findPerpendicularEdgePoint(rectTo, pointFrom);
            }

            mLinePath.prepend(pointFrom);
            mLinePath.append(pointTo);
        } else if (parentItem() != nullptr) {
            // self transitions
            const QRectF rectParent = parentItem()->boundingRect();
            QPointF pointFrom = rectParent.topLeft();
            QPointF pointTo;
            const int selfTransitionsIndex = parentItem()->childItems().indexOf(this);

            pointFrom.setX(pointFrom.x() + SELFTRANSITION_X_OFFSET);
            pointFrom.setY(pointFrom.y() + SELFTRANSITION_Y_OFFSET * (selfTransitionsIndex + 1));
            pointTo.setX(pointFrom.x() + SELFTRANSITION_LENGTH);
            pointTo.setY(pointFrom.y());

            mLinePath.prepend(pointFrom);
            mLinePath.append(pointTo);

            mLabel->setPos(QPoint(pointTo.x() + 10, pointTo.y() - mLabel->boundingRect().height() / 2));
        }

        if (mLinePath.size() >= 2) {
            mSrcGrip->setPos(mLinePath.first());
            mDestGrip->setPos(mLinePath.last());
        }
    } else if (nullptr != mFromElement) {
        currentMovePos = mDestGrip->renderingPos();

        QRectF rectFrom = mapRectFromItem(mFromElement, mFromElement->elementRect());
        QPointF pointFrom;

        // Source endpoint uses the perpendicular-foot rule: the neighbour is the first
        // intermediate grip, or the dragged destination position when there are none.
        if (false == mLinePath.isEmpty()) {
            pointFrom = findPerpendicularEdgePoint(rectFrom, mLinePath.first());
        } else {
            pointFrom = findPerpendicularEdgePoint(rectFrom, currentMovePos);
        }

        mLinePath.prepend(pointFrom);
        mLinePath.append(currentMovePos);
        mSrcGrip->setPos(pointFrom);
    } else if (nullptr != mToElement) {
        currentMovePos = mSrcGrip->renderingPos();

        QRectF rectTo = mapRectFromItem(mToElement, mToElement->elementRect());
        QPointF pointTo;

        // Destination endpoint uses the perpendicular-foot rule: the previous point is the
        // last intermediate grip, or the dragged source position when there are none.
        if (false == mLinePath.isEmpty()) {
            pointTo = findPerpendicularEdgePoint(rectTo, mLinePath.last());
        } else {
            pointTo = findPerpendicularEdgePoint(rectTo, currentMovePos);
        }

        mLinePath.prepend(currentMovePos);
        mLinePath.append(pointTo);
        mDestGrip->setPos(pointTo);
    } else {
        mLinePath.clear();
    }

    // store new geometry in the model
    auto ptrElement = modelElement<model::Transition>();

    if (ptrElement) {
        ptrElement->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, mLinePath);
    }

    updateBoundingRect();
    recalculateLabelPosition();
    updateValidity();
}

void HsmTransition::recalculateLabelPosition() {
    // Don't position label for self-transitions
    if (isSelfTransition() == false) {
        if (mLabel && mLinePath.size() >= 2) {
            // Always position label as central point plus offset
            QPointF mid;
            const int n = mLinePath.size();

            if (n % 2 == 0) {
                const int midIdx = n / 2 - 1;

                mid = (mLinePath[midIdx] + mLinePath[midIdx + 1]) / 2.0;
            } else {
                mid = mLinePath[n / 2];
            }

            mLabel->setPos(mid + mLabelOffset - mLabel->boundingRect().center());
        }
    }

    mLabel->relayout();
}

// =================================================================================================================
// Grip notifications
void HsmTransition::setAllGripsVisibility(const bool visible) {
    for (auto* grip : mLineGrips) {
        if (nullptr != grip) {
            grip->setVisible(visible);
        }
    }

    // Raise the whole transition above its neighbours while its grips are shown, so the grips
    // (and this transition's label) are not covered by other transitions' labels when many
    // transitions are packed closely together. Restore the default stacking once hidden.
    setZValue(visible ? ACTIVE_Z : DEFAULT_Z);
}

bool HsmTransition::onGripMoved(ElementGripItem* grip, const QPointF& delta) {
    qDebug() << "----- onGripMoved: " << delta << ", grip: " << grip;
    if ((nullptr != grip) && (mLinePath.isEmpty() == false)) {
        const int gripIndex = findGripIndex(grip);

        if (gripIndex >= 0) {
            // Inner grips changed
            if ((gripIndex > 0 && gripIndex < (mLinePath.size() - 1)) ||
                (isConnecting() == true &&
                 ((mFromElement == nullptr && grip == mSrcGrip) || (mToElement == nullptr && grip == mDestGrip)))) {
                qDebug() << "--- updated gripIndex=" << gripIndex << ", old pos=" << mLinePath[gripIndex]
                         << ", new pos=" << grip->renderingPos();
                mLinePath[gripIndex] = grip->renderingPos();
                recalculateLine();
                update();
            } else {
                qDebug() << "ignoring onGripMoved call for grip index: " << gripIndex;
            }
        } else {
            qWarning() << "unexpected onGripMoved call. grip not found in lineGrips";
        }

        if (isConnecting() == true && (0 == gripIndex || gripIndex == (mLinePath.size() - 1))) {
            // Check if there is an element under cursor and highlight it
            QPointer<HsmElement> element =
                ViewUtils::topHsmElementAt(scene(), grip->renderingPosScene(), false, true, false, false, nullptr);

            if (mLastConnectionTarget != element) {
                if (mLastConnectionTarget) {
                    mLastConnectionTarget->hightlight(false);
                }

                if (element) {
                    element->hightlight(true);
                }

                mLastConnectionTarget = element;
            }
        }
    } else {
        qWarning() << "unexpected onGripMoved call. grip is null or linePath is empty";
    }

    return true;
}

void HsmTransition::onGripDoubleClick(ElementGripItem* grip) {
    auto it = std::find(mLineGrips.begin(), mLineGrips.end(), grip);

    if (it != mLineGrips.end()) {
        grip->setParentItem(nullptr);
        mLinePath.removeAt(it - mLineGrips.begin());
        // Dont delete it right away because this signal is sent from a context of ElementGripItem
        grip->deleteLater();
        mLineGrips.erase(it);

        recalculateLine();
    }
}

void HsmTransition::onGripMoveEnterEvent(ElementGripItem* gripItem) {
    // NOTE: old element stays connected, but it's impossible for user to resize it
    if (gripItem == mSrcGrip) {
        mPrevConnectedElement = mFromElement;
        mFromElement = nullptr;
        mConnecting = true;
    } else if (gripItem == mDestGrip) {
        mPrevConnectedElement = mToElement;
        mToElement = nullptr;
        mConnecting = true;
    }

    recalculateLine();
}

void HsmTransition::onGripMoveLeaveEvent(ElementGripItem* gripItem) {
    // TODO: implement
    if (isConnecting() == true) {
        if (gripItem == mSrcGrip || gripItem == mDestGrip) {
            // check if there is an element which accepts connections
            HsmElement* targetElement =
                ViewUtils::topHsmElementAt(scene(), gripItem->renderingPosScene(), false, true, false, false, nullptr);

            if (nullptr == targetElement) {
                targetElement = mPrevConnectedElement;
            }

            targetElement->hightlight(false);

            mConnecting = false;
            mLastConnectionTarget.clear();
            recalculateLine();

            if (gripItem == mSrcGrip) {
                emit transitionReconnected(modelId(), targetElement->modelId(), mToElement->modelId());
            } else if (gripItem == mDestGrip) {
                emit transitionReconnected(modelId(), mFromElement->modelId(), targetElement->modelId());
            }
        }
    }
}

void HsmTransition::beginEventEditMode() {
    if (mLabelEvent != nullptr) {
        mLabelEvent->beginEditMode();
    }
}

void HsmTransition::beginEventTypingMode(const QString& newText) {
    if (mLabelEvent != nullptr) {
        mLabelEvent->beginTypingMode(newText);
    }
}

int HsmTransition::findGripIndex(const ElementGripItem* grip) {
    int gripIndex = -1;

    // TODO: use foreach or std::find
    for (int i = 0; i < mLineGrips.size(); ++i) {
        if (mLineGrips[i] == grip) {
            gripIndex = i;
            break;
        }
    }

    return gripIndex;
}

QVariant HsmTransition::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemSelectedHasChanged == change) {
        if (true == isSelected()) {
            setAllGripsVisibility(true);
        } else {
            // Hide grips unless the transition is currently hovered
            if (false == isUnderMouse()) {
                setAllGripsVisibility(false);
            }
        }
    }
    // No need to connect to xChanged/yChanged
    return QGraphicsItem::itemChange(change, value);
}

void HsmTransition::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "HsmTransition::mouseDoubleClickEvent";

    // block creation of intermediate points for self-transitions
    if (isSelfTransition() == false) {
        std::tuple<bool, QPointF, int> result = isPointOnTheLine(event->pos());

        if (true == std::get<0>(result)) {
            // TODO: use smart pointer
            ElementGripItem* grip = new ElementGripItem(this);
            QPointF gripPos = mapToScene(std::get<1>(result));

            grip->init();
            grip->setPos(std::get<1>(result));
            grip->setVisible(true);
            mLinePath.insert(std::get<2>(result) + 1, std::get<1>(result));
            // TODO: validate
            mLineGrips.insert(mLineGrips.begin() + std::get<2>(result) + 1, grip);

            // Recalculate line in case label position needs to be changed
            recalculateLine();
        }
    }

    QGraphicsItem::mouseDoubleClickEvent(event);
}

// =================================================================================================================
// Utils
std::tuple<bool, QPointF, int> HsmTransition::isPointOnTheLine(const QPointF& pos) {
    bool intersects = false;
    QPointF linePoint;
    int prevPointIndex = -1;
    const int cornersCount = mLinePath.size();

    for (int i = 0; i < cornersCount; ++i) {
        if (QLineF(mLinePath[i], pos).length() <= SELECTION_DISTANCE) {
            // Is pressed point close enough of the grip-point
            intersects = true;
            break;
        } else {
            if (i < mLinePath.count() - 1) {
                QLineF line(mLinePath[i], mLinePath[i + 1]);
                // Is pressed point close enough of the line
                QLineF line2(pos, pos + QPointF(SELECTION_DISTANCE, SELECTION_DISTANCE));

                line2.setAngle(line.angle() + 90);

                QLineF::IntersectionType intersection = line.intersects(line2, &linePoint);

                if (QLineF::BoundedIntersection == intersection) {
                    intersects = true;
                } else {
                    line2.setAngle(line.angle() - 90);
                    intersection = line.intersects(line2, &linePoint);
                    intersects = (QLineF::BoundedIntersection == intersection);
                }

                if (true == intersects) {
                    prevPointIndex = i;
                    break;
                }
            }
        }
    }

    return std::make_tuple(intersects, linePoint, prevPointIndex);
}

};  // namespace view
