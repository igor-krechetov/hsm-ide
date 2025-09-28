#ifndef HSMSTATEELEMENT_HPP
#define HSMSTATEELEMENT_HPP

#include "private/styles/HsmRectangularElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmStateElement : public HsmRectangularElement {
    Q_OBJECT

public:
    HsmStateElement();
    virtual ~HsmStateElement() = default;

    virtual bool acceptsChildren() const;

protected:
    void updateBoundingRect(const QRectF& newRect = QRectF()) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void centerHeader();

protected slots:
    void onTextChanged();

private:
    QGraphicsTextItem* mTextItem = nullptr;
};

};  // namespace view

#endif  // HSMSTATEELEMENT_HPP
