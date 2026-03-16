#ifndef THEME_HPP
#define THEME_HPP

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPen>
#include <Qt>

namespace view {
namespace theme {

struct NodeStyle {
    QPen borderPen;
    QPen highlightBorderPen;
    QPen selectedBorderPen;
    QBrush backgroundBrush;
    QBrush substateBackgroundBrush;
    QBrush includeBackgroundBrush;
    QBrush mainBrush;
    QColor textColor;
    QBrush editingBackgroundBrush;
    QFont labelFont;
    QFont historyFont;
    qreal cornerRadius = 5.0;
    qreal finalInnerRadiusFactor = 0.7;
};

struct TransitionStyle {
    QPen pen;
    QPen selectedPen;
    QPen debugSelectionPen;
};

struct GridStyle {
    QPen majorLinePen;
    QPen minorLinePen;
};

struct GripStyle {
    QColor color;
    QColor hoverColor;
    QPen debugPen;
};

struct ConnectionArrowStyle {
    QBrush validBrush;
    QBrush invalidBrush;
};

struct Theme {
    NodeStyle node;
    TransitionStyle transition;
    GridStyle grid;
    GripStyle grip;
    ConnectionArrowStyle connectionArrow;
};

}  // namespace theme
}  // namespace view

#endif  // THEME_HPP
