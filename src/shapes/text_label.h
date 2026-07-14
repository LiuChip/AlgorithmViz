#ifndef TEXT_LABEL_H
#define TEXT_LABEL_H

#include "shape.h"

class TextLabel : public Shape
{
public:
    TextLabel(qreal x, qreal y, const QString &text);
    TextLabel(QPointF point, const QString &text);
    Shape *clone() const override;

    void setText(const QString &text);
    QString getText() const;

    void setTextInfo(TextStyle textStyle) override;
    void setTextInfo() override;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QSizeF calculateTextSize() const;
    void refreshTextGeometry();
};

#endif // TEXT_LABEL_H
