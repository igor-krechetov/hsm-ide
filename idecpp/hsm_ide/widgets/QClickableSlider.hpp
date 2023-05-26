#ifndef QCLICKABLESLIDER_HPP
#define QCLICKABLESLIDER_HPP

#include <QSlider>
#include <QStyleOptionSlider>
#include <QStyle>

class QClickableSlider : public QSlider
{
public:
    QClickableSlider(QWidget* parent = nullptr);
    virtual ~QClickableSlider() = default;

protected:
    virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int pixelPosToRangeValue(const QPoint& pos) const;
};

#endif // QCLICKABLESLIDER_HPP
