#include "QClickableSlider.hpp"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

QClickableSlider::QClickableSlider(QWidget* parent)
    : QSlider(parent) {}

void QClickableSlider::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setValue(pixelPosToRangeValue(event->pos()));
    } else {
        QSlider::mousePressEvent(event);
    }
}

int QClickableSlider::pixelPosToRangeValue(const QPoint& pos) const {
    int sliderLength = 0;
    int sliderMin = 0;
    int sliderMax = 0;
    QStyleOptionSlider opt;

    initStyleOption(&opt);

    const QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    const QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    if (orientation() == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }

    const QPoint pr = pos - sr.center() + sr.topLeft();
    const int p = (orientation() == Qt::Horizontal ? pr.x() : pr.y());

    return QStyle::sliderValueFromPosition(minimum(), maximum(), p - sliderMin, sliderMax - sliderMin, opt.upsideDown);
}
