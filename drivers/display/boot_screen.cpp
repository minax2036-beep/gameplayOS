#include "boot_screen.hpp"
namespace gameplayos::display {
namespace {
struct Colour { std::uint8_t r, g, b; };
constexpr Colour BLACK{0,0,0}, WHITE{238,238,238}, GREY{170,170,170};
constexpr Colour YELLOW{190,190,0}, CYAN{0,175,175}, GREEN{0,175,0};
constexpr Colour MAGENTA{175,0,175}, RED{175,0,0}, BLUE{0,0,175};
constexpr std::uint32_t LOG_HEIGHT=120, SCALE=2, CHAR_WIDTH=12, LINE_HEIGHT=18;
constexpr char CHARS[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_:./[]() ";
constexpr std::uint8_t FONT[][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
{14,17,17,15,1,1,14},{0,0,0,31,0,0,0},{0,4,4,0,4,4,0},
{0,4,4,0,4,4,8},{0,0,0,0,0,4,8},{1,2,2,4,8,8,16},
{14,8,8,8,8,8,14},{14,2,2,2,2,2,14},{4,10,17,17,17,10,4},
{4,10,17,17,17,10,4},{0,0,0,0,0,0,0}
};
void rect(const Framebuffer& f,std::uint32_t x,std::uint32_t y,std::uint32_t w,std::uint32_t h,Colour c){
 const std::uint32_t right=(x>f.width||w>f.width-x)?f.width:x+w;
 const std::uint32_t bottom=(y>f.height||h>f.height-y)?f.height:y+h;
 for(std::uint32_t py=y;py<bottom;++py) for(std::uint32_t px=x;px<right;++px) put_pixel(f,px,py,c.r,c.g,c.b);
}
const std::uint8_t* glyph(char ch){
 if(ch>='a'&&ch<='z') ch=static_cast<char>(ch-'a'+'A');
 for(std::size_t i=0;i<sizeof(CHARS)-1;++i) if(CHARS[i]==ch) return FONT[i];
 return FONT[sizeof(CHARS)-2];
}
void text(const Framebuffer& f,std::uint32_t x,std::uint32_t y,const char* s,Colour c){
 if(!s)return;
 for(std::size_t i=0;s[i];++i){
  const auto* g=glyph(s[i]); const std::uint32_t ox=x+static_cast<std::uint32_t>(i)*CHAR_WIDTH;
  for(std::uint32_t row=0;row<7;++row) for(std::uint32_t col=0;col<5;++col)
   if(g[row]&(1u<<(4u-col))) rect(f,ox+col*SCALE,y+row*SCALE,SCALE,SCALE,c);
 }
}
Colour level_colour(LogLevel l){
 switch(l){case LogLevel::Success:return {0,220,100};case LogLevel::Warning:return {255,205,40};case LogLevel::Error:return {255,75,75};default:return WHITE;}
}
const char* level_name(LogLevel l){
 switch(l){case LogLevel::Success:return "OK";case LogLevel::Warning:return "WARN";case LogLevel::Error:return "ERROR";default:return "INFO";}
}
}
bool draw_boot_test_screen(const Framebuffer& f){
 if(!is_valid(f)||f.width<7||f.height<=LOG_HEIGHT)return false;
 const std::uint32_t ph=f.height-LOG_HEIGHT, h1=ph*70/100, h2=ph*9/100, h3=ph-h1-h2;
 constexpr Colour top[]={GREY,YELLOW,CYAN,GREEN,MAGENTA,RED,BLUE};
 constexpr Colour mid[]={BLUE,BLACK,MAGENTA,BLACK,CYAN,BLACK,GREY};
 constexpr Colour low[]={{8,28,58},WHITE,{38,0,68},{16,16,16},{8,8,8},{24,24,24},{16,16,16}};
 for(std::uint32_t i=0;i<7;++i){
  const std::uint32_t x0=f.width*i/7,x1=f.width*(i+1)/7,w=x1-x0;
  rect(f,x0,0,w,h1,top[i]); rect(f,x0,h1,w,h2,mid[i]); rect(f,x0,h1+h2,w,h3,low[i]);
 }
 rect(f,0,ph,f.width,LOG_HEIGHT,BLACK);
 return draw_boot_log(f,nullptr,0);
}
bool draw_boot_log(const Framebuffer& f,const BootLogEntry* entries,std::size_t count){
 if(!is_valid(f)||f.height<=LOG_HEIGHT)return false;
 if(count>0&&entries==nullptr)return false;
 const std::uint32_t top=f.height-LOG_HEIGHT;
 rect(f,0,top,f.width,LOG_HEIGHT,BLACK);
 text(f,12,top+7,"GAMEPLAYOS BOOT LOG",{0,220,130});
 if(!entries)return true;
 constexpr std::size_t MAX_LINES=5;
 const std::size_t first=count>MAX_LINES?count-MAX_LINES:0;
 for(std::size_t i=0;i<count-first;++i){
  const auto& e=entries[first+i]; const std::uint32_t y=top+25+static_cast<std::uint32_t>(i)*LINE_HEIGHT;
  text(f,12,y,"[",GREY); text(f,24,y,level_name(e.level),level_colour(e.level));
  text(f,84,y,"]",GREY); text(f,102,y,e.message,level_colour(e.level));
 }
 return true;
}
} // namespace gameplayos::display
