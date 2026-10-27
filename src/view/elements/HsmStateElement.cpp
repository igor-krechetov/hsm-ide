#include "HsmStateElement.hpp"

#include <QColor>
#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QTextDocument>
#include <cmath>

#include "HsmTransition.hpp"
#include "model/elements/RegularState.hpp"
#include "private/HsmStateBodySection.hpp"
#include "private/HsmStateTextItem.hpp"
#include "view/elements/ElementTypeIds.hpp"
#include "view/theme/ThemeManager.hpp"

namespace view {

HsmStateElement::HsmStateElement()
    : HsmRectangularElement(HsmElementType::STATE) {}

HsmStateElement::HsmStateElement(const QSizeF& size)
    : HsmRectangularElement(HsmElementType::STATE, size) {}

void HsmStateElement::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    HsmRectangularElement::init(modelEntity);

    // NOTE: all objects will be deleted using parent-child mechanism, so need for explicit cleanup or smart pointers
    // Create header section
    mStateNameLabel = new HsmStateTextItem(this, this);  // Make editable label a direct child
    // Other sections
    mSelfTransitionsSection = new QGraphicsRectItem(this);
    mPropertiesSection = new QGraphicsTextItem("Properties", this);
    mBodySection = new HsmStateBodySection(this);
    // Separator lines
    mHeaderSeparator = new QGraphicsLineItem(this);
    mSelfTransitionsSeparator = new QGraphicsLineItem(this);
    mPropertiesSeparator = new QGraphicsLineItem(this);

    connect(mStateNameLabel->document(), &QTextDocument::contentsChanged, this, &HsmStateElement::onStateNameChanged);
    connect(mStateNameLabel, &HsmStateTextItem::editingFinished, this, &HsmStateElement::onStateNameEditFinished);
    connect(mStateNameLabel, &HsmStateTextItem::textGeometryChanged, this, &HsmStateElement::centerHeader);
    connect(mBodySection, &HsmStateBodySection::substatesChanged, this, &HsmStateElement::onSubstatesChanged);

    mSelfTransitionsSection->setPen(Qt::NoPen);
    mBodySection->setPen(Qt::NoPen);

    const auto applyTheme = [this]() {
        const auto& theme = ThemeManager::instance().theme();

        mStateNameLabel->setDefaultTextColor(mHasDuplicateName ? theme.node.duplicateWarningColor : theme.node.textColor);
        mPropertiesSection->setDefaultTextColor(theme.node.textColor);
        mHeaderSeparator->setPen(theme.node.borderPen);
        mSelfTransitionsSeparator->setPen(theme.node.borderPen);
        mPropertiesSeparator->setPen(theme.node.borderPen);
    };

    applyTheme();
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this, applyTheme]() {
        applyTheme();
        update();
    });

    onModelDataChanged();
    layoutSections();
}

void HsmStateElement::onStateNameChanged() {
    // Only update header position, do not update model here
    centerHeader();
}

void HsmStateElement::onStateNameEditFinished() {
    auto entityPtr = modelElement<model::RegularState>();

    if (mStateNameLabel && entityPtr) {
        entityPtr->setName(mStateNameLabel->toPlainText());
    }
}

void HsmStateElement::setDuplicateNameWarning(const bool duplicated) {
    if (mHasDuplicateName != duplicated) {
        mHasDuplicateName = duplicated;

        if (mStateNameLabel) {
            const auto& theme = ThemeManager::instance().theme();
            mStateNameLabel->setDefaultTextColor(duplicated ? theme.node.duplicateWarningColor : theme.node.textColor);
        }

        if (duplicated) {
            setToolTip(tr("Duplicate state name. State names must be unique across the model."));
        } else {
            setToolTip(QString());
        }

        update();
    }
}

void HsmStateElement::centerHeader() {
    if (mStateNameLabel) {
        mStateNameLabel->setTextWidth(-1);  // Use minimum width based on content
        QRectF textRect = mStateNameLabel->boundingRect();
        qreal x = mOuterRect.left() + (mOuterRect.width() - textRect.width()) / 2;
        qreal y = mOuterRect.top() + 10.0;  // Top padding within header section
        mStateNameLabel->setPos(x, y);
    }
}

QList<QGraphicsItem*> HsmStateElement::hsmChildItems() const {
    return (nullptr != mBodySection ? mBodySection->childItems() : QList<QGraphicsItem*>());
}

bool HsmStateElement::hasSubstates() const {
    return (nullptr != mBodySection ? mBodySection->hasSubstates() : false);
}

// bool HsmStateElement::isDirectChild(HsmElement* item) const {
//     return (nullptr != mBodySection ? mBodySection->childItems().contains(item) : false)
// }

