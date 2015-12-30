#include "GraphNode.h"
#include <iostream>
#include <QGraphicsSceneMouseEvent>
#include "Configuration.h"

using namespace std;

GraphNode::GraphNode() : QGraphicsWidget()
{
}

GraphNode::GraphNode(vector<Instruction_t> &instructionsVector, vector<duint> &instructionsAddresses, duint address, duint eip)
    :
    QGraphicsWidget(),
    mAddress(address),
    mInstructionsVector(instructionsVector),
    mInstructionsAddresses(instructionsAddresses),
    mEip(eip),
    mHighlightInstructionAt(-1)
{
    updateTokensVector();

    setAcceptHoverEvents(true);
}

GraphNode::GraphNode(const GraphNode & other)
{
    setInstructionsVector(other.mInstructionsVector);
}

GraphNode & GraphNode::operator=(const GraphNode & other)
{
    // TODO : Complete this
    setInstructionsVector(other.mInstructionsVector);
    return *this;
}

void GraphNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->save();

    // Draw node block background
    painter->fillRect(0, 0, mCachedWidth, mCachedHeight, Qt::white);

    // Draw node borders
    painter->setPen(QPen(Qt::black, 1));
    painter->drawLine(0, mLineHeight, 0, mCachedHeight-1);
    painter->drawLine(mCachedWidth-1, mLineHeight, mCachedWidth-1, mCachedHeight-1);
    painter->drawLine(0, mCachedHeight-1, mCachedWidth-1, mCachedHeight-1);

    //draw node contents
    painter->setFont(this->mFont);
    int i = 0;
    int x = mSpacingX * 0.5 + mSpacingBreakpoint, y = mLineHeight + mSpacingY * 0.5;

    // Block address
    painter->fillRect(0, 0, mCachedWidth, mLineHeight, Qt::cyan);
    painter->setPen(Qt::black);
    painter->drawText(0, 0, mCachedWidth, mLineHeight, Qt::AlignHCenter | Qt::AlignVCenter, "0x" + QString::number(mAddress, 16).toUpper());

    // Block instructions
    for(QList<RichTextPainter::CustomRichText_t> &richText : mRichTextVector)
    {
        // Draw instructions
        RichTextPainter::paintRichText(painter, x, y, mCachedWidth, mLineHeight, 0, &richText, QFontMetrics(this->mFont).width(QChar(' ')));

        // Draw circle if on a BP
        drawBp(painter, mInstructionsAddresses[i], 0, y);

        // Draw EIP "cursor"
        if(mInstructionsAddresses[i] == mEip)
            drawEip(painter, 0, y);

        // Draw background of highlighted instruction if there is any highlighted
        if(mHighlightInstructionAt == i)
            painter->fillRect(0, y, mCachedWidth, mLineHeight, QBrush(QColor(0, 0, 0, 150)));

        y += mLineHeight + mLineSpacingY;
        ++i;
    }

    painter->restore();
}

duint GraphNode::getAddress()
{
    return mAddress;
}

dsint GraphNode::getInstructionIndexAtPos(const QPointF& pos) const
{
    // Gets the instruction index at the cursor position
    duint instructionIndex = -1;
    for(duint i = 0; i < mInstructionsVector.size(); ++i)
    {
        dsint currentInstructionMinHeight = (i+1) * (mLineHeight + mLineSpacingY) + (mSpacingY/2);
        dsint currentInstructionMaxHeight = (i+2) * (mLineHeight+mLineSpacingY) + (mSpacingY/2);

        if(pos.y() >= currentInstructionMinHeight && pos.y() <= currentInstructionMaxHeight)
        {
            instructionIndex = i;
            break;
        }
    }

    return instructionIndex;
}

QString GraphNode::getLongestInstruction()
{
    dsint maxInstructionLength = 0;
    QString maxInstruction = "";

    for(CapstoneTokenizer::InstructionToken &token : mTokensVector)
    {
        dsint currentInstructionLength = 0;
        QString currentInstruction = "";
        for(CapstoneTokenizer::SingleToken &singleToken : token.tokens)
        {
            currentInstructionLength += singleToken.text.length();
            currentInstruction += singleToken.text;
        }

        if(currentInstructionLength > maxInstructionLength)
        {
            maxInstructionLength = currentInstructionLength;
            maxInstruction = currentInstruction;
        }
    }

    return maxInstruction;
}

