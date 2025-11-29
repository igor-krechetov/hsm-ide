#ifndef NONEMPTYDIRFILESYSTEMPROXYMODEL_HPP
#define NONEMPTYDIRFILESYSTEMPROXYMODEL_HPP

#include <QSortFilterProxyModel>
#include <QFileSystemModel>

namespace view {

class NonEmptyDirFileSystemProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit NonEmptyDirFileSystemProxyModel(QObject* parent = nullptr);

    QString getFilePath(const QModelIndex& proxyIndex) const;

protected:
    // Override filterAcceptsRow to hide empty directories
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
};

} // namespace view

#endif // NONEMPTYDIRFILESYSTEMPROXYMODEL_HPP
