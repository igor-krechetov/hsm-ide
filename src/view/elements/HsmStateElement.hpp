#ifndef HSMSTATEELEMENT_HPP
#define HSMSTATEELEMENT_HPP

#include <QSize>

#include "private/styles/HsmRectangularElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmStateTextItem;
class HsmStateBodySection;

class HsmStateElement : public HsmRectangularElement {
    Q_OBJECT

public:
    HsmStateElement();
    explicit HsmStateElement(const QSizeF& size);
    virtual ~HsmStateElement() = default;

    void init(const QSharedPointer<model::StateMachineEntity>& modelEntity) override;
    bool acceptsChildElement(const HsmElementType type) const override;
    bool canBeTopLevel() const override;
    
    virtual QList<QGraphicsItem*> hsmChildItems() const;
    bool hasSubstates() const override;
    // bool isDirectChild(HsmElement* item) const override;
    // QRectF childrenRect() const override;
    void addChildItem(HsmElement* child) override;
    void removeChildItem(HsmElement* child) override;

    void resizeToFitChildItem(HsmElement* child) override;
    void normalizeElementRect() override;

    QPointF mapFromSceneToBody(const QPointF& point) const override;

protected:
    bool isInitialized() const;
    void updateBoundingRect(const QRectF& newRect = QRectF()) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void centerHeader();
    void layoutSections();  // New: lays out all sections vertically

    void layoutSelfTransitions();

    int getSelfTransitionsCount() const;

protected slots:
    void onStateNameChanged();
    void onStateNameEditFinished();
    void onModelDataChanged() override;
    void onSubstatesChanged(const bool substates);

private:
    // Layout constants
    static constexpr qreal HEADER_HEIGHT = 40.0;
    static constexpr qreal SECTION_SPACING = 0.0;
    static constexpr qreal BODY_MIN_HEIGHT = 40.0;

    // Section items
    HsmStateTextItem* mStateNameLabel = nullptr;
    QGraphicsRectItem* mSelfTransitionsSection = nullptr;
    QGraphicsTextItem* mPropertiesSection = nullptr;
    HsmStateBodySection* mBodySection = nullptr;
    // Separator lines
    QGraphicsLineItem* mHeaderSeparator = nullptr;
    QGraphicsLineItem* mSelfTransitionsSeparator = nullptr;
    QGraphicsLineItem* mPropertiesSeparator = nullptr;
};

};  // namespace view

#endif  // HSMSTATEELEMENT_HPP
