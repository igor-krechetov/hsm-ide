#include "HsmStateBodySection.hpp"

#include "view/elements/ElementTypeIds.hpp"

#include "HsmElement.hpp"

namespace view {

HsmStateBodySection::HsmStateBodySection(QGraphicsItem* parent)
    : QObject()
    , QGraphicsRectItem(parent) {}

bool HsmStateBodySection::hasSubstates() const {
    return mHasSubstates;
}

QVariant HsmStateBodySection::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (QGraphicsItem::ItemChildAddedChange == change || QGraphicsItem::ItemChildRemovedChange == change) {
        const bool oldState = mHasSubstates;

        mHasSubstates = false;

        for (QGraphicsItem* child : childItems()) {
            if ( IS_HSM_ELEMENT_TYPE(child->type()) ) {
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
