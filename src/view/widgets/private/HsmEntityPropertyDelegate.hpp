#ifndef HSMENTITYPROPERTYDELEGATE_HPP
#define HSMENTITYPROPERTYDELEGATE_HPP

#include <QModelIndex>
#include <QRect>
#include <QStyledItemDelegate>

namespace view {

class HsmEntityPropertyDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit HsmEntityPropertyDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

signals:
    void finishEditing(QWidget* editor) const;

private slots:
    void onEditingFinished(QWidget* editor);

private:
    static QRect removeButtonRect(const QStyleOptionViewItem& option);
    static QRect moveUpButtonRect(const QStyleOptionViewItem& option);
    static QRect moveDownButtonRect(const QStyleOptionViewItem& option);
    static QRect addButtonRect(const QStyleOptionViewItem& option);

    void drawButton(QPainter* painter,
                    const QStyleOptionViewItem& option,
                    const QRect& rect,
                    const QString& glyph,
                    const bool hovered) const;

private:
    // Tracks which sub-button (by row + screen rect) the pointer is over, for hover highlight.
    mutable QModelIndex mHoverIndex;
    mutable QRect mHoverButtonRect;
};

};  // namespace view

#endif  // HSMENTITYPROPERTYDELEGATE_HPP