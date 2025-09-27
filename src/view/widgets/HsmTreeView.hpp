#ifndef HSMTREEVIEW_HPP
#define HSMTREEVIEW_HPP

#include <QSet>
#include <QTreeView>

class HsmTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit HsmTreeView(QWidget* parent = nullptr);
    virtual ~HsmTreeView() = default;

    void setModel(QAbstractItemModel* model) override;

protected slots:
    void onModelReset();
    void onModelAboutToBeReset();
    void onModelRowsInserted(const QModelIndex& parent, int first, int last);

private:
    void saveExpandedState();
    void restoreExpandedState();
    QString getIndexPath(const QModelIndex& index) const;
    QModelIndex findIndexByPath(const QString& path) const;

private:
    QSet<QString> mExpandedPaths;
    bool mRestoringState = false;
};

#endif  // HSMTREEVIEW_HPP
