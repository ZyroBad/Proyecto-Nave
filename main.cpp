#include "framebuffer.h"
#include "obj_loader.h"
#include "triangle.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef NAVE_HAS_SDL2
#define NAVE_HAS_SDL2 0
#endif
#if NAVE_HAS_SDL2
#include <SDL.h>
#endif

namespace {
constexpr int W=960, H=720;
const Color background{12,17,29,255};
struct Camera {
    float yaw = std::atan2(-10.0f, 8.0f);
    float pitch = std::atan2(8.0f, std::sqrt(8.0f*8.0f + 10.0f*10.0f));
    float zoom = 1.0f;
};
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 norm(Vec3 a){float n=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return {a.x/n,a.y/n,a.z/n};}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Color materialColor(const std::string& name){
    if(name=="Rojo") return {211,69,67,255};
    if(name=="Gris_oscuro") return {82,101,124,255};
    if(name=="Amarillo") return {246,190,67,255};
    return {160,177,199,255};
}
void append32(std::vector<std::uint8_t>& out,std::uint32_t n){out.push_back(n>>24);out.push_back(n>>16);out.push_back(n>>8);out.push_back(n);}
std::uint32_t crc32(const std::uint8_t* p,std::size_t n){std::uint32_t c=0xffffffffu;for(std::size_t i=0;i<n;++i){c^=p[i];for(int b=0;b<8;++b)c=(c>>1)^((c&1)?0xedb88320u:0);}return c^0xffffffffu;}
void chunk(std::vector<std::uint8_t>& png,const char* type,const std::vector<std::uint8_t>& data){append32(png,static_cast<std::uint32_t>(data.size()));const auto start=png.size();for(int i=0;i<4;++i)png.push_back(static_cast<std::uint8_t>(type[i]));png.insert(png.end(),data.begin(),data.end());append32(png,crc32(png.data()+start,png.size()-start));}
bool savePng(const std::string& path,const Framebuffer& fb){
    std::vector<std::uint8_t> raw;raw.reserve(static_cast<std::size_t>(fb.height())*(1+fb.width()*4));
    for(int y=0;y<fb.height();++y){raw.push_back(0);for(int x=0;x<fb.width();++x){const auto c=fb.pixels()[static_cast<std::size_t>(y*fb.width()+x)];raw.insert(raw.end(),{c.r,c.g,c.b,c.a});}}
    // PNG usa zlib; escribimos bloques DEFLATE sin comprimir para no depender de otra biblioteca.
    std::vector<std::uint8_t> z{0x78,0x01};std::size_t pos=0;while(pos<raw.size()){const std::size_t len=std::min<std::size_t>(65535,raw.size()-pos);const bool last=pos+len==raw.size();z.push_back(last?1:0);z.push_back(len&255);z.push_back(len>>8);const auto inv=static_cast<std::uint16_t>(~len);z.push_back(inv&255);z.push_back(inv>>8);z.insert(z.end(),raw.begin()+pos,raw.begin()+pos+len);pos+=len;}
    std::uint32_t a=1,b=0;for(auto v:raw){a=(a+v)%65521;b=(b+a)%65521;}append32(z,(b<<16)|a);
    std::vector<std::uint8_t> png{137,80,78,71,13,10,26,10};std::vector<std::uint8_t> ihdr;append32(ihdr,fb.width());append32(ihdr,fb.height());ihdr.insert(ihdr.end(),{8,6,0,0,0});chunk(png,"IHDR",ihdr);chunk(png,"IDAT",z);chunk(png,"IEND",{});
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(png.data()),static_cast<std::streamsize>(png.size()));return static_cast<bool>(out);
}
void render(const ObjModel& model,Framebuffer& fb,const Camera& camera){
    const Vec3 target{0,0,0};
    const Vec3 eye{std::cos(camera.pitch)*std::cos(camera.yaw),
                   std::cos(camera.pitch)*std::sin(camera.yaw),
                   std::sin(camera.pitch)};
    const Vec3 forward=norm(sub(target,eye));
    const Vec3 right=norm(cross(forward,{0,0,1})); const Vec3 up=cross(right,forward);
    float minX=1e30f,maxX=-1e30f,minY=1e30f,maxY=-1e30f;
    for(const auto& v:model.vertices){const float x=dot(v,right),y=dot(v,up);minX=std::min(minX,x);maxX=std::max(maxX,x);minY=std::min(minY,y);maxY=std::max(maxY,y);}
    const float scale=std::min((fb.width()-100)/(maxX-minX),(fb.height()-100)/(maxY-minY))*camera.zoom;
    const float cx=(minX+maxX)*0.5f,cy=(minY+maxY)*0.5f;
    auto project=[&](Vec3 p){return ScreenVertex{fb.width()*0.5f+(dot(p,right)-cx)*scale,fb.height()*0.5f-(dot(p,up)-cy)*scale,dot(p,forward)};};
    fb.clear(background);
    // Recorremos cada cara, recuperamos sus tres vértices ya convertidos a índices desde cero y rasterizamos.
    for(const Face& face:model.faces){const Vec3& a=model.vertices[face.vertex[0]];const Vec3& b=model.vertices[face.vertex[1]];const Vec3& c=model.vertices[face.vertex[2]];triangle(fb,project(a),project(b),project(c),materialColor(face.material));}
}
}

