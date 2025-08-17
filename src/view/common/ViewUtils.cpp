#include "ViewUtils.hpp"

#include <QGraphicsScene>
#include <QPointF>

#include "view/elements/private/HsmElement.hpp"

namespace view {

HsmElement* ViewUtils::topHsmElementAt(QGraphicsScene* scene,
                                       const QPointF& pos,
                                       const bool onlyConnectable,
                                       const bool onlyAcceptsChildren) {
    HsmElement* element = nullptr;

    if (scene != nullptr) {
        QList<QGraphicsItem*> targetItems = scene->items(pos);

        // TODO: account for subitems
        for (auto targetItem : targetItems) {
            if (nullptr != targetItem) {
                QVariant elementType = targetItem->data(USERDATA_HSM_ELEMENT_TYPE);

                if (elementType.isValid() && elementType.toInt() != static_cast<int>(view::HsmElementType::UNKNOWN)) {
                    // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
                    //       to use dynamic_cast. parentItem() != parentObject()
                    element = dynamic_cast<HsmElement*>(targetItem);

                    if (nullptr != element) {
                        if (onlyConnectable) {
                            if (element->isConnectable() == false) {
                                qDebug() << "Target is not connectable: " << element->modelId()
                                         << " | viewElementType=" << elementType << " | " << element;
                                element = nullptr;
                            }
                        }
                        if (onlyAcceptsChildren) {
                            if (element->acceptsChildren() == false) {
                                qDebug() << "Target doesn't accept children: " << element->modelId()
                                         << " | viewElementType=" << elementType << " | " << element;
                                element = nullptr;
                            }
                        }

                        if (nullptr != element) {
                            break;
                        }
                    }
                }
            }
        }
    }

    return element;
}

};  // namespace view