void GraphNode::drawEip(QPainter *painter, int x, int y)
{
    duint topLeftArrowX = mSpacingBreakpoint * 0.25;
    duint topLeftArrowY = y + (mLineHeight * 0.2);
    duint middleRightArrowX = mSpacingBreakpoint * 0.5;
    duint middleRightArrowY = y+(mLineHeight * 0.5);
    duint bottomLeftArrowX = topLeftArrowX;
    duint bottomLeftArrowY = y + mLineHeight - (mLineHeight * 0.2);


    QPointF points[3] =
    {
        QPointF(topLeftArrowX, topLeftArrowY),
        QPointF(middleRightArrowX, middleRightArrowY),
        QPointF(bottomLeftArrowX, bottomLeftArrowY)
    };

    painter->setBrush(QBrush(Qt::blue));
    painter->drawPolygon(points, 3);

    painter->setPen(QPen(Qt::blue, 2));
    painter->drawLine(0, middleRightArrowY, topLeftArrowX, middleRightArrowY);
}

void GraphNode::drawBp(QPainter* painter, duint address, int x, int y)
{
    if(DbgGetBpxTypeAt(address) != bp_none)
    {
        duint circleRadius = mSpacingBreakpoint * 0.3;
        duint circleCenterX = mSpacingBreakpoint * 0.9;
        duint circleCenterY = y + (mLineHeight * 0.5);
        painter->setBrush(QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor")));
        painter->drawEllipse(QPointF(circleCenterX, circleCenterY), circleRadius, circleRadius);
    }
}

void GraphNode::updateCache()
{
    QString maxInstruction = getLongestInstruction();


    QFontMetrics metrics(this->mFont);
    mCachedWidth = metrics.width(maxInstruction) + mSpacingX + mSpacingBreakpoint;
    mCachedHeight =((metrics.height() + mLineSpacingY) * mInstructionsVector.size()) + metrics.height() + mSpacingY; // +metrics.height() => for block address line
    mLineHeight = metrics.height();
}

void GraphNode::updateRichText()
{
    for(CapstoneTokenizer::InstructionToken &token : mTokensVector)
    {
        QList<RichTextPainter::CustomRichText_t> richText;
        CapstoneTokenizer::TokenToRichText(token, richText, 0);
        mRichTextVector.push_back(richText);
    }
}


void GraphNode::updateTokensVector()
{
    for(duint i=0; i < mInstructionsVector.size(); ++i)
    {
        mTokensVector.push_back(mInstructionsVector[i].tokens);
    }

    updateCache();
    updateRichText();
}


bool GraphNode::sceneEvent(QEvent* event)
{
    // Mouse click
    if(event->type() == QEvent::GraphicsSceneMousePress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if(mouseEvent->button() != Qt::LeftButton)
            return true;

        duint clickedInstructionIndex = getInstructionIndexAtPos(mouseEvent->pos());

        // No instruction clicked
        if(clickedInstructionIndex == -1)
            return true;

        Instruction_t clickedInstruction = mInstructionsVector.at(clickedInstructionIndex);
        if(clickedInstruction.branchDestination != 0)
            emit drawGraphAt(clickedInstruction.branchDestination, mEip);

        return true;
    }

    // Mouse move
    else if(event->type() == QEvent::GraphicsSceneHoverMove)
    {
        setCursor(Qt::ArrowCursor);

        QGraphicsSceneMouseEvent* mouseEvent = reinterpret_cast<QGraphicsSceneMouseEvent*>(event);
        mHighlightInstructionAt = getInstructionIndexAtPos(mouseEvent->pos());

        update();
        return true;
    }

    // Leaving widget area
    else if(event->type() == QEvent::GraphicsSceneHoverLeave)
    {
        mHighlightInstructionAt = -1;
        update();
        return true;
    }

    return false;
}

void GraphNode::setInstructionsVector(const vector<Instruction_t> &instructionsVector)
{
    mInstructionsVector = instructionsVector;
}

QRectF GraphNode::boundingRect() const
{
    return QRectF(0, 0, mCachedWidth, mCachedHeight);
}
