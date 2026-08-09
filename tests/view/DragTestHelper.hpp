#ifndef DRAGTESTHELPER_HPP
#define DRAGTESTHELPER_HPP

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPointF>

#include "view/elements/HsmStateElement.hpp"

namespace DragTestHelper {

/**
 * @brief Put an element into drag state by simulating a mouse press event.
 *
 * This sends a QGraphicsSceneMouseEvent through the scene to trigger
 * HsmElement::mousePressEvent, which sets mDragState = PREPARE and
 * mPrevItemPos = pos(). Then sets drag mode to SINGLE so isDragged()
 * returns true after the first move.
 */
inline void beginDrag(view::HsmElement* element) {
    Q_ASSERT(element != nullptr);
    Q_ASSERT(element->scene() != nullptr);

    QGraphicsScene* scene = element->scene();

    // Create a mouse press event at the element's scene position
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setButtons(Qt::LeftButton);
    pressEvent.setScenePos(element->scenePos());
    pressEvent.setPos(element->mapFromScene(element->scenePos()));

    // Send through the scene — this calls QGraphicsItem::mousePressEvent
    // which is intercepted by HsmElement::mousePressEvent
    scene->sendEvent(element, &pressEvent);

    // Set drag mode to SINGLE so isDragged() returns true
    element->setDragMode(true);
}

/**
 * @brief Shared state for tracking simulated cursor positions during drag.
 * This models Qt's behavior where the cursor position accumulates
 * independently of the element's snapped position.
 */
inline QHash<view::HsmElement*, QPointF>& dragCursorPositions() {
    static QHash<view::HsmElement*, QPointF> map;
    return map;
}

/**
 * @brief Simulate one frame of mouse movement during a drag.
 *
 * Models Qt's actual drag behavior: the cursor position accumulates
 * independently of the element's snapped position. Each call adds sceneDelta
 * to an internal unsnapped cursor tracker, then computes the target local
 * position from that accumulated cursor position. This matches how Qt passes
 * value = pressPos + totalMouseDelta to itemChange, independent of snap.
 *
 * @param element The element being dragged (must be in drag state)
 * @param sceneDelta How much the mouse moved in scene coordinates this frame
 * @return The element's scene position after the move was committed (may be snapped)
 */
inline QPointF simulateDragMove(view::HsmElement* element, const QPointF& sceneDelta) {
    Q_ASSERT(element != nullptr);

    auto& cursorPositions = dragCursorPositions();

    if (!cursorPositions.contains(element)) {
        // First call after beginDrag: initialize cursor at element's current scene pos
        cursorPositions[element] = element->scenePos();
    }

    // Accumulate mouse movement (independent of element's snapped position)
    cursorPositions[element] += sceneDelta;
    QPointF targetScenePos = cursorPositions[element];

    // Convert target scene position to local (parent) coordinates
    QPointF targetLocalPos;
    if (element->parentItem() == nullptr) {
        targetLocalPos = targetScenePos;
    } else {
        targetLocalPos = element->parentItem()->mapFromScene(targetScenePos);
    }

    // Call setPos which triggers itemChange(ItemPositionChange, targetLocalPos)
    element->setPos(targetLocalPos);

    // Return the committed scene position (after snapping and layout adjustments)
    return element->scenePos();
}

/**
 * @brief End the drag by simulating a mouse release event.
 *
 * Sends a QGraphicsSceneMouseEvent to trigger HsmElement::mouseReleaseEvent,
 * which resets mDragState and mDragMode. Also cleans up cursor tracking state.
 */
inline void endDrag(view::HsmElement* element) {
    Q_ASSERT(element != nullptr);
    Q_ASSERT(element->scene() != nullptr);

    QGraphicsScene* scene = element->scene();

    // Create a mouse release event at the element's current scene position
    QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
    releaseEvent.setButton(Qt::LeftButton);
    releaseEvent.setButtons(Qt::NoButton);
    releaseEvent.setScenePos(element->scenePos());
    releaseEvent.setPos(element->mapFromScene(element->scenePos()));

    // Send through the scene — this calls HsmElement::mouseReleaseEvent
    scene->sendEvent(element, &releaseEvent);

    // Clean up cursor tracking state
    dragCursorPositions().remove(element);
}

}  // namespace DragTestHelper

#endif  // DRAGTESTHELPER_HPP
