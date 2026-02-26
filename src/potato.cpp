#include "potato.hpp"
#include <cstring>

Potato::Potato(int hops) : hops(hops) {
    std::memset(trace, 0, sizeof(trace)); // Initialize trace array to 0
    traceLength = 0;
}

int Potato::getHops() const {
    return hops;
}

void Potato::decrementHops() {
    if (hops > 0) {
        hops--;
    }
}

void Potato::addTrace(int playerId) {
    if (traceLength < 512) {
        trace[traceLength++] = playerId;
    }
}

const int * Potato::getTrace() const {
    return trace;
}

int Potato::getTraceLength() const {
    return traceLength;
}