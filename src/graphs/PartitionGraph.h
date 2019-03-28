/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */

#ifndef FSM_GRAPHS_PARTITIONGRAPH_H_
#define FSM_GRAPHS_PARTITIONGRAPH_H_

#include <vector>
#include <memory>
#include "fsm/Dfsm.h"
#include "graphs/Graph.h"
#include "trees/SplittingTreeNode.h"

using namespace std;

class PartitionGraph:public Graph {
protected:

public:

    /**
    * Creates a new partitiongraph
    */
    PartitionGraph();

    /**
    * Creates nodes for partition blocks represented by `nodeA` and `nodeB` respectively, if they do not exist, and adds an edge from
    * `nodeA` to `nodeB` with label `x`
    * @param nodeA the corresponding splitting tree node that contains the block, which represents the node from which the new edge starts
    * @param nodeB the corresponding splitting tree node that contains the block, which represents the node at which the new edge ends
    * @param x the label of the new edge
    */
    void addEdge(shared_ptr<SplittingTreeNode>& nodeA, shared_ptr<SplittingTreeNode>& nodeB,int x);

    /**
    * Check if there is a path of c-valid inputs from `node` to a splitting tree node, that is either marked a-valid or b-valid.
    * If thats the case, a pointer to the target splitting tree node is returned, otherwise a nullptr.
    * If there are multiple options for a path, the one, which results into the shortest input trace for `node`, is chosen.
    * @param node the starting node of the path to search for
    * @param cTrace the trace of c-valid inputs that labels the path from `node` to the a- or b-valid marked target node
    */
    shared_ptr<SplittingTreeNode> findPathToAOrBValidNode(shared_ptr<SplittingTreeNode>& node, vector<int>& cTrace);

};


#endif //FSM_GRAPHS_PARTITIONGRAPH_H_