// QRectF HsmStateElement::childrenRect() const {
//     QRectF rect;

//     if (mBodySection) {
//         for (QGraphicsItem* child : childItems()) {
//             QVariant userType = child->data(USERDATA_HSM_ELEMENT_TYPE);

//             if (userType.isValid()) {
//                 rect = rect.united(child->mapRectToParent(child->boundingRect()));
//             }
//         }
//     }

//     return rect;
// }

void HsmStateElement::addChildItem(HsmElement* child) {
    if (nullptr != child && nullptr != mBodySection) {
        if (child->elementType() == HsmElementType::TRANSITION) {
            if (child->type() == HSM_ELEMENT_TYPE_TRANSITION) {
                HsmTransition* transition = qgraphicsitem_cast<HsmTransition*>(child);

                if (transition) {
                    if (transition->isSelfTransition()) {
                        qDebug() << "add SELF TRANSITION";
                        connect(child, &QObject::destroyed, this, [this, child]() { layoutSections(); });

                        child->setParentItem(mSelfTransitionsSection);
                        layoutSections();
                    } else {
                        qDebug() << "add regular TRANSITION";
                        child->setParentItem(mBodySection);
                    }
                }
            } else {
                qCritical() << Q_FUNC_INFO
                            << "mismatch between HsmElement type is TRANSITION, but it's not an instance of HsmTransition";
            }
        } else {
            child->setParentItem(mBodySection);
        }
    } else {
        qFatal("calling addChildItem on non-initialised HsmStateElement");
    }
}

void HsmStateElement::removeChildItem(HsmElement* child) {
    qDebug() << "HsmStateElement::removeChildItem" << child;
    HsmRectangularElement::removeChildItem(child);
    layoutSections();
}

void HsmStateElement::resizeToFitChildItem(HsmElement* child) {
    // Suppress bodySection movement for the WHOLE duration of a drag-driven resize.
    // resizeElement() updates the model size, which emits modelDataChanged and
    // re-enters onModelDataChanged() -> layoutSections() SYNCHRONOUSLY. That
    // re-entrant relayout (and any cascading resizeParentToFitChildItem) must not
    // move the bodySection while the child is being actively dragged: moving it
    // shifts the dragged child's parent coordinate system between mouse events,
    // which corrupts Qt's delta-based drag tracking and makes the child (and the
    // parent's auto-resize) accelerate / run away.
    // A single scoped flag spanning the whole method keeps every nested relayout on
    // the suppressed path; the body is placed correctly afterwards by
    // normalizeElementRect(). Previously the flag was toggled around individual
    // calls, leaving the signal-driven relayout unguarded.
    const bool childIsDragging = child->isInDragState();
    QScopedValueRollback<bool> suppressGuard(mSuppressBodySectionMovement);
    if (childIsDragging) {
        mSuppressBodySectionMovement = true;
    }

    if (isDirectChild(child) == true) {
        // Get child bounding rectangle in parent coordinates
        QRectF childRect = child->mapRectToParent(child->elementRect());
        // Get parent's current rectangle
        QRectF parentRect = elementRect();
        QRectF parentBodyRect = mBodySection->boundingRect();

        // Calculate required size to fit child
        QRectF parentNewRect = parentRect;
        QRectF parentNewBodyRect = parentBodyRect.united(childRect);

        qreal dw = parentNewBodyRect.width() - parentBodyRect.width();
        qreal dh = parentNewBodyRect.height() - parentBodyRect.height();

        if (parentBodyRect != parentNewBodyRect) {
            qDebug() << "RESIZE_FIT:" << modelId() << "childRect=" << childRect << "bodyRect=" << parentBodyRect
                     << "newBodyRect=" << parentNewBodyRect << "outerRect=" << parentRect << "->" << parentNewRect;
            if (parentBodyRect.left() != parentNewBodyRect.left()) {
                parentNewRect.adjust(-dw - cChildPadding, 0, 0, 0);
            }
            if (parentBodyRect.top() != parentNewBodyRect.top()) {
                parentNewRect.adjust(0, -dh - cChildPadding, 0, 0);
            }
            if (parentBodyRect.right() != parentNewBodyRect.right()) {
                parentNewRect.adjust(0, 0, dw + cChildPadding, 0);
            }
            if (parentBodyRect.bottom() != parentNewBodyRect.bottom()) {
                parentNewRect.adjust(0, 0, 0, dh + cChildPadding);
            }

            resizeElement(parentNewRect);
            resizeParentToFitChildItem();
        }
    }

    layoutSections();
}

