#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#include "cell.hpp"
#include "grid.hpp"
#include "image.hpp"

void display();

int main(int argc, char* argv[])
{   
    srand(time(NULL));
    Grid grid(100);
    if (argc == 2) {
        grid.set_counter_measure(atoi(argv[1]));
    }

    // reproduction_rate,  max_population, migration_pararameter
    grid.set_params(0.78, 5000, 0.2);
    grid.init_present_grid();    
    Image image(100, 100);
    
    int month = 0;
    for (int t = 0; t < 48; ++t)
    {
        month = t % 12;
        grid.get_future_grid(month);
        image.create_image(grid, 1000, month, true);
    }

    return 0;
}

