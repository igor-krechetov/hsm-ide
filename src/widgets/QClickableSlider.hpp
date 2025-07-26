#ifndef QCLICKABLESLIDER_HPP
#define QCLICKABLESLIDER_HPP

#include <QSlider>

class QClickableSlider : public QSlider
{
public:
    QClickableSlider(QWidget* parent = nullptr);
    virtual ~QClickableSlider() = default;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int pixelPosToRangeValue(const QPoint& pos) const;
};

#endif // QCLICKABLESLIDER_HPP
