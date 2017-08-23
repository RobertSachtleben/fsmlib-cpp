/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_FSM_FSMNODE_H_
#define FSM_FSM_FSMNODE_H_

#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <deque>

#include "fsm/FsmVisitor.h"

class FsmTransition;
class FsmPresentationLayer;
class OutputTree;
class Tree;
class OutputTrace;
class InputTrace;
class OFSMTable;
class PkTable;
class DFSMTableRow;

class FsmNode : public std::enable_shared_from_this<FsmNode>
{
private:
    std::vector<std::shared_ptr<FsmTransition> > transitions;
	int id;
	std::string name;
	bool visited;
	int color;
	std::shared_ptr<FsmPresentationLayer> presentationLayer;
	std::shared_ptr<std::pair<std::shared_ptr<FsmNode>, std::shared_ptr<FsmNode>>> derivedFromPair;
    
    bool isInitialNode;
    
    /**
     *  List of requirements satisfied by the node
     */
    std::vector<std::string> satisfies;
    
public:
	const static int white = 0;
	const static int grey = 1;
	const static int black = 2;
	FsmNode(const int id,
            const std::shared_ptr<FsmPresentationLayer> presentationLayer);
	FsmNode(const int id,
            const std::string & name,
            const std::shared_ptr<FsmPresentationLayer> presentationLayer);
    
    /**
     * Add a transition to the node. If another transition with the same label and
     * the same target node already exists, the new transition is silently ignored.
     */
	void addTransition(std::shared_ptr<FsmTransition> transition);
    
    
    std::vector<std::shared_ptr<FsmTransition> >& getTransitions();
    /**
     * Calculates all possible outputs for a given input.
     * @param The given input.
     * @return All possible outputs for the given input.
     */
    std::vector<OutputTrace> getPossibleOutputs(const int input) const;
	int getId() const;
	std::string getName() const;
	bool hasBeenVisited() const;
	void setVisited();
    void setUnvisited();
	void setPair(const std::shared_ptr<FsmNode> l, const std::shared_ptr<FsmNode> r);
	void setPair(const std::shared_ptr<std::pair<std::shared_ptr<FsmNode>, std::shared_ptr<FsmNode>>> p);
	bool isDerivedFrom(const std::shared_ptr<std::pair<std::shared_ptr<FsmNode>, std::shared_ptr<FsmNode>>> p) const;
	std::shared_ptr<std::pair<std::shared_ptr<FsmNode>, std::shared_ptr<FsmNode>>> getPair() const;
	std::shared_ptr<FsmNode> apply(const int e, OutputTrace & o);
	OutputTree apply(const InputTrace & itrc, bool markAsVisited = false);

	/**
	Return the set of FsmNode instances reachable from this node after
	having applied the input trace itrc
	@param itrc Input Trace to be applied to the FSM, starting with this FsmNode
	@return Set of FsmNode instances reachable from this node via
	input trace itrc.
	*/
	std::unordered_set<std::shared_ptr<FsmNode>> after(const InputTrace & itrc);

    /**
    Return the set of FsmNode instances reachable from this node and with the given output
    trace after having applied the input trace itrc
    @param itrc Input Trace to be applied to the FSM, starting with this FsmNode
    @param itrc Output Trace that has to be produced by the given input.
    @return Set of FsmNode instances reachable from this node via
    input trace itrc witht the given output trace {@code otrc}.
    */
    std::unordered_set<std::shared_ptr<FsmNode>> after(const InputTrace & itrc, const InputTrace & otrc);

	/**
	Return list of nodes that can be reached from this node
	when applying input x

	@param x FSM input, to be applied in this node
	@return empty list, if no transition is defined from this node
	with input x
	list of target nodes reachable from this node under input x
	otherwise.
	*/
	std::vector<std::shared_ptr<FsmNode>> after(const int x);
	std::unordered_set<std::shared_ptr<FsmNode>> afterAsSet(const int x);
    std::unordered_set<std::shared_ptr<FsmNode>> afterAsSet(const int x, const int y);
	void setColor(const int color);
	int getColor();
	std::shared_ptr<DFSMTableRow> getDFSMTableRow(const int maxInput);
	bool distinguished(const std::shared_ptr<FsmNode> otherNode, const std::vector<int>& iLst);
	std::shared_ptr<InputTrace> distinguished(const std::shared_ptr<FsmNode> otherNode, std::shared_ptr<Tree> w);
    bool rDistinguished(const std::shared_ptr<FsmNode> otherNode, const std::vector<int>& iLst);
    std::shared_ptr<InputTrace> rDistinguished(const std::shared_ptr<FsmNode> otherNode, std::shared_ptr<Tree> w);

	/**
	Calculate a distinguishing input trace for a DFSM node. The algorithm is based
	on Pk-tables
	@param otherNode The other FSM state, to be distinguished from this FSM state
	@param pktblLst  List of Pk-tables, pre-calculated for this DFSM
	@param maxInput  Maximal value of the input alphabet with range 0..maxInput
	@return Distinguishing trace as instance of InputTrace
	*/
	InputTrace calcDistinguishingTrace(const std::shared_ptr<FsmNode> otherNode, const std::vector<std::shared_ptr<PkTable>>& pktblLst, const int maxInput);

	/**
	Calculate a distinguishing input trace for a (potentially nondeterministic)
	FSM node. The algorithm is based on OFSM-tables
	@param otherNode The other FSM state, to be distinguished from this FSM state
	@param ofsmTblLst  List of OFSM-tables, pre-calculated for this FSM
	@param maxInput  Maximal value of the input alphabet with range 0..maxInput
	@param maxOutput Maximal value of the output alphabet in range 0..maxOutput
	@return Distinguishing trace as instance of InputTrace
	*/
	InputTrace calcDistinguishingTrace(const std::shared_ptr<FsmNode> otherNode, const std::vector<std::shared_ptr<OFSMTable>>& ofsmTblLst, const int maxInput, const int maxOutput);
	bool isObservable() const;

	/**
	 * Check if outgoing transitions of this node are deterministic
	 */
	bool isDeterministic() const;
    
    /** 
     *  Mark that this noe is the initial node
     */
    void markAsInitial() { isInitialNode = true; }
    bool isInitial() const { return isInitialNode; }
    
    /**
     * Accept an FsmVisitor, if this node has not been visited already.
     * If the visitor is accepted, this node calls the accept methods of 
     * all transitions.
     */
    void accept(FsmVisitor& v);
    void accept(FsmVisitor& v,
                std::deque< std::shared_ptr<FsmNode> >& bfsq);
    
    /**
     *  Get list of requirements satisified by the node
     */
    std::vector<std::string>& getSatisfied() { return satisfies; }
    void addSatisfies(std::string req) { satisfies.push_back(req); }

    
    
    /** Put node information in dot format into the stream */
	friend std::ostream & operator<<(std::ostream & out, const FsmNode & node);
    
    
	friend bool operator==(FsmNode const & node1, FsmNode const & node2);
};
#endif //FSM_FSM_FSMNODE_H_
