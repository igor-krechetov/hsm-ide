#include "HsmTransition.hpp"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <cmath>

#include "private/ElementGripItem.hpp"
#include "view/common/ViewUtils.hpp"
namespace view {

HsmTransition::HsmTransition()
    : HsmElement(HsmElementType::TRANSITION)
    , mFromElement(nullptr)
    , mToElement(nullptr)
    , mDebugShowSelectionPolygon(false) {
    qDebug() << "CREATE: HsmTransition: " << this;
    // NOTE: there are no grips at the beginning and and of the poly-line
    // TODO: fix me
    // self.lineGrips = [None, None]

    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(5);
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

void HsmTransition::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    setConnectionGripsVisibility(true);
}

void HsmTransition::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    setConnectionGripsVisibility(isSelected());
}

void HsmTransition::updateBoundingRect(const QRectF& newRect) {
    prepareGeometryChange();
    updateSelectionPolygon();

    mOuterRect = mLinePath.boundingRect().normalized().adjusted(-static_cast<int>(SELECTION_DISTANCE),
                                                                -static_cast<int>(SELECTION_DISTANCE),
                                                                static_cast<int>(SELECTION_DISTANCE),
                                                                static_cast<int>(SELECTION_DISTANCE));
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
    if (false == isSelected()) {
        painter->setPen(Qt::SolidLine);
    } else {
        painter->setPen(Qt::DashLine);
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
        double arrowSize = 10.0;
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

    // TODO: Debug code. Remove later
    if (true == mDebugShowSelectionPolygon) {
        painter->setPen(QPen("red"));
        painter->drawPath(mSelectionPath);
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
        mDestGrip->setPos(pos);
    } else {
        qDebug() << "WARNING: unexpected moveConnectionTo call";
    }
}

void HsmTransition::connectElements(HsmElement* fromElement, HsmElement* toElement) {
    qDebug() << Q_FUNC_INFO;
    disconnectElements();
    mFromElement = fromElement;
    mToElement = toElement;
    connect(mFromElement, SIGNAL(geometryChanged(HsmResizableElement*)), this, SLOT(recalculateLine()));
    connect(mToElement, SIGNAL(geometryChanged(HsmResizableElement*)), this, SLOT(recalculateLine()));
    recalculateLine();
}

void HsmTransition::disconnectElements() {
    qDebug() << Q_FUNC_INFO;
    if (mFromElement != nullptr) {
        try {
            QObject::disconnect(mFromElement, SIGNAL(geometryChanged(HsmResizableElement*)), this, SLOT(recalculateLine()));
        } catch (...) {
            qDebug() << "from not connected";
        }
    }
    if (mToElement != nullptr) {
        try {
            QObject::disconnect(mToElement, SIGNAL(geometryChanged(HsmResizableElement*)), this, SLOT(recalculateLine()));
        } catch (...) {
            qDebug() << "to not connected";
        }
    }

    mFromElement = nullptr;
    mToElement = nullptr;
}

QPointer<HsmElement> HsmTransition::connectionCandidate() const {
    return mLastConnectionTarget;
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

void HsmTransition::recalculateLine() {
    qDebug() << Q_FUNC_INFO;
    QPointF currentMovePos;

    if (mLinePath.size() >= 2) {
        mLinePath.removeFirst();
        mLinePath.removeLast();
    }

    // TODO: decide how to snap transition to items
    if ((nullptr != mFromElement) && (nullptr != mToElement)) {
        // posFrom = self.fromElement.mapToScene(self.fromElement.pos())
        // posTo = self.fromElement.mapToScene(self.toElement.pos())
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
        // rectFrom = QRectF()
        // rectTo = QRectF()

        // cout << "from: " << rectFrom << ", to: " << rectTo << endl;
        // cout << "from.b=" << rectFrom.bottom() << ", to.t=" << rectTo.top() << endl;

        // if (mLinePath.size() > 0) {
        //     mLinePath.removeFirst();
        //     mLinePath.removeLast();
        // }
        // mLinePath.clear();

        // for (auto& grip: mLineGrips) {
        //     mLinePath.append(grip->pos());
        // }
        // mLinePath.removeFirst();
        // mLinePath.removeLast();

        QPointF pointFrom;
        QPointF pointTo;

        if (false == mLinePath.isEmpty()) {
            pointFrom = findStartingPointFromPoint(rectFrom, mLinePath.first());
            pointTo = findStartingPointFromPoint(rectTo, mLinePath.last());
        } else {
            pointFrom = findStartingPointFromRect(rectFrom, rectTo);
            pointTo = findStartingPointFromRect(rectTo, rectFrom);
        }

        mLinePath.prepend(pointFrom);
        mLinePath.append(pointTo);
        // qDebug() << "mLinePath: " << mLinePath.size() << ": " << mLinePath;

        mSrcGrip->setPos(mLinePath.first());
        mDestGrip->setPos(mLinePath.last());
    } else if (nullptr != mFromElement) {
        currentMovePos = mDestGrip->pos();

        QRectF rectFrom = mapRectFromItem(mFromElement, mFromElement->elementRect());
        QPointF pointFrom;

        if (false == mLinePath.isEmpty()) {
            pointFrom = findStartingPointFromPoint(rectFrom, mLinePath.first());
        } else {
            pointFrom = findStartingPointFromPoint(rectFrom, currentMovePos);
        }

        mLinePath.prepend(pointFrom);
        mLinePath.append(currentMovePos);
        mSrcGrip->setPos(pointFrom);
        // qDebug() << "DEST changed: " << mLinePath;
    } else if (nullptr != mToElement) {
        currentMovePos = mSrcGrip->pos();

        QRectF rectTo = mapRectFromItem(mToElement, mToElement->elementRect());
        QPointF pointTo;

        if (false == mLinePath.isEmpty()) {
            pointTo = findStartingPointFromPoint(rectTo, mLinePath.last());
        } else {
            pointTo = findStartingPointFromPoint(rectTo, currentMovePos);
        }

        mLinePath.prepend(currentMovePos);
        mLinePath.append(pointTo);
        mDestGrip->setPos(pointTo);
        // qDebug() << "SRC changed: " << mLinePath;
    } else {
        mLinePath.clear();
    }

    updateBoundingRect();
}

// =================================================================================================================
// Grip notifications
void HsmTransition::setConnectionGripsVisibility(const bool visible) {
    mSrcGrip->setVisible(visible);
    mDestGrip->setVisible(visible);
}

bool HsmTransition::onGripMoved(const ElementGripItem* grip, const QPointF& pos) {
    if (mLinePath.isEmpty() == false) {
        const int gripIndex = findGripIndex(grip);
        qDebug() << "HsmTransition::onGripMoved: index=" << gripIndex << ", pos=" << pos;

        if (gripIndex >= 0) {
            // Inner grips changed
            qDebug() << "conditions";
            if ((gripIndex > 0 && gripIndex < (mLinePath.size() - 1)) ||
                (isConnecting() == true && ((mFromElement == nullptr && grip == mSrcGrip) ||
                                            (mToElement == nullptr && grip == mDestGrip)) ) ) {
                qDebug() << "recalc, mLinePath.size=" << mLinePath.size();
                mLinePath[gripIndex] = pos;
                recalculateLine();
                updateBoundingRect();
            }
        }

        if (isConnecting() == true && (0 == gripIndex || gripIndex == (mLinePath.size()-1))) {
            qDebug() << "highlight";
            // Check if there is an element under cursor and highlight it
            QPointer<HsmElement> element = ViewUtils::topHsmElementAt(scene(), pos, true, false);

            qDebug() << "cond2";
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
    }

    return true;
}

void HsmTransition::onGripDoubleClick(ElementGripItem* grip) {
    qDebug() << Q_FUNC_INFO;
    // const int gripIndex = findGripIndex(grip);
    auto it = std::find(mLineGrips.begin(), mLineGrips.end(), grip);

    if (it != mLineGrips.end()) {
        grip->setParentItem(nullptr);
        // TODO:validate
        mLinePath.removeAt(it - mLineGrips.begin());
        // TODO: check for memory leaks
        // delete mLineGrips.takeAt(gripIndex);
        delete grip;
        mLineGrips.erase(it);
        updateBoundingRect();
    }
}

void HsmTransition::onGripMoveEnterEvent(ElementGripItem* gripItem) {
    qDebug() << Q_FUNC_INFO;

    // NOTE: old element stays connected, but it's impossible for user to resize it
    if (gripItem == mSrcGrip) {
        qDebug() << "SRC grip moved";
        mPrevConnectedElement = mFromElement;
        mFromElement = nullptr;
        mConnecting = true;
    } else if (gripItem == mDestGrip) {
        qDebug() << "DEST grip moved";
        mPrevConnectedElement = mToElement;
        mToElement = nullptr;
        mConnecting = true;
    }

    recalculateLine();
}

void HsmTransition::onGripMoveLeaveEvent(ElementGripItem* gripItem) {
    qDebug() << Q_FUNC_INFO;
    // TODO: implement
    if (isConnecting() == true) {
        if (gripItem == mSrcGrip || gripItem == mDestGrip) {
            QPointF pos = gripItem->pos();
            HsmElement* targetElement = ViewUtils::topHsmElementAt(scene(), pos, true, false);

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

// # def onGripLostFocus(self, grip):
// #     # TODO
// #     return True

int HsmTransition::findGripIndex(const ElementGripItem* grip) {
    int gripIndex = -1;

    // TODO: use foreach or std::find
    for (int i = 0; i < mLineGrips.size(); ++i) {
        if (mLineGrips[i] == grip) {
            gripIndex = i;
            break;
        }
    }

    // return (gripIndex > 0 && gripIndex < (mLineGrips.size()-1) ? gripIndex : -1);
    return gripIndex;
}

QVariant HsmTransition::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemSelectedHasChanged == change) {
        setConnectionGripsVisibility(isSelected());
    }

    return QGraphicsItem::itemChange(change, value);
}

void HsmTransition::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "HsmTransition::mouseDoubleClickEvent";
    std::tuple<bool, QPointF, int> result = isPointOnTheLine(event->pos());

    if (true == std::get<0>(result)) {
        // TODO: use smart pointer
        ElementGripItem* grip = new ElementGripItem(this);
        QPointF gripPos = mapToScene(std::get<1>(result));

        grip->init();
        grip->setPos(std::get<1>(result));
        mLinePath.insert(std::get<2>(result) + 1, std::get<1>(result));
        // TODO: validate
        mLineGrips.insert(mLineGrips.begin() + std::get<2>(result) + 1, grip);
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
