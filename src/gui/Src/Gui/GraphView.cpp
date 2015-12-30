
#include "GraphView.h"

void deleteControlFlowGraph(ControlFlowGraph* ctrlFlowGraph)
{
    ctrlFlowGraph->deleteLater();
}


GraphView::GraphView(QWidget *parent) :
    QWidget(parent),
    mVLayout(new QVBoxLayout()),
    mControlFlowGraph(new ControlFlowGraph, deleteControlFlowGraph),
    bControlFlowAnalysisStarted(false)
{
    mButton = new QPushButton("Run the cfanalyze command or click this button to start analysis", this);

//    mVLayout->addWidget(mControlFlowGraph.get());
    mVLayout->addWidget(mButton);
    setLayout(mVLayout);

    connect(mButton, SIGNAL(clicked()), this, SLOT(startControlFlowAnalysis()));
    connect(Bridge::getBridge(), SIGNAL(setControlFlowInfos(duint*)), this, SLOT(setControlFlowInfosSlot(duint*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint,dsint)), this, SLOT(drawGraphAtAddressSlot(dsint, dsint)));
}

GraphView::~GraphView()
{
    mVLayout->deleteLater();
}

duint GraphView::getEip()
{
    REGDUMP regDump;
    DbgGetRegDump(&regDump);
    return regDump.regcontext.cip;
}

void GraphView::setControlFlowInfosSlot(duint *controlFlowInfos)
{
    if(controlFlowInfos && mControlFlowGraph.get())
    {
        if(mControlFlowGraph.get())
        {
            mVLayout->removeWidget(mControlFlowGraph.get());
            mControlFlowGraph.reset();
            mControlFlowGraph = std::make_unique<ControlFlowGraph>();
            mVLayout->addWidget(mControlFlowGraph.get());
        }

        auto controlFlowStruct = reinterpret_cast<CONTROLFLOWINFOS*>(controlFlowInfos);
        auto basicBlockInfo = reinterpret_cast<BASICBLOCKMAP*>(controlFlowStruct->blocks);

        delete controlFlowStruct;

        mControlFlowGraph->setBasicBlocks(basicBlockInfo);
        mControlFlowGraph->drawGraphAtSlot(mEip, mEip);
    }
}

void GraphView::startControlFlowAnalysis()
{
    if(!DbgIsDebugging())
    {
        QMessageBox::information(this, "Not debugging", "This only works on a debugged process !");
        return;
    }

    mEip = getEip();
    mButton->hide();
    mControlFlowGraph->startControlFlowAnalysis();
    bControlFlowAnalysisStarted = true;
}

void GraphView::drawGraphAtAddressSlot(dsint va, dsint eip)
{
    mEip = eip;

    if(bControlFlowAnalysisStarted)
        mControlFlowGraph->drawGraphAtSlot(va, eip);
}

void GraphView::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized)
    {
        if(mControlFlowGraph.get() == nullptr)
            mControlFlowGraph = std::make_unique<ControlFlowGraph>();

        mButton->show();
    }
    else if(state == stopped)
    {
        mButton->show();
        mControlFlowGraph.reset();
    }
}

