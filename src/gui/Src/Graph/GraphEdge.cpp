#include "GraphEdge.h"
#include "GraphNode.h"

using namespace std;

GraphEdge::GraphEdge(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect, EDGE_TYPE edgeType, float minNodeDistance, std::vector<GraphNode*>* graphNodeItems, std::vector<GraphEdge*>* graphEdgeItems)
    :
    QGraphicsItem(),
    mEdgeType(edgeType),
    mMinNodeDistance(minNodeDistance),
    mBends(bends),
    mGraphNodeItems(graphNodeItems),
    mGraphEdgeItems(graphEdgeItems)
{

    if(mEdgeType == EDGE_LEFT)
        mEdgeColor = Qt::red;
    else
        mEdgeColor = Qt::green;

    QList<QPointF> linePoints = calculateLine(start, end, bends, sourceRect, targetRect);
    QList<QPointF> arrowPoints = calculateArrow(linePoints);

    mBoundingRect = calculateBoundingRect(linePoints, arrowPoints);
    preparePainterPaths(linePoints, arrowPoints);
}

QRectF GraphEdge::boundingRect() const
{
    return mBoundingRect;
}

void GraphEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //save painter
    painter->save();

    //set painter options
    painter->setRenderHint(QPainter::Antialiasing);

#ifdef _DEBUG
    //draw bounding rect
    painter->setPen(QPen(Qt::magenta, 1));
    painter->drawRect(boundingRect());
#endif //_DEBUG


    int lineSize = 2;

    //draw line
    painter->setPen(QPen(mEdgeColor, lineSize));
    painter->drawPath(mLine);

    //draw arrow
    painter->setBrush(QBrush(mEdgeColor));
    painter->setPen(QPen(mEdgeColor, lineSize));
    painter->drawPath(mArrow);

#ifdef DRAW_OGDF_BENDS
    // draw ogdf bend point
    painter->setPen(QPen(Qt::magenta, 4));
    painter->drawPoint(mBendPoint);
#endif

    //restore painter
    painter->restore();
}

qreal GraphEdge::calculateDistance(QPointF p1, QPointF p2)
{
    QPointF d = p2 - p1;
    return sqrt(d.x() * d.x() + d.y() * d.y());
}

QPointF GraphEdge::calculateNearestIntersect(QRectF rect, QPointF p1, QPointF p2)
{
    QPointF intersectPoint;
    QLineF line(p1, p2);
    auto rectLines = getLinesFromRect(rect);

    for(auto& rectLine : rectLines)
    {
        if(rectLine.intersect(line, &intersectPoint) == QLineF::BoundedIntersection)
            break;
    }

    return intersectPoint;
}

QList<QPointF> GraphEdge::calculateLine(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect)
{
    QPointF nearestI = calculateNearestIntersect(sourceRect, start, end);
    start=nearestI;

    nearestI = calculateNearestIntersect(targetRect, start, end);
    end=nearestI;

    QList<QPointF> linePoints;

    // handle bend points properly since ogdf doesn't do that...
    if(bends.size())
        linePoints = calculateBendPointsAndAdd(start, end, bends);
    else
    {
        linePoints << start << end;
    }

    return linePoints;
}

QList<QPointF> GraphEdge::calculateArrow(const QList<QPointF> & linePoints)
{
    //arrow
    int len=linePoints.length();
    QLineF perpLine = QLineF(linePoints[len-1], linePoints[len-2]).normalVector();



    QLineF a;
    a.setP1(linePoints[len-1]);
    a.setAngle(perpLine.angle() - 45);
    a.setLength(mArrowLen);

    QLineF b;
    b.setP1(linePoints[len-1]);
    b.setAngle(perpLine.angle() - 135);
    b.setLength(mArrowLen);

    QLineF c;
    c.setP1(a.p2());
    c.setP2(b.p2());

    QList<QPointF> arrowPoints;
    arrowPoints << a.p1() << a.p2() << c.p1() << c.p2() << b.p2() << b.p1();
    return arrowPoints;
}

