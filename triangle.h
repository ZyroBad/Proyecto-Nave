#pragma once

#include "color.h"
#include "framebuffer.h"

struct ScreenVertex { float x, y, z; };
void triangle(Framebuffer& framebuffer, ScreenVertex a, ScreenVertex b, ScreenVertex c, Color color);
