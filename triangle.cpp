#include "triangle.h"
#include <algorithm>
#include <cmath>

static float edge(const ScreenVertex& a, const ScreenVertex& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

void triangle(Framebuffer& fb, ScreenVertex a, ScreenVertex b, ScreenVertex c, Color color) {
    const float area = edge(a, b, c.x, c.y);
    if (std::abs(area) < 1e-6f) return;
    // Acotamos el rectángulo del triángulo al framebuffer antes de escribir píxeles.
    const int minX = std::max(0, static_cast<int>(std::floor(std::min({a.x,b.x,c.x}))));
    const int maxX = std::min(fb.width()-1, static_cast<int>(std::ceil(std::max({a.x,b.x,c.x}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({a.y,b.y,c.y}))));
    const int maxY = std::min(fb.height()-1, static_cast<int>(std::ceil(std::max({a.y,b.y,c.y}))));
    for (int y=minY; y<=maxY; ++y) for (int x=minX; x<=maxX; ++x) {
        const float px=x+0.5f, py=y+0.5f;
        const float wa=edge(b,c,px,py)/area, wb=edge(c,a,px,py)/area, wc=edge(a,b,px,py)/area;
        if (wa>=-1e-6f && wb>=-1e-6f && wc>=-1e-6f)
            fb.putPixel(x,y,wa*a.z+wb*b.z+wc*c.z,color);
    }
}