int main(int argc,char** argv){
    const bool headless=argc>1 && std::string(argv[1])=="--render";
    const std::string input=argc>2?argv[2]:"assets/nave.obj";
    const std::string output=argc>3?argv[3]:"docs/render.png";
    ObjModel model;std::string error;if(!loadObj(input,model,error)){std::cerr<<error<<'\n';return 1;}
    std::cout<<"Cargados "<<model.vertices.size()<<" vértices y "<<model.faces.size()<<" triángulos.\n";
    Framebuffer fb(W,H,background);Camera camera;render(model,fb,camera);
    if(headless){if(!savePng(output,fb)){std::cerr<<"No se pudo escribir: "<<output<<'\n';return 1;}std::cout<<"Imagen guardada en "<<output<<'\n';return 0;}
#if NAVE_HAS_SDL2
    if(SDL_Init(SDL_INIT_VIDEO)!=0){std::cerr<<"SDL_Init: "<<SDL_GetError()<<'\n';return 1;}
    SDL_Window* window=SDL_CreateWindow("Nave espacial - rasterizador CPU",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if(window) SDL_SetWindowMinimumSize(window,640,480);
    SDL_Renderer* renderer=window?SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC):nullptr;
    SDL_Texture* texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,W,H):nullptr;
    if(!window||!renderer||!texture){std::cerr<<"SDL: "<<SDL_GetError()<<'\n';if(texture)SDL_DestroyTexture(texture);if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);SDL_Quit();return 1;}
    if(SDL_RenderSetLogicalSize(renderer,W,H)!=0){std::cerr<<"SDL_RenderSetLogicalSize: "<<SDL_GetError()<<'\n';SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 1;}
    bool running=true, dragging=false, dirty=true;
    while(running){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) running=false;
            else if(e.type==SDL_KEYDOWN){
                if(e.key.keysym.sym==SDLK_ESCAPE) running=false;
                else if(e.key.keysym.sym==SDLK_r){camera=Camera{};render(model,fb,camera);dirty=true;}
            } else if(e.type==SDL_WINDOWEVENT && (e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED || e.window.event==SDL_WINDOWEVENT_RESTORED || e.window.event==SDL_WINDOWEVENT_MAXIMIZED)) dirty=true;
            else if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT) dragging=true;
            else if(e.type==SDL_MOUSEBUTTONUP && e.button.button==SDL_BUTTON_LEFT) dragging=false;
            else if(e.type==SDL_MOUSEMOTION && dragging){
                camera.yaw+=e.motion.xrel*0.008f;
                camera.pitch=std::clamp(camera.pitch+e.motion.yrel*0.008f,-1.35f,1.35f);
                render(model,fb,camera);dirty=true;
            } else if(e.type==SDL_MOUSEWHEEL){
                const int direction=e.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-1:1;
                camera.zoom=std::clamp(camera.zoom*std::pow(1.12f,static_cast<float>(direction*e.wheel.y)),0.55f,3.0f);
                render(model,fb,camera);dirty=true;
            }
        }
        if(dirty){
            SDL_UpdateTexture(texture,nullptr,fb.pixels().data(),W*sizeof(Color));
            SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,nullptr,nullptr);SDL_RenderPresent(renderer);dirty=false;
        }
        SDL_Delay(8);
    }
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
#else
    std::cerr<<"Este ejecutable no tiene SDL2; usa --render para generar la imagen o instala SDL2 y recompila.\n";return 2;
#endif
    return 0;
}
