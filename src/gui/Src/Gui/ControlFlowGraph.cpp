#include "ControlFlowGraph.h"
#include "Configuration.h"

void deleteGraphNodeVector(GRAPHNODEVECTOR* graphNodeVector)
{
    for(auto& ptr : *graphNodeVector)
        ptr->deleteLater();
}

ControlFlowGraph::ControlFlowGraph(QWidget *parent) : QWidget(parent),
    mBasicBlockInfo(nullptr),
    mDisas(new QBeaEngine(-1)),
    mScene(new QGraphicsScene()),
    mGraphicsView(new QGraphView()),
    bProgramInitialized(false),
    mVLayout(new QVBoxLayout()),
    mGraphNodeVector(new GRAPHNODEVECTOR, deleteGraphNodeVector)
{

    mScene->setBackgroundBrush(ConfigColor("DisassemblyBackgroundColor"));
    mGraphicsView->setScene(mScene);
    mGraphicsView->setDragMode(QGraphView::ScrollHandDrag);

    mVLayout->addWidget(mGraphicsView);
    setLayout(mVLayout);

    setupGraph();

    connect(Bridge::getBridge(), SIGNAL(setControlFlowInfos(duint*)), this, SLOT(setControlFlowInfosSlot(duint*)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint,dsint)), this, SLOT(disassembleAtSlot(dsint, dsint)));

}


void ControlFlowGraph::startControlFlowAnalysis()
{
    DbgCmdExec(QString("cfanalyze").toUtf8().constData());
}

void ControlFlowGraph::setUnconditionalBranchEdgeColor()
{
    for(auto const &nodeGraphEdge : mNodeGraphEdge)
    {
        if(nodeGraphEdge.second.size() == 1)
            nodeGraphEdge.second.at(0)->setEdgeColor(Qt::blue);
    }
}

ControlFlowGraph::~ControlFlowGraph()
{
    mGraphNodeVector.reset();
    mGraphicsView->deleteLater();
    mScene->deleteLater();
    mVLayout->deleteLater();
    mTree.reset();
    mOHL.reset();
    mSL.reset();
    mGA.reset();
    delete mBasicBlockInfo;
    delete mDisas;
}

void ControlFlowGraph::setupGraph()
{
    using namespace ogdf;
    using namespace std;


    //initialize graph
    mGA = make_unique<GraphAttributes>(mG, GraphAttributes::nodeGraphics |
                                       GraphAttributes::edgeGraphics |
                                       GraphAttributes::nodeLabel |
                                       GraphAttributes::nodeStyle |
                                       GraphAttributes::edgeType |
                                       GraphAttributes::edgeArrow |
                                       GraphAttributes::edgeStyle);


    //add nodes
    mTree = make_unique<Tree<GraphNode*>>(&mG, mGA.get());

    // Make sure there is some spacing
    QRectF sceneRect = mGraphicsView->sceneRect();
    sceneRect.adjust(-20, -40, 20, 40);
    mGraphicsView->setSceneRect(sceneRect);

    mGraphicsView->show();
}


void ControlFlowGraph::setupTree(duint va)
{
    if(mBasicBlockInfo == nullptr)
        return;

    using namespace ogdf;
    using namespace std;

    QTextStream out(stdout);
    BASICBLOCKMAP::iterator it;
    std::vector<Instruction_t> instructionsVector;
    std::vector<duint> instructionsAddresses;

    // Clear graph and tree
    mG.clear();
    mTree->clear();
    mGraphNodeVector->clear();

    if(!va)
        it = mBasicBlockInfo->begin();
    else
        it = mBasicBlockInfo->find(va);

    // Couldn't find the basic block of the VA
    if(it == mBasicBlockInfo->end())
        return;


    // Disassemble the instruction at the address
    readBasicBlockInstructions(it, instructionsVector, instructionsAddresses);

    // Add root node first
    duint addr = it->first;
    mGraphNodeVector->push_back( make_unique<GraphNode>(instructionsVector, instructionsAddresses, addr, mEip) );
    connect(mGraphNodeVector->back().get(), SIGNAL(drawGraphAt(duint, duint)), this, SLOT(drawGraphAtSlot(duint, duint)), Qt::QueuedConnection);

    Node<GraphNode *> *rootNode = mTree->newNode(mGraphNodeVector->back().get());
    addAllNodes(it, rootNode);
}

