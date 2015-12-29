#ifndef _GRAPH_EDGE_H
#define _GRAPH_EDGE_H

#include <ogdf/basic/geometry.h>
#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsProxyWidget>
#include "GraphProcessor.h"


QList<QLineF> getLinesFromRect(QRectF& rect);
QList<QLineF> getLinesFromPainterPath(QPainterPath& painterPath);
QList<QPointF> findIntersectionPoints(QRectF& node, QPainterPath& edge);

class GraphEdge : public QGraphicsItem
{
public:
    enum EDGE_TYPE
    {
        EDGE_RIGHT,
        EDGE_LEFT
    };

    enum SEARCH_DIRECTION
    {
        TO_RIGHT,
        TO_LEFT
    };

    GraphEdge(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect, EDGE_TYPE edgeType, float minNodeDistance, std::vector<QGraphicsProxyWidget*>* graphNodeProxies, std::vector<GraphEdge*>* graphNodeItems);
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    qreal calculateDistance(QPointF p1, QPointF p2);
    QPointF calculateNearestIntersect(QRectF rect, QPointF p1, QPointF p2);
    QList<QPointF> calculateLine(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect);
    QList<QPointF> calculateArrow(const QList<QPointF> & linePoints);
    QRectF calculateBoundingRect(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints);
    void preparePainterPaths(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints);
    void setEdgeColor(QBrush edgeColor);
    GraphEdge::EDGE_TYPE getEdgeType();
    QPainterPath shape() const override;

private:
    QList<QPointF> calculateBendPointsAndAdd(QPointF start, QPointF end, const ogdf::DPolyline& bends);
    void fixOverlappingArrows(QPointF& start, QPointF& end, QList<QPointF>& linePoints,  const ogdf::DPoint& ogdfBend,  std::vector<QPointF*>& bendPoints);
    int getOffsetToAvoidOverlappingEdges(QPointF p1, QPointF p2, SEARCH_DIRECTION searchDirection);
    bool isEdgeCollidingWithNodes(const QPolygonF& edgePolygon) const;
    QPolygonF polygonFromEdge(const QPointF& start, const QPointF& end, const ogdf::DPolyline& bends) const;

private:
    QPainterPath mLine;
    QPainterPath mArrow;
    QPointF mBendPoint;
    QRectF mBoundingRect;
    QBrush mEdgeColor;
    EDGE_TYPE mEdgeType;
    float mMinNodeDistance;
    ogdf::DPolyline mBends;
    const qreal mArrowLen = 7;
    std::vector<GraphEdge*>* mGraphEdgeItems;
    std::vector<QGraphicsProxyWidget*>* mGraphNodeProxies;
};

#endif //_GRAPH_EDGE_H
