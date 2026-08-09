#include "HsmStateBodySection.hpp"

#ifdef DEBUG_RENDERING
  #include <QPainter>
#endif  // DEBUG_RENDERING

#include "HsmElement.hpp"
#include "view/elements/ElementTypeIds.hpp"

namespace view {

HsmStateBodySection::HsmStateBodySection(QGraphicsItem* parent)
    : QObject()
    , QGraphicsRectItem(parent) {}

bool HsmStateBodySection::hasSubstates() const {
    return mHasSubstates;
}

#ifdef DEBUG_RENDERING
void HsmStateBodySection::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    QGraphicsRectItem::paint(painter, option, widget);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 0, 0, 50));
    painter->drawRect(rect());
}
#endif  // DEBUG_RENDERING

QVariant HsmStateBodySection::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemChildAddedChange == change || QGraphicsItem::ItemChildRemovedChange == change) {
        const bool oldState = mHasSubstates;

        mHasSubstates = false;

        for (QGraphicsItem* child : childItems()) {
            if (IS_HSM_ELEMENT_TYPE(child->type())) {
                HsmElement* element = qgraphicsitem_cast<HsmElement*>(child);

                if ((nullptr != element) && (element->elementType() != HsmElementType::TRANSITION)) {
                    mHasSubstates = true;
                    break;
                }
            }
        }

        if (oldState != mHasSubstates) {
            emit substatesChanged(mHasSubstates);
        }
    }

    return QGraphicsRectItem::itemChange(change, value);
}

int HsmStateBodySection::type() const {
    return view::ELEMENT_TYPE_STATE_BODY_SECTION;
}

}  // namespace view