void ControlFlowGraph::setupGraphLayout()
{
    using namespace ogdf;
    using namespace std;


    // TODO : Better way to do this
    if(mOHL.get() && mSL.get())
    {
        mSL->call(*mGA);
        return;
    }


    mOHL = make_unique<OptimalHierarchyLayout>();
    mOHL->nodeDistance(mMinNodeDistance);
    mOHL->layerDistance(50.0);
    mOHL->fixedLayerDistance(false);
    mOHL->weightBalancing(0.0);
    mOHL->weightSegments(0.0);

    mSL = make_unique<SugiyamaLayout>();
    mSL->setRanking(new OptimalRanking);
    mSL->setCrossMin(new MedianHeuristic);
    mSL->alignSiblings(false);
    mSL->setLayout(mOHL.get());
    mSL->call(*mGA);
}

void ControlFlowGraph::addGraphToScene()
{
    using namespace ogdf;

    mNodeGraphEdge.clear();
    mScene->clear();
    mGraphicsView->viewport()->repaint(); // removes some artifacts when drawing a new graph

    adjustNodesSize();

    // Apply the graph layout after we've set the nodes sizes
    setupGraphLayout();

    addNodesToScene();

    addEdgesToScene();

    // Change unconditionalBranches colors to something different than for conditional branches
    setUnconditionalBranchEdgeColor();

    mGraphicsView->setSceneRect(mScene->itemsBoundingRect());
}

void ControlFlowGraph::adjustNodesSize()
{
    // Adjust node size
    ogdf::node v;
    forall_nodes(v, mG)
    {
        Node<GraphNode* > *node = mTree->findNode(v);
        if (node)
        {
            QRectF rect = node->data()->boundingRect();
            mGA->width(v) = rect.width();
            mGA->height(v) = rect.height();
        }
    }
}

void ControlFlowGraph::addNodesToScene()
{
    mGraphNodeItems.clear();

    ogdf::node v;
    forall_nodes(v, mG)
    {
        Node<GraphNode* > *node = mTree->findNode(v);
        if (node)
        {
            //draw node using x,y
            QRectF rect = node->data()->boundingRect();
            qreal x = mGA->x(v) - (rect.width()/2);
            qreal y = mGA->y(v) - (rect.height()/2);
            node->data()->setGeometry(x, y, rect.width(), rect.height());
            mScene->addItem(node->data());
            mGraphNodeItems.push_back(node->data());
        }
    }
}

void ControlFlowGraph::addEdgesToScene()
{
    mGraphEdgeItems.clear();

    //draw edges
    ogdf::edge e;
    forall_edges(e, mG)
    {
        const auto bends = mGA->bends(e);
        const auto source = e->source();
        const auto target = e->target();

        GraphNode* sourceGraphNode = mTree->findNode(source)->data();
        GraphNode* targetGraphNode = mTree->findNode(target)->data();

        QRectF sourceRect = sourceGraphNode->geometry();
        QRectF targetRect = targetGraphNode->geometry();
        sourceRect.adjust(-4, -4, 4, 4);
        targetRect.adjust(-4, -4, 4, 4);

        QPointF start(mGA->x(source), mGA->y(source));
        QPointF end(mGA->x(target), mGA->y(target));

        GraphEdge* edge = nullptr;
        auto const sourceNodeLeft = mTree->findNode(source)->left();

        if(sourceNodeLeft && sourceNodeLeft->data()->getAddress() == targetGraphNode->getAddress())
            edge = new GraphEdge(start, end, bends, sourceRect, targetRect, GraphEdge::EDGE_LEFT, mMinNodeDistance, &mGraphNodeItems, &mGraphEdgeItems);
        else
            edge = new GraphEdge(start, end, bends, sourceRect, targetRect, GraphEdge::EDGE_RIGHT, mMinNodeDistance, &mGraphNodeItems, &mGraphEdgeItems);

        mNodeGraphEdge[source].push_back(std::unique_ptr<GraphEdge>(edge));
        mScene->addItem(edge);
        mGraphEdgeItems.push_back(edge);
    }
}

