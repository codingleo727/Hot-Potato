#pragma once
#ifndef POTATO_HPP
#define POTATO_HPP

class Potato {
public:
    Potato() = default;
    ~Potato() = default;
    Potato(int hops);

    /**
     * Get the number of hops remaining in the potato. 
     * @return the number of hops remaining in the potato, or -1 if the potato is invalid
     */
    int getHops() const;

    /**
     * Decrement the number of hops remaining in the potato by 1, if the number of hops is greater than 0. 
     * If the number of hops is already 0 or negative, this function does nothing.
     */
    void decrementHops();

    /**
     * Add the given player ID to the trace of the potato, which is a sequence of player IDs representing the path the potato has taken through the players. 
     * The player ID should be added to the end of the trace, and the trace length should be incremented by 1. 
     * If the trace is already at its maximum length of 512, this function should not add the player ID and should not increment the trace length.
     * @param playerId the ID of the player to add to the trace of the potato
     */
    void addTrace(int playerId);
    /**
     * Get a pointer to the trace array of the potato, which is an array of player IDs representing the path the potato has taken through the players. 
     * The trace should be in the order that the player IDs were added to the trace, with the first player ID at index 0 
     * and the most recently added player ID at index traceLength - 1.
     * @return a pointer to the trace array of the potato, or nullptr if the potato is invalid
     */
    const int * getTrace() const;

    /**
     * Get the length of the trace of the potato, which is the number of player IDs currently stored in the trace array. 
     * The trace length should be equal to the number of times addTrace() has been called on the potato, and should be less than or equal to 512.
     * @return the length of the trace of the potato, or -1 if the potato
     */
    int getTraceLength() const;
private:
    int hops = 0;
    int trace[512] = {};
    int traceLength = 0;
};
#endif