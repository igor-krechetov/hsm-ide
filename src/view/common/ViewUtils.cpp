#include "ViewUtils.hpp"

#include <QGraphicsScene>
#include <QPointF>

#include "view/elements/private/HsmElement.hpp"

namespace view {

HsmElement* ViewUtils::topHsmElementAt(QGraphicsScene* scene,
                                       const QPointF& scenePos,
                                       const bool onlyConnectable,
                                       const bool onlyAcceptsConnections,
                                       const bool onlyAcceptsChildren,
                                       const HsmElement* ignoreElement) {
    HsmElement* element = nullptr;

    if (scene != nullptr) {
        QList<QGraphicsItem*> targetItems = scene->items(scenePos);

        // TODO: account for subitems
        for (auto targetItem : targetItems) {
            if (nullptr != targetItem) {
                QVariant elementType = targetItem->data(USERDATA_HSM_ELEMENT_TYPE);

                if (elementType.isValid() && elementType.toInt() != static_cast<int>(view::HsmElementType::UNKNOWN)) {
                    // NOTE: because QGraphicsObject is a child to both QObject and QGraphicsItem it's crucial
                    //       to use dynamic_cast. parentItem() != parentObject()
                    element = dynamic_cast<HsmElement*>(targetItem);

                    if (nullptr != element) {
                        if (ignoreElement == element) {
                            element = nullptr;
                        } else if (onlyConnectable && element->isConnectable() == false) {
                            qDebug() << "Target is not connectable: " << element->modelId()
                                     << " | viewElementType=" << elementType << " | " << element;
                            element = nullptr;
                        } else if (onlyAcceptsConnections && element->acceptsConnections() == false) {
                            qDebug() << "Target does not accept connections: " << element->modelId()
                                     << " | viewElementType=" << elementType << " | " << element;
                            element = nullptr;
                        } else if (onlyAcceptsChildren && element->acceptsChildren() == false) {
                            qDebug() << "Target doesn't accept children: " << element->modelId()
                                     << " | viewElementType=" << elementType << " | " << element;
                            element = nullptr;
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