void ControlFlowGraph::addAllNodes(BASICBLOCKMAP::iterator it, Node<GraphNode *> *parentNode)
{
    using namespace std;

    QByteArray byteArray(MAX_STRING_SIZE, 0);
    duint left  = it->second.left; // Left child addr
    duint right = it->second.right; // Right child addr

    // No childs
    if(!left && !right)
        return;

    for(int i=0; i < 2; i++)
    {
        // If not left or right child continue..
        if((i == 0 && !left) || (i == 1 && !right))
            continue;

        BASICBLOCKMAP::iterator itChild;
        Node<GraphNode*>* node = nullptr;

        // Left
        if(i == 0)
        {
            itChild = mBasicBlockInfo->find(left);
            node = mTree->findNodeByAddress(left);
        }
        //Right
        else
        {
            itChild = mBasicBlockInfo->find(right);
            node = mTree->findNodeByAddress(right);
        }

        // If we found the basicblock for left or right
        if(itChild != mBasicBlockInfo->end())
        {
            duint childNodeAddr;

            if(i == 0) // Add node to left of parentNode
                childNodeAddr = left;
            else       // Add node to right of parentNode
                childNodeAddr = right;

            // Node does not exist, create it
            if(node == nullptr)
            {
                std::vector<Instruction_t> instructionsVector;
                std::vector<duint> instructionsAddresses;
                readBasicBlockInstructions(itChild, instructionsVector, instructionsAddresses);

                mGraphNodeVector->push_back( make_unique<GraphNode>(instructionsVector, instructionsAddresses, childNodeAddr, mEip) );

                node = mTree->newNode(mGraphNodeVector->back().get());

                connect(mGraphNodeVector->back().get(), SIGNAL(drawGraphAt(duint, duint)), this, SLOT(drawGraphAtSlot(duint, duint)), Qt::QueuedConnection);
            }

            Node<GraphNode*>* newParentNode = nullptr;
            Node <GraphNode*>* parentNodeLeftRight  = nullptr;

            if(i == 0)
                parentNodeLeftRight = parentNode->left();
            else
                parentNodeLeftRight = parentNode->right();

            // Edge already exists between parentNode and left / right, we've been here before..
            if(parentNodeLeftRight && (parentNodeLeftRight->data()->getAddress() == left || parentNodeLeftRight->data()->getAddress() == right))
                return;

            if(i == 0)
                newParentNode = parentNode->setLeft(node);
            else
                newParentNode = parentNode->setRight(node);

            addAllNodes(itChild, newParentNode);
        }
    }


}



bool ControlFlowGraph::findBasicBlock(duint& va)
{
    if(mBasicBlockInfo == nullptr)
        return false;

    if(!mBasicBlockInfo->size())
        return false;

    bool bFound = false;
    BASICBLOCKMAP::iterator it = mBasicBlockInfo->find(va);

    // If the block was not found by addr, check if it belongs to a basic block
    if(it == mBasicBlockInfo->end())
    {
        for(it = mBasicBlockInfo->begin(); it != mBasicBlockInfo->end(); it++)
        {
            if(va >= it->second.start && va <= it->second.end)
            {
                va = it->first;
                bFound = true;
                break;
            }
        }
    }
    else
        bFound = true;

    return bFound;
}

void ControlFlowGraph::readBasicBlockInstructions(BASICBLOCKMAP::iterator it, std::vector<Instruction_t> &instructionsVector, std::vector<duint>& instructionsAddresses)
{
    duint addr = it->first;
    duint startAddr = it->second.start;
    duint endAddr = it->second.end;
    duint baseAddr = DbgMemFindBaseAddr(addr, 0);
    QByteArray byteArray(MAX_STRING_SIZE, 0);

    // Read basic block instructions
    for(startAddr; startAddr <= endAddr;)
    {
        if(!DbgMemRead(startAddr, reinterpret_cast<unsigned char*>(byteArray.data()), 16))
            return;

        // Add instruction to the vector
        Instruction_t wInstruction = mDisas->DisassembleAt(reinterpret_cast<byte_t*>(byteArray.data()), byteArray.length(), 0, baseAddr, startAddr-baseAddr);

        instructionsVector.push_back(wInstruction);
        instructionsAddresses.push_back(startAddr);
        startAddr += wInstruction.length;
    }
}

void ControlFlowGraph::drawGraphAtSlot(duint va, duint eip)
{
    mEip = eip;
    bool bFound = findBasicBlock(va);

    if(!bFound)
    {
        QString labelText = "<h1 style='background-color:" + ConfigColor("DisassemblyBackgroundColor").name() + "'>";
        labelText += "No graph available for current instruction ";
        labelText += "0x" + QString::number(va, 16).toUpper() + "</h4>";

        QLabel *label = new QLabel(labelText);
        mScene->clear();
        mScene->addWidget(label);

        mGraphicsView->setSceneRect(mScene->itemsBoundingRect());

        return;
    }

    setupTree(va);
    addGraphToScene();

    mGraphicsView->verticalScrollBar()->setValue(0);
}

void ControlFlowGraph::setBasicBlocks(BASICBLOCKMAP *basicBlockInfo)
{
    mBasicBlockInfo = basicBlockInfo;
}
