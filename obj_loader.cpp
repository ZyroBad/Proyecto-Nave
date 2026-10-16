#include "obj_loader.h"
#include <fstream>
#include <sstream>

static bool parseVertexIndex(const std::string& token, std::size_t count, int& index) {
    const auto slash = token.find('/');
    const std::string part = token.substr(0, slash);
    try {
        std::size_t used = 0; const int raw = std::stoi(part, &used);
        if (used != part.size() || raw == 0) return false;
        const long resolved = raw > 0 ? raw - 1 : static_cast<long>(count) + raw;
        if (resolved < 0 || resolved >= static_cast<long>(count)) return false;
        index = static_cast<int>(resolved); return true;
    } catch (...) { return false; }
}

bool loadObj(const std::string& path, ObjModel& model, std::string& error) {
    std::ifstream in(path); if (!in) { error="No se pudo abrir el OBJ: "+path; return false; }
    ObjModel parsed; std::string line, material; int lineNumber=0;
    while (std::getline(in,line)) {
        ++lineNumber; std::istringstream row(line); std::string tag; row>>tag;
        if (tag=="v") { Vec3 v{}; if (!(row>>v.x>>v.y>>v.z)) { error="Vértice inválido en línea "+std::to_string(lineNumber); return false; } parsed.vertices.push_back(v); }
        else if (tag=="usemtl") row>>material;
        else if (tag=="f") {
            std::vector<std::string> refs; std::string ref; while (row>>ref) refs.push_back(ref);
            if (refs.size()!=3) { error="Se esperaba una cara triangular en línea "+std::to_string(lineNumber); return false; }
            Face face{}; face.material=material;
            // OBJ numera desde 1: convertimos a vector desde 0; los índices negativos son relativos al final.
            for (int i=0;i<3;++i) if (!parseVertexIndex(refs[i],parsed.vertices.size(),face.vertex[i])) {
                error="Índice de vértice inválido ('"+refs[i]+"') en línea "+std::to_string(lineNumber); return false;
            }
            parsed.faces.push_back(face);
        }
    }
    if (parsed.vertices.empty() || parsed.faces.empty()) { error="El OBJ no contiene vértices o caras"; return false; }
    model=std::move(parsed); return true;
}