void HsmStateElement::normalizeElementRect() {
    // Compensate child positions for the parent pos shift that normalizeElementRect will cause.
    // When bodySection.pos.x tracked outerRect.left() during the drag, no X-compensation is
    // needed (the bodySection reset cancels the parent shift). But when bodySection movement
    // was suppressed (during resizeToFitChildItem for dragged children), bodySection stayed
    // at its original position while outerRect went negative. In that case, we must also
    // compensate children's X positions for the parent pos shift.
    const qreal xShift = mOuterRect.left() - (mBodySection != nullptr ? mBodySection->pos().x() : 0.0);
    const qreal yShift = mOuterRect.top();

    if (mBodySection != nullptr && (std::abs(xShift) > 1e-9 || std::abs(yShift) > 1e-9)) {
        for (QGraphicsItem* child : mBodySection->childItems()) {
            const auto savedFlags = child->flags();
            child->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
            child->setPos(child->pos().x() - xShift, child->pos().y() - yShift);
            child->setFlags(savedFlags);
        }
    }

    mSuppressChildCompensation = true;
    HsmRectangularElement::normalizeElementRect();
    mSuppressChildCompensation = false;

    layoutSections();
}

QPointF HsmStateElement::mapFromSceneToBody(const QPointF& point) const {
    return mBodySection->mapFromScene(point);
}

void HsmStateElement::beginNameEditMode() {
    if (mStateNameLabel != nullptr) {
        mStateNameLabel->beginEditMode();
    }
}

void HsmStateElement::beginNameTypingMode(const QString& newText) {
    if (mStateNameLabel != nullptr) {
        mStateNameLabel->beginTypingMode(newText);
    }
}

QRectF HsmStateElement::bodyBoundingRect() const {
    return (mBodySection != nullptr ? mBodySection->boundingRect() : QRectF());
}

QRectF HsmStateElement::sceneBodyBoundingRect() const {
    return (mBodySection != nullptr ? mBodySection->sceneBoundingRect() : QRectF());
}

void HsmStateElement::onModelDataChanged() {
    qDebug() << "---- HsmStateElement::onModelDataChanged";
    if (isInitialized()) {
        auto entityPtr = modelElement<model::RegularState>();

        if (entityPtr) {
            QSignalBlocker block(mStateNameLabel->document());
            mStateNameLabel->setPlainText(entityPtr->name());
            centerHeader();

            // Update properties section text
            QStringList actions;

            auto appendSlot = [&actions](const QString& label, const model::ModelActionList& slotActions) {
                if (slotActions.size() == 1) {
                    actions << label + ": " + slotActions.first()->serialize();
                } else if (slotActions.size() > 1) {
                    actions << label + ":";
                    for (const auto& action : slotActions) {
                        actions << "- " + action->serialize();
                    }
                }
            };

            appendSlot("onEntry", entityPtr->onEnteringActions());
            if (entityPtr->hasOnStateChangedAction()) {
                actions << "do: " + entityPtr->onStateChangedAction()->serialize();
            }
            appendSlot("onExit", entityPtr->onExitingActions());

            mPropertiesSection->setPlainText(actions.join("\n"));
        }

        layoutSections();
    }
}

void HsmStateElement::onSubstatesChanged(const bool substates) {
    Q_UNUSED(substates);
    update();
}

