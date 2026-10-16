#pragma once

#include "color.h"
#include <vector>

class Framebuffer {
public:
    Framebuffer(int width, int height, Color clearColor);
    void clear(Color color);
    void putPixel(int x, int y, float depth, Color color);
    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<Color>& pixels() const { return pixels_; }
private:
    int width_, height_;
    std::vector<Color> pixels_;
    std::vector<float> depth_;
};
