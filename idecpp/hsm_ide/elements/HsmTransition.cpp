
#include "HsmTransition.hpp"

HsmTransition::HsmTransition()
    : mFromElement(nullptr)
    , mToElement(nullptr)
    , mDebugShowSelectionPolygon(false) {
    // NOTE: there are no grips at the beginning and and of the poly-line
    // TODO: fix me
    // self.lineGrips = [None, None]

    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(0);
    updateBoundingRect();
}

void HsmTransition::updateBoundingRect() {
    prepareGeometryChange();
    updateSelectionPolygon();

    mOuterRect = mLinePath.boundingRect().normalized().adjusted(-static_cast<int>(Consts::SELECTION_DISTANCE),
                                                                -static_cast<int>(Consts::SELECTION_DISTANCE),
                                                                static_cast<int>(Consts::SELECTION_DISTANCE),
                                                                static_cast<int>(Consts::SELECTION_DISTANCE));
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

    for (int i = 0; i < (mLinePath.size() - 1); ++i) {
        painter->drawLine(mLinePath[i], mLinePath[i + 1]);
    }

    // TODO: Debug code. Remove later
    if (true == mDebugShowSelectionPolygon) {
        painter->setPen(QPen("red"));
        painter->drawPath(mSelectionPath);
    }
}

void HsmTransition::beginConnection(HsmElement* fromElement, const QPointF& pos) {
    // print(f"{fromElement}, {pos}")
    removeConnection();
    mFromElement = fromElement;
    mCurrentMovePos = pos;
    recalculateLine();
}

bool HsmTransition::isConnecting() const {
    return (nullptr != mFromElement) && (nullptr == mToElement);
}

void HsmTransition::moveConnectionTo(const QPointF& pos) {
    if (true == isConnecting()) {
        mCurrentMovePos = pos;
        recalculateLine();
    } else {
        qDebug() << "WARNING: unexpected moveConnectionTo call";
    }
}

void HsmTransition::connectElements(HsmElement* fromElement, HsmElement* toElement) {
    removeConnection();
    mFromElement = fromElement;
    mToElement = toElement;
    recalculateLine();
    connect(mFromElement, &HsmElement::geometryChangedEvent, this, &HsmTransition::recalculateLine);
    connect(mToElement, &HsmElement::geometryChangedEvent, this, &HsmTransition::recalculateLine);
}

void HsmTransition::recalculateLine() {
    // TODO: Recalculate start and end positions when related items were moved
}

void HsmTransition::geometryChangedEvent() {
    recalculateLine();
}

void HsmTransition::removeConnection() {
    if (mFromElement != nullptr) {
        try {
            QObject::disconnect(mFromElement, &HsmElement::geometryChanged, this, &HsmTransition::recalculateLine);
        } catch (...) {
            qDebug() << "from not connected";
        }
    }
    if (mToElement != nullptr) {
        try {
            QObject::disconnect(mToElement, &HsmElement::geometryChanged, this, &HsmTransition::recalculateLine);
        } catch (...) {
            qDebug() << "to not connected";
        }
    }

    mFromElement = nullptr;
    mToElement = nullptr;
    // recalculateLine();
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

void HsmTransition::recalculateLine(QObject* element) {
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

        if (mLinePath.size() > 0) {
            mLinePath.removeFirst();
            mLinePath.removeLast();
        }

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
    } else if (nullptr != mFromElement) {
        QRectF rectFrom = mapRectFromItem(mFromElement, mFromElement->elementRect());
        QPointF pointFrom = findStartingPointFromPoint(rectFrom, mCurrentMovePos);

        mLinePath.clear();
        mLinePath.append(pointFrom);
        mLinePath.append(mCurrentMovePos);
    } else {
        mLinePath.clear();
    }

    updateBoundingRect();
}

// =================================================================================================================
// Grip notifications
bool HsmTransition::onGripMoved(HsmTransitionGripItem* grip, const QPointF& pos) {
    const int gripIndex = findGripIndex(grip);

    if (gripIndex >= 0) {
        mLinePath[gripIndex] = pos;
        recalculateLine();
        updateBoundingRect();
    }

    return true;
}

void HsmTransition::onGripDoubleClick(HsmTransitionGripItem* grip) {
    const int gripIndex = findGripIndex(grip);

    if (gripIndex >= 0) {
        grip->setParentItem(nullptr);
        mLinePath.removeAt(gripIndex);
        delete mLineGrips.takeAt(gripIndex);
        updateBoundingRect();
    }
}

// # def onGripLostFocus(self, grip):
// #     # TODO
// #     return True

int HsmTransition::findGripIndex(HsmTransitionGripItem* grip) {
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

void HsmTransition::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    // TODO: replace with tuple
    QPair<bool, QPointF> result = isPointOnTheLine(event->pos());

    if (result.first) {
        // TODO: use smart pointer
        HsmTransitionGripItem* grip = new HsmTransitionGripItem(this);

        grip->setPos(result.second);
        mLinePath.insert(result.third + 1, result.second);
        mLineGrips.insert(result.third + 1, grip);
    }

    QGraphicsItem::mouseDoubleClickEvent(event);
}

// -----------------------------------
// Utils
// TODO: replace with tuple
QPair<bool, QPointF> HsmTransition::isPointOnTheLine(const QPointF& pos) {
    bool intersects = false;
    QPointF linePoint;
    int prevPointIndex = -1;
    const int cornersCount = mLinePath.size();

    for (int i = 0; i < cornersCount; ++i) {
        // QPointF point1 = mLinePath[i];
        // QPointF point2 = mLinePath[(i + 1) % cornersCount];
        // QPainterPath path;

        // path.moveTo(point1);
        // path.lineTo(point2);

        // if (path.length() > SELECTION_DISTANCE) {
        //     QRectF rect = QRectF(point1, point2)
        //                       .normalized()
        //                       .adjusted(-SELECTION_DISTANCE, -SELECTION_DISTANCE, SELECTION_DISTANCE, SELECTION_DISTANCE);

        //     if (rect.contains(pos.toPoint())) {
        //         intersects = true;
        //         linePoint = QPointF((point1.x() + point2.x()) / 2, (point1.y() + point2.y()) / 2);
        //         prevPointIndex = i;
        //         break;
        //     }
        // }
        if (QLineF(self.linePath[i], pos).length() <= SELECTION_DISTANCE) {
            // Is pressed point close enough of the grip-point
            intersects = true;
            break
        }
        else {
            if (i < self.linePath.count() - 1) {
                QLineF line(self.linePath[i], self.linePath[i + 1]);
                // Is pressed point close enough of the line
                QLine line2(pos, pos + QPointF(self.SELECTION_DISTANCE, self.SELECTION_DISTANCE));

                line2.setAngle(line.angle() + 90);

                QPointF linePoint;
                QLineF::IntersectionType intersection = line.intersects(line2, &linePoint);

                if (QLineF::BoundedIntersection == intersection) {
                    intersects = true;
                }
                else {
                    line2.setAngle(line.angle() - 90);
                    intersection = line.intersects(line2, &linePoint);
                    intersects = (QLineF::BoundedIntersection == intersection);
                }

                if (true ==intersects) {
                    prevPointIndex = i;
                    break;
                }
            }
        }
    }

    // TODO: make tuple
    return qMakePair(intersects, linePoint, prevPointIndex);
}