QRectF GraphEdge::calculateBoundingRect(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints)
{
    QList<QPointF> allPoints;
    allPoints << linePoints << arrowPoints;
    //find top-left and bottom-right points for the bounding rect
    QPointF topLeft = allPoints[0];
    QPointF bottomRight = topLeft;
    for(auto p : allPoints)
    {
        qreal x = p.x();
        qreal y = p.y();

        if(x < topLeft.x())
            topLeft.setX(x);
        if(y < topLeft.y())
            topLeft.setY(y);

        if(x > bottomRight.x())
            bottomRight.setX(x);
        if(y > bottomRight.y())
            bottomRight.setY(y);
    }
    return QRectF(topLeft, bottomRight);
}

void GraphEdge::preparePainterPaths(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints)
{
//    if(mBends.size() != mBends.size())
//    {
//        QPointF lineP[3] = {
//            linePoints[0],
//            QPointF(mBends.back().m_x, mBends.back().m_y),
//            linePoints[2]
//        };
//        mLine = GraphProcessor::quadraticBezierPath(lineP, 3, 2000);
//    }


    //edge line
    QPolygonF polyLine;
    for(auto p : linePoints)
        polyLine << p;

    mLine.addPolygon(polyLine);

    //arrow
    QPolygonF polyArrow;
    for(auto p : arrowPoints)
        polyArrow << p;

    mArrow.addPolygon(polyArrow);

}

void GraphEdge::setEdgeColor(QBrush edgeColor)
{
    mEdgeColor = edgeColor;
}

GraphEdge::EDGE_TYPE GraphEdge::getEdgeType()
{
    return mEdgeType;
}

QPainterPath GraphEdge::shape() const
{
    return mLine;
}

QList<QPointF> GraphEdge::calculateBendPointsAndAdd(QPointF start, QPointF end, const ogdf::DPolyline& bends)
{

    QList<QPointF> linePoints;
    linePoints << start;

    // If edge doesn't collide with any node, no need to adjust anything
    auto polygon = polygonFromEdge(start, end, bends);
    if(!isEdgeCollidingWithNodes(polygon))
    {
        // Add all bend points
        for(const auto& bend : bends)
        {
            linePoints << QPointF(bend.m_x, bend.m_y);
        }

        // Add last point
        linePoints << end;

        return linePoints;
    }


    auto centerMinNodeDistance = mMinNodeDistance / 2.f;

    // startEndDirection > 0 => topDown direction
    // startEndDirection < 0 => bottomUp direction
    auto startEndDirection = end.y() - start.y();

    if(startEndDirection < 0) // this is intended to work for topDown direction, so just swap for bottomUp
        std::swap(start, end);

    for(const auto& bend : bends)
    {
        // Fix to
        // -----
        // | B |
        // -----
        //   |
        //   |_ (2)
        // (1) |
        //   -----
        //   | B |
        //   -----
        if(bend.m_x == start.x())
        {
            QPointF bendPoint1(bend.m_x, end.y() - centerMinNodeDistance); // (1)
            QPointF bendPoint2(end.x(), end.y() - centerMinNodeDistance);  // (2)

            vector<QPointF*> bendsPoints = {&bendPoint1, &bendPoint2};
            fixOverlappingArrows(start, end, linePoints, bend, bendsPoints);

            linePoints << bendPoint1 << bendPoint2;
        }

        // Fix to
        //   -----
        //   | B |
        //   -----
        // (2)_|
        //   | (1)
        //   |
        // -----
        // | B |
        // -----
        else if(bend.m_x == end.x())
        {
            QPointF bendPoint1(start.x(), start.y() + centerMinNodeDistance); // (1)
            QPointF bendPoint2(bend.m_x, start.y() + centerMinNodeDistance);  // (2)

            vector<QPointF*> bendsPoints = {&bendPoint1, &bendPoint2};
            fixOverlappingArrows(start, end, linePoints, bend, bendsPoints);

            linePoints << bendPoint1 << bendPoint2;
        }

        // Fix to (when edge overlaps with middle block)
        // -----
        // | B |
        // -----
        //   |___ (2)
        //  (1)  |
        //       |
        // ----- |
        // | B | |
        // ----- |
        //       |
        // (4)___|
        //   |   (3)
        // -----
        // | B |
        // -----
        else
        {
            QPointF bendPoint1(start.x(), start.y() + centerMinNodeDistance); // (1)
            QPointF bendPoint2(bend.m_x, start.y() + centerMinNodeDistance);  // (2)
            QPointF bendPoint3(bend.m_x, end.y() - centerMinNodeDistance);    // (3)
            QPointF bendPoint4(end.x(), end.y() - centerMinNodeDistance);     // (4)

            vector<QPointF*> bendsPoints = {&bendPoint1, &bendPoint2, &bendPoint3, &bendPoint4};
            fixOverlappingArrows(start, end, linePoints, bend, bendsPoints);

            linePoints << bendPoint1 << bendPoint2 << bendPoint3 << bendPoint4 ;
        }
    }

    linePoints.push_back(end);

#ifdef DRAW_OGDF_BENDS
    mBendPoint = QPointF(bends.back().m_x, bends.back().m_y);
#endif

    return linePoints;
}

