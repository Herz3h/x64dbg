#ifndef _GRAPH_NODE_H
#define _GRAPH_NODE_H

#include <QWidget>
#include <QFrame>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFontMetrics>
#include <QGraphicsWidget>
#include <vector>
#include "Imports.h"
#include "RichTextPainter.h"
#include "capstone_gui.h"
#include "QBeaEngine.h"

class GraphNode : public QGraphicsWidget
{
    Q_OBJECT

public:
    GraphNode();
    GraphNode(std::vector<Instruction_t>& instructionsVector, std::vector<duint>& instructionsAddresses, duint address = 0, duint eip = 0);
    GraphNode(const GraphNode& other);
    GraphNode& operator=(const GraphNode& other);
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    duint getAddress();
    dsint getInstructionIndexAtPos(const QPointF& pos) const;
    QString getLongestInstruction();
    QRectF boundingRect() const override;

private:
    void drawEip(QPainter* painter, int x, int y);
    void drawBp(QPainter* painter, duint address, int x, int y);
    void updateCache();
    void updateRichText();
    void updateTokensVector();
    bool sceneEvent(QEvent* event) override;
    void setInstructionsVector(const std::vector<Instruction_t>& instructionsVector);

signals:
    void drawGraphAt(duint va, duint eip);

private:
    duint mEip;
    duint mHighlightInstructionAt;
    duint mAddress;
    duint mLineHeight;
    qreal mCachedWidth;
    qreal mCachedHeight;
    const duint mSpacingX = 12;
    const duint mSpacingY = 12;
    const duint mSpacingBreakpoint = 12;
    const duint mWidthEipIndicatorY = 6;
    const duint mLineSpacingY = 3;
    QFont mFont = QFont("Lucida Console", 8, QFont::Normal, false);
    std::vector<Instruction_t> mInstructionsVector;
    std::vector<CapstoneTokenizer::InstructionToken> mTokensVector;
    std::vector<QList<RichTextPainter::CustomRichText_t> > mRichTextVector;
    std::vector<duint> mInstructionsAddresses;
};

#endif //_GRAPH_NODE_H
