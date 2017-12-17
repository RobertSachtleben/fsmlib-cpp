/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_FSM_SEGMENTEDTRACE_H_
#define FSM_FSM_SEGMENTEDTRACE_H_

#include <iostream>
#include <vector>
#include <deque>

#include "fsm/FsmNode.h"

class TraceSegment {
    
private:
    std::shared_ptr< std::vector<int> > segment;
    size_t prefix;
    std::shared_ptr<FsmNode> tgtNode;
    
public:
    
    TraceSegment();
    TraceSegment(std::shared_ptr< std::vector<int> > segment,
                 size_t prefix = std::string::npos,
                 std::shared_ptr<FsmNode> tgtNode = nullptr);
    
    /** Shallow copy */
    TraceSegment(const TraceSegment& other);
    
    void setPrefix(size_t pref);
    
    size_t getPrefix() const { return prefix; }
    
    std::shared_ptr< std::vector<int> > get() { return segment; }
    
    std::vector<int> getCopy();
    
    size_t size();
    
    int at(size_t n);
    
    std::shared_ptr<FsmNode> getTgtNode() { return tgtNode; }
    void setTgtNode(const std::shared_ptr<FsmNode> tgtNode) { this->tgtNode = tgtNode; }
    
};

class SegmentedTrace
{
private:
    
    std::deque< std::shared_ptr<TraceSegment> > segments;
    
public:
    
    SegmentedTrace(std::deque< std::shared_ptr<TraceSegment> > segments);
    SegmentedTrace(const SegmentedTrace& other);
    
    void add(std::shared_ptr<TraceSegment> seg);
    
    std::vector<int> getCopy();
    
    std::shared_ptr<FsmNode> getTgtNode();
    
    size_t size() { return segments.size(); }
    
    std::shared_ptr<TraceSegment> back() {
         return (segments.empty()) ? nullptr : segments.back();
    }
    
    std::shared_ptr<TraceSegment> front() {
        return (segments.empty()) ? nullptr : segments.front();
    }
    
    const std::deque< std::shared_ptr<TraceSegment> >& getSegments() { return segments; }
	 
};
#endif  
