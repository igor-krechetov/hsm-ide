#ifndef HSMELEMENTSLIST_HPP
#define HSMELEMENTSLIST_HPP

#include <QListWidget>

class HsmElementsList : public QListWidget
{
    Q_OBJECT
public:
    HsmElementsList(QWidget *parent = nullptr);
};

#endif // HSMELEMENTSLIST_HPP
