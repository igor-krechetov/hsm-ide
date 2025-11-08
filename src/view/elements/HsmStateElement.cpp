#include "HsmStateElement.hpp"

#include <QColor>
#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QSignalBlocker>
#include <QTextDocument>

#include "model/RegularState.hpp"
#include "private/HsmStateTextItem.hpp"
#include "HsmTransition.hpp"

namespace view {

HsmStateElement::HsmStateElement()
    : HsmRectangularElement(HsmElementType::STATE) {}

HsmStateElement::HsmStateElement(const QSizeF& size)
    : HsmRectangularElement(HsmElementType::STATE, size) {}

void HsmStateElement::init(const QSharedPointer<model::StateMachineEntity>& modelEntity) {
    HsmRectangularElement::init(modelEntity);

    // Create header section
    mStateNameLabel = new HsmStateTextItem(this);  // Make editable label a direct child
    // Other sections
    mSelfTransitionsSection = new QGraphicsRectItem(this);
    mPropertiesSection = new QGraphicsTextItem("Properties", this);
    mBodySection = new QGraphicsRectItem(this);
    // Separator lines
    mHeaderSeparator = new QGraphicsLineItem(this);
    mSelfTransitionsSeparator = new QGraphicsLineItem(this);
    mPropertiesSeparator = new QGraphicsLineItem(this);

    connect(mStateNameLabel->document(), &QTextDocument::contentsChanged, this, &HsmStateElement::onStateNameChanged);
    connect(mStateNameLabel, &HsmStateTextItem::editingFinished, this, &HsmStateElement::onStateNameEditFinished);

    mSelfTransitionsSection->setPen(Qt::NoPen);
    mBodySection->setPen(Qt::NoPen);

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

bool HsmStateElement::acceptsChildren() const {
    return true;
}

QList<QGraphicsItem*> HsmStateElement::hsmChildItems() const {
    return (nullptr != mBodySection ? mBodySection->childItems() : QList<QGraphicsItem*>());
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
            HsmTransition* transition = qgraphicsitem_cast<HsmTransition*>(child);

            if (transition) {
                if (transition->isSelfTransition()) {
                    qDebug() << "add SELF TRANSITION";
                    connect(child, &QObject::destroyed, this, [this, child]() {
                        layoutSections();
                    });

                    child->setParentItem(mSelfTransitionsSection);
                    layoutSections();
                } else {
                    qDebug() << "add regular TRANSITION";
                    child->setParentItem(mBodySection);
                }
            } else {
                qCritical() << Q_FUNC_INFO << "mismatch between HsmElement type is TRANSIOTION, but it's not an instance of HsmTransition";
            }
        } else {
            child->setParentItem(mBodySection);
        }
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

            QGraphicsRectItem* body = qgraphicsitem_cast<QGraphicsRectItem*>(mBodySection);

            resizeElement(parentNewRect);
            resizeParentToFitChildItem();
        }
    }

    layoutSections();
}

void HsmStateElement::normalizeElementRect() {
    HsmRectangularElement::normalizeElementRect();

    layoutSections();
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

            if (!entityPtr->onEnteringCallback().isEmpty()) {
                actions << "onEntry: " + entityPtr->onEnteringCallback();
            }
            if (!entityPtr->onStateChangedCallback().isEmpty()) {
                actions << "do: " + entityPtr->onStateChangedCallback();
            }
            if (!entityPtr->onExitingCallback().isEmpty()) {
                actions << "onExit: " + entityPtr->onExitingCallback();
            }

            mPropertiesSection->setPlainText(actions.join("\n"));
        }

        layoutSections();
    }
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

        qDebug() << "------------- mSelfTransitionsSection->setRect";
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

        // Body section
        qDebug() << "------------- mBodySection->setRect";
        mBodySection->setRect(rect.left(), y, w, rect.bottom() - y);

        // Calculate minimum height based on content
        setMinHeight(headerHeight + SECTION_SPACING + selfTransHeight + SECTION_SPACING + propsHeight + SECTION_SPACING +
                     BODY_MIN_HEIGHT);
    }

    update();
}

void HsmStateElement::layoutSelfTransitions() {
    if (mSelfTransitionsSection) {
        for (auto child: mSelfTransitionsSection->childItems()) {
            HsmTransition* transition = qgraphicsitem_cast<HsmTransition*>(child);

            if (transition) {
                transition->recalculateLine();
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
    HsmRectangularElement::paint(painter, option, widget);
    // Sections and separators are QGraphicsItems, so no extra drawing needed here
}

};  // namespace view