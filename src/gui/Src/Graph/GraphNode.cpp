#include "GraphNode.h"
#include <iostream>
#include "Configuration.h"

using namespace std;

GraphNode::GraphNode() : QWidget()
{
}

GraphNode::GraphNode(vector<Instruction_t> &instructionsVector, vector<duint> &instructionsAddresses, duint address, duint eip)
    :
    mAddress(address),
    mInstructionsVector(instructionsVector),
    mInstructionsAddresses(instructionsAddresses),
    mEip(eip),
    mHighlightInstructionAt(-1)
{
    updateTokensVector();

    setAttribute(Qt::WA_TranslucentBackground);
    setContentsMargins(0,0,0,0);
    setMouseTracking(true); // required for mouse move event
    installEventFilter(this);
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

QRectF GraphNode::boundingRect() const
{
    return this->geometry();
}

QPainterPath GraphNode::shape() const
{
    QPainterPath p;
    p.addRect(this->geometry());
    return p;
}

void GraphNode::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    painter.fillRect(0, 0, mCachedWidth, mCachedHeight, Qt::white);

    // Draw node borders
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(0, mLineHeight, 0, mCachedHeight-1);
    painter.drawLine(mCachedWidth-1, mLineHeight, mCachedWidth-1, mCachedHeight-1);
    painter.drawLine(0, mCachedHeight-1, mCachedWidth-1, mCachedHeight-1);

    //draw node contents
    painter.setFont(this->mFont);
    int i = 0;

    int x = mSpacingX * 0.5 + mSpacingBreakpoint, y = mLineHeight + mSpacingY * 0.5;

    // Block address
    painter.fillRect(0, 0, mCachedWidth, mLineHeight, Qt::cyan);
    painter.setPen(Qt::black);
    painter.drawText(0, 0, mCachedWidth, mLineHeight, Qt::AlignHCenter | Qt::AlignVCenter, "0x" + QString::number(mAddress, 16).toUpper());

    // Block instructions
    for(QList<RichTextPainter::CustomRichText_t> &richText : mRichTextVector)
    {

        RichTextPainter::paintRichText(&painter, x, y, mCachedWidth, mLineHeight, 0, &richText, QFontMetrics(this->mFont).width(QChar(' ')));

        // Draw circle if on a BP
        if(DbgGetBpxTypeAt(mInstructionsAddresses[i]) != bp_none)
        {
            duint circleRadius = mSpacingBreakpoint * 0.3;
            duint circleCenterX = mSpacingBreakpoint * 0.9;
            duint circleCenterY = y + (mLineHeight * 0.5);
            painter.setBrush(QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor")));
            painter.drawEllipse(QPointF(circleCenterX, circleCenterY), circleRadius, circleRadius);
        }

        // Draw EIP "cursor"
        if(mInstructionsAddresses[i] == mEip)
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

            painter.setBrush(QBrush(Qt::blue));
            painter.drawPolygon(points, 3);

            painter.setPen(QPen(Qt::blue, 2));
            painter.drawLine(0, middleRightArrowY, topLeftArrowX, middleRightArrowY);
        }

        if(mHighlightInstructionAt == i)
            painter.fillRect(0, y, mCachedWidth, mLineHeight, QBrush(QColor(0, 0, 0, 150)));

        y += mLineHeight + mLineSpacingY;
        ++i;
    }

}

dsint GraphNode::getInstructionIndexAtPos(const QPoint &pos) const
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

bool GraphNode::eventFilter(QObject *object, QEvent *event)
{
    // Mouse click
    if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if(mouseEvent->button() != Qt::LeftButton)
            return true;

        duint clickedInstructionIndex = getInstructionIndexAtPos(mouseEvent->pos());

        cerr << "Clicked : " << clickedInstructionIndex << endl;
        cerr.flush();
        // No instruction clicked
        if(clickedInstructionIndex == -1)
            return true;

        Instruction_t clickedInstruction = mInstructionsVector.at(clickedInstructionIndex);
        if(clickedInstruction.branchDestination != 0)
            emit drawGraphAt(clickedInstruction.branchDestination, mEip);

        return true;
    }

    // Mouse move
    else if(event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        mHighlightInstructionAt = getInstructionIndexAtPos(mouseEvent->pos());
        repaint();
        return true;
    }

    // Leaving widget area
    else if(event->type() == QEvent::Leave)
    {
        mHighlightInstructionAt = -1;
        repaint();
        return true;
    }

    return false;
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

void GraphNode::setInstructionsVector(const vector<Instruction_t> &instructionsVector)
{
    mInstructionsVector = instructionsVector;
}

void GraphNode::updateCache()
{
    QString maxInstruction = getLongestInstruction();

    QFontMetrics metrics(this->mFont);
    mCachedWidth = metrics.width(maxInstruction) + mSpacingX + mSpacingBreakpoint;
    mCachedHeight =((metrics.height() + mLineSpacingY) * mInstructionsVector.size()) + metrics.height() + mSpacingY; // +metrics.height() => for block address line
    mLineHeight = metrics.height();

    // Will get updated later from ControlFlowGraph
    this->setGeometry(0, 0, mCachedWidth, mCachedHeight);
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

duint GraphNode::address()
{
    return mAddress;
}
