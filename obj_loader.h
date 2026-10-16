#pragma once

#include <string>
#include <vector>

struct Vec3 { float x, y, z; };
struct Face { int vertex[3]; std::string material; };
struct ObjModel { std::vector<Vec3> vertices; std::vector<Face> faces; };
bool loadObj(const std::string& path, ObjModel& model, std::string& error);
