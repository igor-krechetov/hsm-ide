#ifndef HSMINCLUDEELEMENT_HPP
#define HSMINCLUDEELEMENT_HPP

#include <QSize>

#include "HsmStateElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmStateTextItem;

class HsmIncludeElement : public HsmStateElement {
    Q_OBJECT

public:
    HsmIncludeElement();
    explicit HsmIncludeElement(const QSizeF& size);
    virtual ~HsmIncludeElement() = default;

    void init(const QSharedPointer<model::StateMachineEntity>& modelEntity) override;
    bool acceptsChildren() const override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

};  // namespace view

#endif  // HSMINCLUDEELEMENT_HPP
