#include "framebuffer.h"
#include <limits>

Framebuffer::Framebuffer(int width, int height, Color clearColor)
    : width_(width), height_(height), pixels_(static_cast<std::size_t>(width * height)),
      depth_(static_cast<std::size_t>(width * height)) { clear(clearColor); }

void Framebuffer::clear(Color color) {
    std::fill(pixels_.begin(), pixels_.end(), color);
    std::fill(depth_.begin(), depth_.end(), std::numeric_limits<float>::infinity());
}

void Framebuffer::putPixel(int x, int y, float depth, Color color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    const std::size_t index = static_cast<std::size_t>(y * width_ + x);
    if (depth < depth_[index]) { depth_[index] = depth; pixels_[index] = color; }
}
