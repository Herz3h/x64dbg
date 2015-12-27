#ifndef _NODE_H
#define _NODE_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

template <typename T>
class Node
{
public:
    explicit Node(ogdf::Graph* G, ogdf::GraphAttributes* GA, ogdf::node ogdfNode, T data)
        :
        mG(G),
        mGA(GA),
        mData(data),
        mOgdfNode(ogdfNode),
        mLeft(nullptr),
        mRight(nullptr)
    {}

    explicit Node(const Node& node)
    {
        mG = node.mG;
        mGA = node.mGA;
        mData = node.mData;
        mOgdfNode = node.mOgdfNode;
        mLeft = node.mLeft;
        mRight = node.mRight;
    }

    explicit Node(const Node&& node)
    {
        mG = std::move(node.mG);
        mGA = std::move(node.mGA);
        mData = std::move(node.mData);
        mOgdfNode = std::move(node.mOgdfNode);
        mLeft = std::move(node.mLeft);
        mRight = std::move(node.mRight);
    }

    Node<T>* setLeft(Node* left)
    {
        return left ? mLeft = makeNodeWithEdge(left) : nullptr;
    }
    Node<T>* setRight(Node* right)
    {
        return right ? mRight = makeNodeWithEdge(right) : nullptr;
    }

    Node<T>* setData(T data)
    {
        mData = data;
        return this;
    }

    Node<T>* left() const
    {
        return mLeft;
    }
    Node<T>* right() const
    {
        return mRight;
    }
    T data() const
    {
        return mData;
    }
    ogdf::node ogdfNode() const
    {
        return mOgdfNode;
    }

private:
    T mData;
    Node<T>* mLeft;
    Node<T>* mRight;
    ogdf::Graph* mG;
    ogdf::GraphAttributes* mGA;
    ogdf::node mOgdfNode;

    Node* makeNodeWithEdge(Node<T>* node)
    {
        auto edge = mG->newEdge(mOgdfNode, node->mOgdfNode);
        mGA->arrowType(edge) = ogdf::EdgeArrow::eaLast;
        return node;
    }
};

#endif //_NODE_H
