#ifndef VIEWUTILS_HPP
#define VIEWUTILS_HPP

#include "view/elements/private/HsmElement.hpp"

class QGraphicsScene;
class QPointF;

namespace view {

class ViewUtils {
public:
    // Need to specify childType if onlyAcceptsChildren is true
    static HsmElement* topHsmElementAt(QGraphicsScene* scene,
                                       const QPointF& scenePos,
                                       const bool onlyConnectable,
                                       const bool onlyAcceptsConnections,
                                       const bool onlyAcceptsChildren,
                                       const bool ignoreSelected,
                                       const HsmElement* ignoreElement = nullptr,
                                       const HsmElementType childType = HsmElementType::UNKNOWN);
};

};  // namespace view

#endif  // VIEWUTILS_HPP