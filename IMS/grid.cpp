#include "grid.hpp"
#include <math.h>

void Grid::set_counter_measure(int counter_measure) {
    this->counter_measure = counter_measure;
}

void Grid::set_params(
    double reproduction_rate, double max_population, double migration_param) {
    this->fertility = (1 - reproduction_rate);
    this->mortality = (-reproduction_rate/max_population);
    this->max_population = max_population;
    this->migration_param = migration_param;
}

double Grid::random_double(double min, double max)
{
    double r = (double)rand()/(double)RAND_MAX+1.0;
    return min + r * (max - min);
}

void Grid::fill_chunk_of_present_grid(int xmin, int ymin, int width, double value) {
    for (int y = ymin; y < width+ymin; ++y)
    {
        for (int x = xmin; x < width+xmin; ++x)
        {
            this->present_grid[order_from_coords(x, y)].state = value;
        }
    }
}

void Grid::init_present_grid() {
    for (int y = 0; y < this->width; ++y)
    {
        for (int x = 0; x < this->width; ++x)
        {
            // creates Cell and appends it behind the last Cell
            this->present_grid.emplace_back(x,y,0);
        }
    }
    int randx = 0;
    int randy = 0;
    int rand_cnt = 0;
    double rand_dens;
    for (int i = 0; i < 40; ++i)
    {   
        rand_cnt = rand() % 20;
        randx = rand() % (this->width-rand_cnt);
        randy = rand() % (this->width-rand_cnt);
        
        rand_dens = random_double(0.07, 0.1);
        this->fill_chunk_of_present_grid(randx,randy,rand_cnt,rand_dens);
    }
}

Cell Grid::get_present_cell(int x, int y) {
    return this->present_grid[order_from_coords(x, y)];
}

Cell Grid::get_future_cell(int x, int y) {
    return this->future_grid[order_from_coords(x, y)];
}

int Grid::order_from_coords(int x, int y) {
    return x + (y*this->width);
}

void Grid::print_present_grid() {
    for (int y = 0; y < this->width; ++y)
    {
        for (int x = 0; x < this->width; ++x)
        {
            printf("%.02f ", this->present_grid[order_from_coords(x, y)].state);
        }
        cout << endl;
    }
    cout << endl;
}

void Grid::get_future_grid(int month) {
    double state = 0;
    double new_state = 0;
    int order = 0;
    double winter_coef = 0.85;
    double deep_plow_coef = 0.5;
    double shallow_plow_coef = 0.80;
    double stutox_coef = 0.20;

    /* Set winter for the year
       Months starts at 3 (March) - start of reproductive phase
    */

    if (month > 7) {
        int winter_intensity = (int) (1 + 3 * (rand() / RAND_MAX+1.0));
        if (winter_intensity == 1){ //weak winter
            winter_coef = 0.85;
        }
        else if(winter_intensity == 2){ //medium strong winter
            winter_coef = 0.80;
        }
        else{ // strong winter
            winter_coef = 0.75;
        }
    }

    for (int y = 0; y < this->width; ++y)
    {
        for (int x = 0; x < this->width; ++x)
        {
            order = order_from_coords(x,y);
            state = present_grid[order].state;

            // 7 - October
            if (month >= 7) {
                new_state = state +
                this->mortality*pow(state,2) +
                this->migration_param*diffusion_operator(x,y);
                if (month > 7) {
                    new_state *= winter_coef;
                    if (month == 7 && this->counter_measure == DEEP_PLOW) {
                        new_state *= deep_plow_coef;
                    }
                }
            }
            else {
                new_state = state + this->fertility*state +
                this->mortality*pow(state,2) +
                this->migration_param*diffusion_operator(x,y);
                // 2 - April
                if (month == 2 && this->counter_measure == SHALLOW_PLOW) {
                    new_state *= shallow_plow_coef;
                }
                // 5 - August
                if (month == 3 && this->counter_measure == STUTOX) {
                    new_state *= stutox_coef;
                }
            }
             
            if (new_state < 0) {
                new_state = 0;
            }
            this->future_grid[order].state = new_state;
        }
    }
    copy_future_to_present_grid();
}

void Grid::copy_future_to_present_grid() {
    for (int y = 0; y < this->width; ++y)
    {
        for (int x = 0; x < this->width; ++x)
        {
            this->present_grid[order_from_coords(x, y)].state =
            this->future_grid[order_from_coords(x, y)].state;
        }
    }
}

double Grid::get_average() {
    double sum = 0.0;
    for (int y = 0; y < this->width; ++y)
    {
        for (int x = 0; x < this->width; ++x)
        {
            sum += this->present_grid[order_from_coords(x, y)].state;
        }
    }
    double area = this->width*this->width;
    return sum/area;
}

double Grid::diffusion_operator(int x, int y) {
    double diffusion_operator = 0.0;
    unsigned int count = 0;
    
    if (x > 0) {
        diffusion_operator += get_present_cell(x-1, y).state;
        count++;
    }
    
    if (x < this->max_idx) {
        diffusion_operator += get_present_cell(x+1, y).state;
        count++;
    }
    
    if (y > 0) {
        diffusion_operator += get_present_cell(x, y-1).state;
        count++;
    }
    
    if (y < this->max_idx) {
        count++;
        diffusion_operator += get_present_cell(x, y+1).state;
    }

    diffusion_operator -= (count*get_present_cell(x, y).state);
    return diffusion_operator;
}
