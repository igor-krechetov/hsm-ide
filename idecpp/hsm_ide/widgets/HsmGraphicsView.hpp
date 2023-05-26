#ifndef HSMGRAPHICSVIEW_HPP
#define HSMGRAPHICSVIEW_HPP

#include <QGraphicsView>

class HsmGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    HsmGraphicsView(QWidget *parent = nullptr);
};

#endif // HSMGRAPHICSVIEW_HPP
