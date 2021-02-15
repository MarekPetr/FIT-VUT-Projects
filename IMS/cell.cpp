#include "cell.hpp"

void Cell::updateDensity(double state) {
    this->state = state;
}

void Cell::setCell(int x, int y, double state) {
    this->state = state;
    this->x = x;
    this->y = y;
}