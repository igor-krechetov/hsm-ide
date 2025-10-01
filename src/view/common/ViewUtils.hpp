#ifndef VIEWUTILS_HPP
#define VIEWUTILS_HPP

class QGraphicsScene;
class QPointF;

namespace view {

class HsmElement;

class ViewUtils {
public:
    static HsmElement* topHsmElementAt(QGraphicsScene* scene,
                                       const QPointF& scenePos,
                                       const bool onlyConnectable,
                                       const bool onlyAcceptsConnections,
                                       const bool onlyAcceptsChildren,
                                       const HsmElement* ignoreElement);
};

};  // namespace view

#endif  // VIEWUTILS_HPP