#include "image.hpp"

Image::Image(unsigned long width, unsigned long height) {
    this->imgSize = Size2d(width*GRID_INC+GRID_INC, height*GRID_INC+GRID_INC);
    this->width = width;
    this->height = height;
}

void Image::create_pixel(Mat img, Point pixel, Scalar color) {
    int x = pixel.x*GRID_INC;
    int y = pixel.y*GRID_INC;
    for (int i = 0; i < GRID_INC; ++i)
    {
        line(img, Point(x,y+i), Point(x+GRID_INC, y+i), color);
    }
}

// round double to two decimal places
double Image::round(double var)
{
    double value = (int)(var * 100 + .5);
    return (double)value / 100;
} 

void Image::create_image(Grid grid, int waitKeyTime, int month, bool showWindow = false) {
    Mat image = Mat(this->imgSize.height, this->imgSize.width, CV_8UC3, Scalar(255, 255, 255));
    //this->createGrid(image);

    for(int y = 0; y <= grid.width; y++) //for row in matrix
    {
        for(int x = 0; x <= grid.width; x++) //for col in matrix
        {   
            double state = round(grid.present_grid[grid.order_from_coords(x, y)].state);
            if(state <= 0.02) {
                this->create_pixel(image, Point(y, x), CELL_0);
            }
            else if(state <=0.05) {
                this->create_pixel(image, Point(y, x), CELL_1);
            }
            else if(state <= 0.10) {
                this->create_pixel(image, Point(y, x), CELL_2);
            }
            else if (state <= 0.20) {
                this->create_pixel(image, Point(y, x), CELL_3);;
            }
            else if (state <= 0.30) {
                this->create_pixel(image, Point(y, x), CELL_4);
            }
            else if (state <= 0.40) {
                this->create_pixel(image, Point(y, x), CELL_5);
            }
            else if (state <= 0.50) {
                this->create_pixel(image, Point(y, x), CELL_6);
            }
            else
            {
                this->create_pixel(image, Point(y, x), CELL_7);
            }
        }
    }

    if (showWindow) {
        int real_month = ((month+2) % 12)+1;
        string counter_measure = "";
        if (grid.counter_measure == DEEP_PLOW && (real_month == 10 || real_month == 11)) {
            counter_measure = " Hluboka orba";
        } else if (grid.counter_measure == SHALLOW_PLOW && (real_month == 4 || real_month == 5)){
            counter_measure = " Melka orba";
        } else if (grid.counter_measure == STUTOX && (real_month == 5 || real_month == 6)){
            counter_measure = " Stutox";
        }
        string name = "Mesic " + std::to_string(real_month) + counter_measure;

        putText(image, name, Point2f(20,20), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255, 255));
        namedWindow("Cellular automata 1ha", CV_WINDOW_AUTOSIZE);
        imshow("Cellular automata 1ha", image);
    }
    waitKey(waitKeyTime);
}
