#include "HsmTransition.hpp"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsScene>
#include <QPointF>
#include <QPolygonF>
#include <QSignalBlocker>
#include <cmath>

#include "model/Transition.hpp"
#include "private/ElementGripItem.hpp"
#include "private/HsmStateTextItem.hpp"
#include "view/common/ViewUtils.hpp"
#include "HsmStateElement.hpp"

/*
TODO: sometimes self-transitions get positioned incorrectly (shifted to the left or right) when moving them around
*/

namespace view {

HsmTransition::HsmTransition()
    : HsmElement(HsmElementType::TRANSITION, QSizeF(1, 1))
    , mFromElement(nullptr)
    , mToElement(nullptr)
    , mDebugShowSelectionPolygon(false) {
    qDebug() << "CREATE: HsmTransition: " << this;
    // NOTE: there are no grips at the beginning and end of the poly-line
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

void HsmTransition::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    HsmElement::init(modelEntity);

    // Create label
    mLabel = new HsmStateTextItem(this);

    connect(mLabel, &HsmStateTextItem::editingFinished, this, &HsmTransition::onEventEditFinished);
    connect(mLabel, &HsmStateTextItem::positionChanged, this, &HsmTransition::onEventLabelPositionChanged);

    // Pull latest data from the model
    onModelDataChanged();
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
    disconnectElements();

    if (nullptr != fromElement && nullptr != toElement) {
        mFromElement = fromElement;
        mToElement = toElement;

        // NOTE: we always add transition to the parent state of fromElement
        auto parent = fromElement->hsmParentItem();

        // TODO: how do we add it to substates?
        if (nullptr != fromElement && fromElement == toElement) {
            qDebug() << "---- added SELF TRANSITION to " << fromElement;
            fromElement->addChildItem(this);
        } else if (nullptr == parent) {
            qDebug() << "---- added TRANSITION to SCEENE" << scene();
            fromElement->scene()->addItem(this);
        } else {
            qDebug() << "---- added TRANSITION to " << parent;
            parent->addChildItem(this);
        }

        if (isSelfTransition() == false) {
            connect(mFromElement, SIGNAL(geometryChanged(HsmElement*)), this, SLOT(recalculateLine()));
            connect(mToElement, SIGNAL(geometryChanged(HsmElement*)), this, SLOT(recalculateLine()));
        }

        recalculateLine();

        if (mLabel) {
            mLabel->makeMovable(mFromElement != mToElement);
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

void HsmTransition::onEventEditFinished() {
    auto entityPtr = modelElement<model::Transition>();

    if (mLabel && entityPtr) {
        entityPtr->setEvent(mLabel->toPlainText());
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

    if (mLabel && entityPtr) {
        QSignalBlocker block(mLabel);
        mLabel->setPlainText(entityPtr->event());
    }

    update();  // trigger repaint
}

void HsmTransition::recalculateLine() {
    qDebug() << "---- recalculateLine" << this;
    // TODO: fix connection incorectly attaching to the center of elements when they are too close
    QPointF currentMovePos;

    if (mLinePath.size() >= 2) {
        mLinePath.removeFirst();
        mLinePath.removeLast();
    }

    // TODO: decide how to snap transition to items
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
                pointFrom = findStartingPointFromPoint(rectFrom, mLinePath.first());
                pointTo = findStartingPointFromPoint(rectTo, mLinePath.last());
            } else {
                pointFrom = findStartingPointFromRect(rectFrom, rectTo);
                pointTo = findStartingPointFromRect(rectTo, rectFrom);
            }

            mLinePath.prepend(pointFrom);
            mLinePath.append(pointTo);
            // qDebug() << "mLinePath: " << mLinePath.size() << ": " << mLinePath;
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

            // qDebug() << "parentItem()->boundingRect()" << rectParent;
            // qDebug() << pointFrom << pointTo;

            mLinePath.prepend(pointFrom);
            mLinePath.append(pointTo);

            mLabel->setPos(QPoint(pointTo.x() + 10, pointTo.y() - mLabel->boundingRect().height() / 2));
        }

        if (mLinePath.size() >= 2) {
            mSrcGrip->setPos(mLinePath.first());
            mDestGrip->setPos(mLinePath.last());
        }
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

    // Don't position label for self-transitions
    if (isSelfTransition() == false && mLabel && mLinePath.size() >= 2) {
        // Always position label as central point plus offset
        QPointF mid;
        const int n = mLinePath.size();

        if (n % 2 == 0) {
            const int midIdx = n / 2 - 1;

            mid = (mLinePath[midIdx] + mLinePath[midIdx + 1]) / 2.0;
        } else {
            mid = mLinePath[n / 2];
        }

        QSignalBlocker block(mLabel);
        mLabel->setPos(mid + mLabelOffset);
    }
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
        // qDebug() << "HsmTransition::onGripMoved: index=" << gripIndex << ", pos=" << pos;

        if (gripIndex >= 0) {
            // Inner grips changed
            if ((gripIndex > 0 && gripIndex < (mLinePath.size() - 1)) ||
                (isConnecting() == true &&
                 ((mFromElement == nullptr && grip == mSrcGrip) || (mToElement == nullptr && grip == mDestGrip)))) {
                mLinePath[gripIndex] = pos;
                recalculateLine();
                updateBoundingRect();
            }
        }

        if (isConnecting() == true && (0 == gripIndex || gripIndex == (mLinePath.size() - 1))) {
            // Check if there is an element under cursor and highlight it
            QPointer<HsmElement> element = ViewUtils::topHsmElementAt(scene(), mapToScene(pos), false, true, false, nullptr);

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
            // check if there is an element which accepts connections
            HsmElement* targetElement = ViewUtils::topHsmElementAt(scene(), mapToScene(pos), false, true, false, nullptr);

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
        setConnectionGripsVisibility(isSelected());
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
