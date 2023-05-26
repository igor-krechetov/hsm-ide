#include "QClickableSlider.hpp"

QClickableSlider::QClickableSlider(QWidget* parent)
    : QSlider(parent)
{
}

void QClickableSlider::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        int val = pixelPosToRangeValue(event->pos());
        setValue(val);
    }
    else
    {
        QSlider::mousePressEvent(event);
    }
}

int QClickableSlider::pixelPosToRangeValue(const QPoint& pos) const
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    int sliderLength, sliderMin, sliderMax;
    if (orientation() == Qt::Horizontal)
    {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    }
    else
    {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }

    QPoint pr = pos - sr.center() + sr.topLeft();
    int p = orientation() == Qt::Horizontal ? pr.x() : pr.y();

    return QStyle::sliderValueFromPosition(minimum(), maximum(), p - sliderMin, sliderMax - sliderMin, opt.upsideDown);
}
