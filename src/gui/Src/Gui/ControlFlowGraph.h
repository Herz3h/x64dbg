#ifndef CONTROLFLOWGRAPH_H
#define CONTROLFLOWGRAPH_H

#include <set>
#include <map>
#include <QWidget>
#include <QWheelEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include "Bridge.h"
#include "Tree.h"
#include "GraphNode.h"
#include "GraphEdge.h"
#include "QBeaEngine.h"
#include "capstone_gui.h"
#include "QGraphView.h"
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>

struct BasicBlock
{
    duint start;
    duint end;
    duint left;
    duint right;
    duint function;

    BasicBlock()
    {
        this->start = 0;
        this->end = 0;
        this->left = 0;
        this->right = 0;
        this->function = 0;
    }

    BasicBlock(duint start, duint end, duint left, duint right)
    {
        this->start = start;
        this->end = end;
        this->left = std::min(left, right);
        this->right = std::max(left, right);
        this->function = 0;
    }

    std::string toString()
    {
        char buffer[MAX_STRING_SIZE];
        sprintf_s(buffer, "start:%p,end:%p,left:%p,right:%p,func:%p", start, end, left, right, function);
        return std::string(buffer);
    }
};

typedef std::map<duint, BasicBlock> BASICBLOCKMAP;
typedef std::map<duint, Node<GraphNode *> * > NODEMAP;
typedef std::vector<std::unique_ptr<GraphNode>> GRAPHNODEVECTOR;
typedef std::map<ogdf::node, std::vector<std::unique_ptr<GraphEdge>> > GRAPHEDGEMAP;
typedef std::unique_ptr<GRAPHNODEVECTOR, std::function<void(GRAPHNODEVECTOR*)>> GRAPHNODEVECTORDEL;

class ControlFlowGraph : public QWidget
{
    Q_OBJECT
public:
    explicit ControlFlowGraph(QWidget *parent = 0);
    void startControlFlowAnalysis();
    void setUnconditionalBranchEdgeColor();
    void setupGraph();
    ~ControlFlowGraph();

private:
    void setupGraphLayout();
    void setupTree(duint va = 0);
    void addGraphToScene();
    void adjustNodesSize();
    void addNodesToScene();
    void addEdgesToScene();

    void addAllNodes(BASICBLOCKMAP::iterator it, Node<GraphNode *> *parentNode);
    bool findBasicBlock(duint& va);
    void readBasicBlockInstructions(BASICBLOCKMAP::iterator it, std::vector<Instruction_t>& instructionsVector, std::vector<duint>& instructionsAddresses);

public slots:
    void drawGraphAtSlot(duint va, duint eip);
    void setBasicBlocks(BASICBLOCKMAP* basicBlockInfo);

private:
    bool bProgramInitialized;
    duint mEip;
    QBeaEngine* mDisas;
    BASICBLOCKMAP* mBasicBlockInfo;
    GRAPHEDGEMAP mNodeGraphEdge;
    GRAPHNODEVECTORDEL mGraphNodeVector;
    ogdf::Graph mG;
    std::unique_ptr<ogdf::GraphAttributes> mGA;
    std::unique_ptr<ogdf::SugiyamaLayout> mSL;
    std::unique_ptr<ogdf::OptimalHierarchyLayout> mOHL;
    std::unique_ptr<Tree<GraphNode*>> mTree;
    QVBoxLayout* mVLayout;
    QGraphicsScene* mScene;
    QGraphView* mGraphicsView;
    std::vector<GraphEdge*> mGraphEdgeItems;
    std::vector<QGraphicsProxyWidget*> mGraphNodeProxies;
    const float mMinNodeDistance = 25.0; // this distance is min when fixedLayerDistance=False

};

#endif // CONTROLFLOWGRAPH_H