void GraphEdge::fixOverlappingArrows(QPointF& start, QPointF& end,  QList<QPointF>& linePoints, const ogdf::DPoint& ogdfBend, vector<QPointF*>& bendPoints)
{
    int offset;
    auto bendPoint = bendPoints.front();

    // If ogdfBend is on the left of startPoint, get offset towards left
    if(ogdfBend.m_x < start.x())
        offset = getOffsetToAvoidOverlappingEdges(start, *bendPoint, TO_LEFT);

    // otherwise get offset towards right
    else
        offset = getOffsetToAvoidOverlappingEdges(start, *bendPoint, TO_RIGHT);

    // adjust start point and first bend point with offset
    start.setX(start.x() + offset);
    bendPoint->setX(bendPoint->x() + offset);
    linePoints.replace(0, start);

    bendPoint = bendPoints.back();

    // If ogdfBend is on the left of startPoint, get offset towards left
    if(ogdfBend.m_x < end.x())
        offset = getOffsetToAvoidOverlappingEdges(end, *bendPoint, TO_LEFT);

    // otherwise get offset towards right
    else
        offset = getOffsetToAvoidOverlappingEdges(end, *bendPoint, TO_RIGHT);

    // adjust end point and last bend point with offset
    end.setX(end.x() + offset); // no need to replace it in linePoints, since it's not there yet
    bendPoint->setX(bendPoint->x() + offset);
}

int GraphEdge::getOffsetToAvoidOverlappingEdges(QPointF p1, QPointF p2, SEARCH_DIRECTION searchDirection)
{
    auto offset = 0;
    auto firstIteration = true;
    auto defaultOffsetSize = searchDirection == TO_RIGHT ? 1 : -1;

    QPolygonF tempPolygon;
    tempPolygon << p1 << p2;

    QPainterPath tempPath;
    tempPath.addPolygon(tempPolygon);

    for(GraphEdge* edgeItem : *mGraphEdgeItems)
    {
        // keep increasing the offset while it's colliding with another edge
        while(edgeItem->collidesWithPath(tempPath))
        {
            tempPath = QPainterPath();

            // add the size of the arrow first on the firstIteration...
            if(firstIteration)
            {
                for(auto& point : tempPolygon)
                {
                    point.setX(point.x() + (mArrowLen + 1) * defaultOffsetSize);
                    offset += (mArrowLen + 1) * defaultOffsetSize;
                }

                firstIteration = false;
            }

            // ...then +1/-1
            else
            {
                for(auto& point : tempPolygon)
                {
                    point.setX(point.x() + defaultOffsetSize);
                    offset += defaultOffsetSize;
                }
            }
            tempPath.addPolygon(tempPolygon);
        }
    }

//    if(offset)
//    {
//        std::cerr << "Offset : " << offset << endl;
//        cerr.flush();
//    }
    return offset;
}

