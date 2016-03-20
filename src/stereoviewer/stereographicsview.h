#ifndef STEREOGRAPHICSVIEW_H
#define STEREOGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneHoverEvent>
#include "stereoview.h"

class StereoGraphicsView : public QGraphicsView
{
protected:
    void wheelEvent(QWheelEvent* ev);

public:
    StereoGraphicsView(QWidget* w) : QGraphicsView(w)
    {
        this->setInteractive(true);
    }
};

class main_window;  // forward declaration

class MatchGraphicsItem : public QGraphicsEllipseItem
{
private:
    main_window* const mw;

public:
    const stereoview::feature_id_type feature_id;
    const QPen original_pen;

    MatchGraphicsItem(main_window* _mw, const stereoview::feature_id_type& feat, qreal x, qreal y, qreal w, qreal h, const QPen& pen, QGraphicsItem* parent = NULL)
        : QGraphicsEllipseItem(x, y, w, h, parent), mw(_mw), feature_id(feat), original_pen(pen)
    {
        this->setPen(original_pen);
        this->setAcceptHoverEvents(true);
    }

    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* ev);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* ev);
};

class StereoGraphicsScene : public QGraphicsScene
{
public:    
    //void mousePressEvent(QGraphicsSceneMouseEvent* ev);
};

#endif // STEREOGRAPHICSVIEW_H
