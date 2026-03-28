#ifndef FILTEREDFILESYSTEMPROXYMODEL_HPP
#define FILTEREDFILESYSTEMPROXYMODEL_HPP

#include <QFileSystemModel>
#include <QSortFilterProxyModel>

namespace view {

class FilteredFileSystemProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit FilteredFileSystemProxyModel(QObject* parent = nullptr);

    void setShowEmptyFolders(bool enabled);
    bool isShowEmptyFolders() const;

    QString getFilePath(const QModelIndex& proxyIndex) const;
    bool isDir(const QModelIndex& proxyIndex) const;

private:
    bool hasScxmlRecursively(const QModelIndex& sourceIndex) const;
    bool isScxmlFile(const QString& filePath) const;
    bool mShowEmptyFolders = false;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
};

}  // namespace view

#endif  // FILTEREDFILESYSTEMPROXYMODEL_HPP