QPolygonF GraphEdge::polygonFromEdge(const QPointF& start, const QPointF& end, const ogdf::DPolyline& bends) const
{
    QPolygonF polygon;
    polygon << start;

    for(auto& b : bends)
        polygon << QPointF(b.m_x, b.m_y);

    polygon << end;

    return polygon;
}

bool GraphEdge::isEdgeCollidingWithNodes(const QPolygonF& edgePolygon) const
{
    QPainterPath tempPath;
    tempPath.addPolygon(edgePolygon);

    for(GraphNode* nodeItem : *mGraphNodeItems)
    {
        // TODO : Fix boundingRect(), maybe have to rewrite GraphNode to inherit a QGraphicsItem rather than a QWidget
        // Using findIntersectionsPoints here instead of nodeProxy->collidesWithPath because nodeProxy->boundingRect() gives wrong x/y positions
        auto intersectPoints = findIntersectionPoints(nodeItem->geometry(), tempPath).size();

        if(intersectPoints)
            return true;
    }

    return false;
}


QList<QLineF> getLinesFromRect(QRectF& rect)
{
//    cerr << "-------------------------" << endl;
//    cerr << rect.x() << "," << rect.y() << " Size : " << rect.width() << "," << rect.height() << endl;
//    cerr << "-------------------------" << endl;
//    cerr.flush();

    QList<QLineF> rectLines;

    // Top line
    QPointF lA(rect.x(), rect.y());
    QPointF lB(rect.x() + rect.width(), rect.y());
    rectLines << QLineF(lA, lB);

    // Left Line
    lB.setX(rect.x());
    lB.setY(rect.y() + rect.height());
    rectLines << QLineF(lA, lB);

    // Bottom Line
    lA.setX(rect.x() + rect.width());
    lA.setY(rect.y() + rect.height());
    rectLines << QLineF(lA, lB);

    // Right Line
    lB.setX(rect.x() + rect.width());
    lB.setY(rect.y());
    rectLines << QLineF(lA, lB);

    return rectLines;
}

QList<QLineF> getLinesFromPainterPath(QPainterPath& painterPath)
{
    QList<QLineF> edgeLines;
    QList<QPointF> edgePoints;

    auto edgeElementC = painterPath.elementCount();
    for(int i=0; i < edgeElementC; ++i)
    {
        edgePoints << QPointF(painterPath.elementAt(i).x, painterPath.elementAt(i).y);

        if((edgePoints.size() & 1) == 0)
        {
            edgeLines  << QLineF(edgePoints.at(0), edgePoints.at(1));
            edgePoints.clear();
        }
    }

    // Add last point to make a last line
    if(edgeElementC > 2 && (edgeElementC & 1) == 1)
    {
        QPointF beforeLastP(painterPath.elementAt(edgeElementC-2).x, painterPath.elementAt(edgeElementC-2).y);
        QPointF lastP(painterPath.elementAt(edgeElementC-1).x, painterPath.elementAt(edgeElementC-1).y);
        edgeLines << QLineF(beforeLastP, lastP);
    }

    return edgeLines;
}

QList<QPointF> findIntersectionPoints(QRectF& node, QPainterPath& edge)
{
    QPointF intersectPoint;
    QList<QPointF> intersectPoints;
    auto nodeLines = getLinesFromRect(node);
    auto edgeLines = getLinesFromPainterPath(edge);

    // Loop through all node border lines
    for(auto& nodeLine : nodeLines)
    {
        // Loop through all edge lines
        for(auto& edgeLine : edgeLines)
        {
            // If an edge line intersects with a node border line, add the intersect point to the list
            if(nodeLine.intersect(edgeLine, &intersectPoint) == QLineF::BoundedIntersection)
            {
                intersectPoints << intersectPoint;
            }
        }
    }

    return intersectPoints;
}

