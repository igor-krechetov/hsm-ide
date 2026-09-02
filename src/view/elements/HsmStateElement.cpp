#include "HsmStateElement.hpp"

#include <QColor>
#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
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

        mStateNameLabel->setDefaultTextColor(theme.node.textColor);
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

            // Suppress bodySection movement only when the triggering child is being
            // actively dragged. This prevents the acceleration bug where bodySection
            // shift corrupts the scene-space snap on subsequent mouse events.
            // For cascading resizes (parent expanding within grandparent), bodySection
            // movement is allowed since it doesn't affect the dragged element's mapping.
            const bool childIsDragging = child->isInDragState();
            if (childIsDragging) {
                mSuppressBodySectionMovement = true;
            }
            resizeElement(parentNewRect);
            resizeParentToFitChildItem();
            mSuppressBodySectionMovement = false;
        }
    }

    if (child->isInDragState()) {
        mSuppressBodySectionMovement = true;
    }
    layoutSections();
    mSuppressBodySectionMovement = false;
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

            if (entityPtr->hasOnEnteringAction()) {
                actions << "onEntry: " + entityPtr->onEnteringAction()->serialize();
            }
            if (entityPtr->hasOnStateChangedAction()) {
                actions << "do: " + entityPtr->onStateChangedAction()->serialize();
            }
            if (entityPtr->hasOnExitingAction()) {
                actions << "onExit: " + entityPtr->onExitingAction()->serialize();
            }

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

        if (!mSuppressBodySectionMovement) {
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

    if (hasSubstates() == true) {
        paintRectangularBody(painter, theme.node.substateBackgroundBrush);
    } else {
        paintRectangularBody(painter, theme.node.backgroundBrush);
    }

    // Sections and separators are QGraphicsItems, so no extra drawing needed here
}

};  // namespace view
