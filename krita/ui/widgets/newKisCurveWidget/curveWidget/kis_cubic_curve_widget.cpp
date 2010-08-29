#include "kis_cubic_curve_widget.h"

#include <cmath>
#include <QVector2D>
#include <QPainter>
#include <QPainterPath>

KisCubicCurveWidget::KisCubicCurveWidget(QWidget *parent) :
    KisCurveWidgetBase(parent)
{
}

void KisCubicCurveWidget::paintEvent(QPaintEvent *e)
{
    const qreal CONTROL_POINT_FACTOR=0.4;
    KisCurveWidgetBase::paintEvent(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setMatrix(m_converterMatrix);
    QPainterPath path;
    path.moveTo(m_points.first());

    QList<QPair<QVector2D, QVector2D> > controlPoints;

    for(int i=1; i<m_points.size()-1; i++) {
        QVector2D last(m_points.at((i-1)>=0?(i-1):0));
        QVector2D current(m_points.at(i));
        QVector2D next(m_points.at((i+1)<m_points.size()?(i+1):i));

        QVector2D tangent = next-last;
        tangent.normalize();

        QVector2D ctrlPt1(current - (current.x()-last.x())*CONTROL_POINT_FACTOR*tangent);
        QVector2D ctrlPt2(current + (next.x()-current.x())*CONTROL_POINT_FACTOR *tangent);

        controlPoints.append(QPair<QVector2D, QVector2D>(ctrlPt1, ctrlPt2));
    }

    if(m_points.size()>=3) {
        // compute control point for first line=======================================
        QVector2D first(m_points.first());
        QVector2D second(m_points.at(1));
        QVector2D third(m_points.at(2));

        QVector2D tangent = third-first;
        tangent.normalize();

        QVector2D direction = second-first;
        direction.normalize();

        qreal tau = atan2(tangent.y(), tangent.x());
        qreal delta = atan2(direction.y(), direction.x());

        qreal newAngle = delta+delta-tau;
        if(newAngle>M_PI/2.) newAngle=M_PI/2.;
        if(newAngle<0) newAngle=0;


        QVector2D controlPoint;
        controlPoint.setX(cos(newAngle));
        controlPoint.setY(sin(newAngle));
        controlPoint*=m_points.at(1).x()*CONTROL_POINT_FACTOR;

        controlPoints.prepend(controlPoints.first());
        controlPoints.first().second=controlPoint;

        // compute control point for last line=======================================
        QVector2D last(m_points.last());
        QVector2D secondToLast(m_points.at(m_points.size()-2));
        QVector2D thirdToLast(m_points.at(m_points.size()-3));

        /*QVector2D */tangent = thirdToLast-last;
        tangent.normalize();

        /*QVector2D */direction = secondToLast-last;
        direction.normalize();

        /*qreal */tau = atan2(tangent.y(), tangent.x());
        /*qreal */delta = atan2(direction.y(), direction.x());

        /*qreal */newAngle = delta+delta-tau;
        if(newAngle<M_PI*-1.) newAngle=M_PI*-1.;
        if(newAngle>M_PI/-2.) newAngle=M_PI/-2.;


//        QVector2D controlPoint;
        controlPoint.setX(cos(newAngle));
        controlPoint.setY(sin(newAngle));
        controlPoint*=(last.x()-secondToLast.x())*CONTROL_POINT_FACTOR;
        controlPoint+=last;

        controlPoints.append(controlPoints.last());
        controlPoints.last().first=controlPoint;
    }
    else {
        controlPoints.append(QPair<QVector2D, QVector2D>(QVector2D(m_points.first()), QVector2D(m_points.first())));
        controlPoints.append(QPair<QVector2D, QVector2D>(QVector2D(m_points.last()), QVector2D(m_points.last())));
    }


    for(int i=1; i<m_points.size(); i++) {
        path.cubicTo(controlPoints.at(i-1).second.toPointF(), controlPoints.at(i).first.toPointF(), m_points.at(i));
    }

    painter.drawPath(path);

    paintBlips(&painter);

    // debug output:
//    for(int i=0; i<controlPoints.size(); i++) {
//        painter.drawEllipse(controlPoints.at(i).first.toPoint(), 2, 2);
//        painter.drawText(controlPoints.at(i).first.toPoint(), QString::number(i));
//        painter.drawEllipse(controlPoints.at(i).second.toPoint(), 3, 3);
//        painter.drawEllipse(controlPoints.at(i).second.toPoint(), 1, 1);
//        painter.drawText(controlPoints.at(i).second.toPoint(), QString::number(i));
//    }
}
