/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_FSM_PKTABLEROW_H_
#define FSM_FSM_PKTABLEROW_H_

#include <iostream>

#include "fsm/typedef.inc"

class PkTableRow
{
private:
    // Pointer to I2O map (originating from the DFSM table)
    IOMap& io;
    
    // Pointer to I2P map
    I2PMap& i2p;
public:
    
    /**
     * Create a new PK table, using the references to existing
     * I2O and I2P tables
     */
    PkTableRow(IOMap& io, I2PMap& i2p);
    
    /** Return reference to I2O table */
    IOMap& getIOMap() const;
    
    /** Return reference to I2P table */
    I2PMap& getI2PMap() const;
    
    /**
     * Return the post state number registered in this PK table row
     * which is reached on input x
     */
    int get(const int x) const;
    
    /**
     * Check whether the I2P-part of this PkTableRow equals 
     * the I2P-part of another row, modulo equivalence of states
     * as defined according to the current s2c-map
     */
    bool isEquivalent(const PkTableRow &row, const S2CMap &s2c);
    
    /**
     * Output the PkTableRow to a standard output stream, using 
     * LaTeX tabular format.
     * @param out The standard output stream to use
     * @param pkTableRow The PkTableRow to print
     * @return The standard output stream used, to allow user to cascade <<
     */
    friend std::ostream & operator<<(std::ostream & out, const PkTableRow & pkTableRow);
};
#endif //FSM_FSM_PKTABLEROW_H_
