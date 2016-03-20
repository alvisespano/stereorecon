#include "stereographicsview.h"
#include "mainwindow.h"
#include "config.h"
#include "prelude.h"
#include "globals.h"

//
//
//
// custom graphics view implementation

void StereoGraphicsView::wheelEvent(QWheelEvent* ev)
{
    using prelude::crop;

    const qreal deg = qreal(ev->delta()) / 8.0,
                s = crop(pow(2.0, deg / 240.0), config::ui::min_view_scale, config::ui::max_view_scale);
    scale(s, s);
}

//
//
//
// match graphics item implementation

void MatchGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    mw->delegate_as_event(&main_window::match_item_hover_enter, this);
}

void MatchGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    mw->delegate_as_event(&main_window::match_item_hover_leave, this);
}

//
//
//
// custom scene implementation

/*void StereoGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    const auto&& ep = ev->pos();
    auto* item = itemAt(ep);
    globals::logger->debug("mouse pressed at (%g, %g)", ep.x(), ep.y());
    if (item != NULL)
    {
        const auto& ip = item->pos();
        switch (item->type())
        {
            case QGraphicsEllipseItem::Type:
                globals::logger->msg("feature at (%d, %d) clicked", ip.x(), ip.y());
                break;

            case QGraphicsPixmapItem::Type:
                globals::logger->msg("background image clicked at (%g, %g)", ip.x(), ip.y());
                item->setSelected(true);
                break;

            default:
                globals::logger->msg("unknown area clicked");
        }
    }
}*/
