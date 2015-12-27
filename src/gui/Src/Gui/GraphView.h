#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include "ControlFlowGraph.h"

class GraphView : public QWidget
{
    Q_OBJECT

public:
    explicit GraphView(QWidget *parent = 0);
    ~GraphView();

private:
    duint getEip();

public slots:
    void dbgStateChangedSlot(DBGSTATE state);
    void setControlFlowInfosSlot(duint *controlFlowInfos);
    void startControlFlowAnalysis();
    void drawGraphAtAddressSlot(dsint va, dsint eip);

private:
    duint mEip;
    bool bControlFlowAnalysisStarted;
    QVBoxLayout *mVLayout;
    QPushButton *mButton;
    std::unique_ptr<ControlFlowGraph, std::function<void(ControlFlowGraph *ctrlFlowGraph)>> mControlFlowGraph;

};

#endif // GRAPHVIEW_H
