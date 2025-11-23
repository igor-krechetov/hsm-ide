#include "HsmElementsListDelegate.hpp"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

HsmElementsListDelegate::HsmElementsListDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void HsmElementsListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QRect rect = opt.rect;
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    // Draw background (including selection highlight)
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, opt.palette.color(QPalette::Highlight).lighter(150));
    }

    // Retrieve icon and text
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    const QString text = index.data(Qt::DisplayRole).toString();

    // Calculate icon rect (centered horizontally)
    const int iconX = rect.x() + (rect.width() - cIconSize) / 2;
    const int iconY = rect.y() + cSpacing;

    QRect iconRect(iconX, iconY, cIconSize, cIconSize);

    // Draw the icon
    icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal);

    // Text rect (same horizontal center, wrapped)
    const int textY = iconRect.bottom() + cSpacing;
    QRect textRect(rect.x() + (rect.width() - cTextWidth) / 2, textY, cTextWidth, rect.height());

    // Draw the text
    painter->setPen(opt.palette.color(QPalette::Text));
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, text);

    painter->restore();
}

QSize HsmElementsListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option)

    const QString text = index.data(Qt::DisplayRole).toString();
    QFontMetrics fm(option.font);

    // compute wrapped text height
    QRect wrapped = fm.boundingRect(QRect(0, 0, cTextWidth, 500), Qt::TextWordWrap, text);

    int h = cIconSize + cSpacing + wrapped.height() + cSpacing * 2;
    int w = std::max(cIconSize, cTextWidth) + cSpacing * 2;

    return QSize(w, h);
}
