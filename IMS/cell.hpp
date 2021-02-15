#ifndef CELL_HPP
#define CELL_HPP

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

class Cell {
public:
    Cell(int x, int y, double state) {
        setCell(x,y,state);
    }
    Cell(void) {
        setCell(0,0,0);
    }
    void updateDensity(double state);
    void setCell(int x, int y, double state);
    double state;
    int x;
    int y;
};

#endif