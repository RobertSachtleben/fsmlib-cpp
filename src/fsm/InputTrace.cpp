/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#include "fsm/InputTrace.h"

InputTrace::InputTrace(const std::shared_ptr<FsmPresentationLayer> presentationLayer)
	: Trace(presentationLayer)
{

}

InputTrace::InputTrace(const std::vector<int>& trace, const std::shared_ptr<FsmPresentationLayer> presentationLayer)
	: Trace(trace, presentationLayer)
{

}

InputTrace::InputTrace(const InputTrace& other, size_t n):
    Trace(other.getPresentationLayer())
{
    std::vector<int> otherTrace = other.get();
    if (n > otherTrace.size() - 1)
    {
        n = otherTrace.size() - 1;
    }

    if (n > 0)
    {
        trace = std::vector<int>(otherTrace.begin() + static_cast<std::vector<int>::difference_type>(n), otherTrace.end());
    }
}

std::ostream & operator<<(std::ostream & out, const InputTrace & trace)
{
	for (auto it = trace.cbegin(); it != trace.cend(); ++ it)
	{
		if (it != trace.cbegin())
		{
			out << ".";
		}
        if (*it == -1)
        {
            out << "ε";
        }
        else
        {
            out << trace.presentationLayer->getInId(*it);
        }
	}
	return out;
}