void HsmStateElement::layoutSections() {
    if (isInitialized()) {
        const QRectF rect = mOuterRect;
        const qreal w = rect.width();
        qreal y = rect.top();

        layoutSelfTransitions();

        // Header section
        // mHeaderSection->setPos(rect.left(), y);
        qreal headerHeight = HEADER_HEIGHT;
        y += headerHeight;
        mHeaderSeparator->setLine(rect.left() + cOuterBorderAdjustment, y, rect.right() - cOuterBorderAdjustment, y);
        y += SECTION_SPACING;

        // Self-Transitions section
        const int transitionsCount = getSelfTransitionsCount();
        qreal selfTransHeight = (transitionsCount > 0 ? (transitionsCount + 1) * HsmTransition::SELFTRANSITION_Y_OFFSET : 0.0);

        mSelfTransitionsSection->setRect(rect.left(), y, w, selfTransHeight);
        y += selfTransHeight;

        y += SECTION_SPACING;
        mSelfTransitionsSeparator->setLine(rect.left() + cOuterBorderAdjustment, y, rect.right() - cOuterBorderAdjustment, y);
        y += SECTION_SPACING;

        // Properties section
        qreal propsHeight = 0.0;

        mPropertiesSection->setPos(rect.left(), y);
        mPropertiesSection->setTextWidth(w);

        if (mPropertiesSection->toPlainText().isEmpty() == false) {
            propsHeight = mPropertiesSection->boundingRect().height();
        }

        y += propsHeight;

        mPropertiesSeparator->setLine(rect.left() + cOuterBorderAdjustment, y, rect.right() - cOuterBorderAdjustment, y);
        y += SECTION_SPACING;

        // Body section: use setPos for x so bodySection->pos().x() tracks outerRect.left()
        const qreal oldBodyX = mBodySection->pos().x();
        const qreal newBodyX = rect.left();

        // Never move the bodySection while a child is being actively dragged. This must
        // be a LIVE check (not only the transient mSuppressBodySectionMovement flag): a
        // single resize emits modelDataChanged several times, each re-entering
        // onModelDataChanged() -> layoutSections() SYNCHRONOUSLY, and some of those
        // re-entries run after the transient flag was rolled back. Moving the body then
        // shifts the dragged child's parent coordinate system between mouse events,
        // corrupting Qt's delta-based drag tracking and causing the parent auto-resize /
        // child to run away.
        const bool suppressBodyMovement = mSuppressBodySectionMovement || isChildBeingDragged();

        if (!suppressBodyMovement) {
            mBodySection->setPos(newBodyX, 0);
            mBodySection->setRect(0, y, w, rect.bottom() - y);

            // Compensate child X positions so their scene positions don't shift
            const qreal deltaX = newBodyX - oldBodyX;

            if (std::abs(deltaX) > 1e-9 && !mSuppressChildCompensation) {
                qDebug() << "COMPENSATE:" << modelId() << "bodyX:" << oldBodyX << "->" << newBodyX << "deltaX=" << deltaX;
                for (QGraphicsItem* child : mBodySection->childItems()) {
                    const QPointF oldChildPos = child->pos();
                    const auto savedFlags = child->flags();
                    child->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
                    child->setPos(child->pos() - QPointF(deltaX, 0));
                    child->setFlags(savedFlags);
                    qDebug() << "  child pos:" << oldChildPos << "->" << child->pos() << "flags_after=" << child->flags();
                }
            }
        } else {
            // During resizeToFitChildItem: keep bodySection at its current X position.
            // Moving it shifts the coordinate system, causing the scene-space snap to
            // produce incorrect results on the next mouse event (the acceleration bug).
            // bodySection will be moved to the correct position by normalizeElementRect.
            // Use rect.left() as the rect's x-origin so the body covers the full expanded area.
            mBodySection->setRect(rect.left(), y, w, rect.bottom() - y);
        }

        // Calculate minimum height based on content
        setMinHeight(headerHeight + SECTION_SPACING + selfTransHeight + SECTION_SPACING + propsHeight + SECTION_SPACING +
                     BODY_MIN_HEIGHT);
    }

    update();
}

void HsmStateElement::layoutSelfTransitions() {
    if (mSelfTransitionsSection) {
        for (auto child : mSelfTransitionsSection->childItems()) {
            if (child->type() == HSM_ELEMENT_TYPE_TRANSITION) {
                HsmTransition* transition = qgraphicsitem_cast<HsmTransition*>(child);

                if (transition) {
                    transition->recalculateLine();
                }
            } else {
                qCritical() << Q_FUNC_INFO << "unexpected child type in self-transitions section, expected TRANSITION, got"
                            << child->type();
            }
        }
    }
}

int HsmStateElement::getSelfTransitionsCount() const {
    return (nullptr != mSelfTransitionsSection ? mSelfTransitionsSection->childItems().size() : 0);
}

bool HsmStateElement::isChildBeingDragged() const {
    if (nullptr == mBodySection) {
        return false;
    }

    for (QGraphicsItem* item : mBodySection->childItems()) {
        if (IS_HSM_ELEMENT_TYPE(item->type())) {
            if (auto* element = qgraphicsitem_cast<HsmElement*>(item)) {
                if (element->isInDragState()) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool HsmStateElement::isInitialized() const {
    return (nullptr != mStateNameLabel && nullptr != mSelfTransitionsSection && nullptr != mPropertiesSection &&
            nullptr != mBodySection && nullptr != mHeaderSeparator && nullptr != mSelfTransitionsSeparator &&
            nullptr != mPropertiesSeparator);
}

void HsmStateElement::updateBoundingRect(const QRectF& newRect) {
    HsmRectangularElement::updateBoundingRect(newRect);
    qDebug() << Q_FUNC_INFO;
    layoutSections();
    centerHeader();
}

void HsmStateElement::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    const auto& theme = ThemeManager::instance().theme();

    if (mHasDuplicateName == true) {
        paintRectangularBody(painter, theme.node.invalidStateBackgroundBrush);
    } else if (hasSubstates() == true) {
        paintRectangularBody(painter, theme.node.substateBackgroundBrush);
    } else {
        paintRectangularBody(painter, theme.node.backgroundBrush);
    }

    // Sections and separators are QGraphicsItems, so no extra drawing needed here
}

};  // namespace view
