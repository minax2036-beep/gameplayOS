#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <linux/fb.h>
#include <linux/input.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_INPUTS 16
#define CATEGORIES 6
#define ITEMS 4
#define FRAME_MS 50

struct fb {
    int fd;
    uint8_t *mem;
    size_t len;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    unsigned bpp;
};
struct font_set { FT_Library lib; FT_Face latin, cjk; };

static const char *category[CATEGORIES] =
    {"GAME","NETWORK","MUSIC","VIDEO","SETTINGS","SYSTEM"};
static const char *item[CATEGORIES][ITEMS] = {
    {"GAMES","MEMORY CARD","GAME DATA","GAME OPTIONS"},
    {"NETWORK STATUS","DOWNLOADS","NETWORK OPTIONS","CONNECTION INFO"},
    {"MUSIC PLAYER","MUSIC FILES","PLAYLISTS","AUDIO OPTIONS"},
    {"VIDEO PLAYER","VIDEO FILES","VIDEO OPTIONS","DISPLAY INFO"},
    {"SYSTEM SETTINGS","DISPLAY SETTINGS","SOUND SETTINGS","INPUT SETTINGS"},
    {"SYSTEM INFORMATION","STORAGE INFORMATION","RESTART","ABOUT GAMEPLAYOS"}
};

static uint32_t channel(uint8_t v, struct fb_bitfield f)
{
    if (!f.length) return 0;
    return (((uint32_t)v * ((1u << f.length) - 1u) + 127u) / 255u) << f.offset;
}
static uint32_t pack(struct fb *s, uint8_t r, uint8_t g, uint8_t b)
{
    return channel(r,s->var.red)|channel(g,s->var.green)|
           channel(b,s->var.blue)|channel(255,s->var.transp);
}
static size_t offset_of(struct fb *s,int x,int y)
{
    return (size_t)(y+s->var.yoffset)*s->fix.line_length+
           (size_t)(x+s->var.xoffset)*s->bpp;
}
static void pixel(struct fb *s,int x,int y,uint8_t r,uint8_t g,uint8_t b)
{
    size_t o; uint32_t v; uint8_t *p;
    if(x<0||y<0||x>=(int)s->var.xres||y>=(int)s->var.yres)return;
    o=offset_of(s,x,y); if(o+s->bpp>s->len)return;
    v=pack(s,r,g,b); p=s->mem+o;
    p[0]=(uint8_t)v; if(s->bpp>1)p[1]=(uint8_t)(v>>8);
    if(s->bpp>2)p[2]=(uint8_t)(v>>16); if(s->bpp>3)p[3]=(uint8_t)(v>>24);
}
static void blend(struct fb *s,int x,int y,uint8_t r,uint8_t g,uint8_t b,uint8_t a)
{
    size_t o; uint32_t old,v; uint8_t *p;
    unsigned rr,gg,bb;
    if(!a||x<0||y<0||x>=(int)s->var.xres||y>=(int)s->var.yres)return;
    o=offset_of(s,x,y); if(o+s->bpp>s->len)return;
    p=s->mem+o; old=p[0];
    if(s->bpp>1)old|=(uint32_t)p[1]<<8;
    if(s->bpp>2)old|=(uint32_t)p[2]<<16;
    if(s->bpp>3)old|=(uint32_t)p[3]<<24;
    rr=s->var.red.length?(old>>s->var.red.offset)&((1u<<s->var.red.length)-1u):0;
    gg=s->var.green.length?(old>>s->var.green.offset)&((1u<<s->var.green.length)-1u):0;
    bb=s->var.blue.length?(old>>s->var.blue.offset)&((1u<<s->var.blue.length)-1u):0;
    if(s->var.red.length)rr=rr*255u/((1u<<s->var.red.length)-1u);
    if(s->var.green.length)gg=gg*255u/((1u<<s->var.green.length)-1u);
    if(s->var.blue.length)bb=bb*255u/((1u<<s->var.blue.length)-1u);
    r=(uint8_t)((r*a+rr*(255u-a))/255u);
    g=(uint8_t)((g*a+gg*(255u-a))/255u);
    b=(uint8_t)((b*a+bb*(255u-a))/255u);
    v=pack(s,r,g,b); p[0]=(uint8_t)v;
    if(s->bpp>1)p[1]=(uint8_t)(v>>8);
    if(s->bpp>2)p[2]=(uint8_t)(v>>16);
    if(s->bpp>3)p[3]=(uint8_t)(v>>24);
}
static void rect(struct fb *s,int x,int y,int w,int h,uint8_t r,uint8_t g,uint8_t b)
{
    int xx,yy,x2=x+w,y2=y+h;
    if(x<0)x=0;if(y<0)y=0;
    if(x2>(int)s->var.xres)x2=(int)s->var.xres;
    if(y2>(int)s->var.yres)y2=(int)s->var.yres;
    for(yy=y;yy<y2;yy++)for(xx=x;xx<x2;xx++)pixel(s,xx,yy,r,g,b);
}
static uint32_t utf8(const unsigned char **p)
{
    uint32_t c=*(*p)++;
    if(c<128)return c;
    if((c&224)==192){c=(c&31)<<6;c|=*(*p)++&63;return c;}
    if((c&240)==224){c=(c&15)<<12;c|=(*(*p)++&63)<<6;c|=*(*p)++&63;return c;}
    if((c&248)==240){c=(c&7)<<18;c|=(*(*p)++&63)<<12;c|=(*(*p)++&63)<<6;c|=*(*p)++&63;return c;}
    return '?';
}
static int fonts_open(struct font_set *f)
{
    memset(f,0,sizeof(*f));
    if(FT_Init_FreeType(&f->lib))return -1;
    if(FT_New_Face(f->lib,"/usr/share/fonts/dejavu/DejaVuSans.ttf",0,&f->latin))
        f->latin=NULL;
    if(FT_New_Face(f->lib,"/usr/share/fonts/wqy-zenhei/wqy-zenhei.ttc",0,&f->cjk))
        f->cjk=NULL;
    if(!f->latin&&!f->cjk){FT_Done_FreeType(f->lib);return -1;}
    return 0;
}
static void text(struct fb *s,struct font_set *f,int x,int baseline,
                 const char *str,int size,uint8_t r,uint8_t g,uint8_t b)
{
    const unsigned char *p=(const unsigned char *)str;
    while(*p){
        uint32_t cp=utf8(&p); FT_Face face=(cp>127&&f->cjk)?f->cjk:f->latin;
        FT_UInt glyph; int xx,yy;
        if(!face)face=f->cjk?f->cjk:f->latin;
        if(!face)continue;
        FT_Set_Pixel_Sizes(face,0,(FT_UInt)size);
        glyph=FT_Get_Char_Index(face,cp);
        if(!glyph&&f->cjk&&face!=f->cjk){
            face=f->cjk;FT_Set_Pixel_Sizes(face,0,(FT_UInt)size);
            glyph=FT_Get_Char_Index(face,cp);
        }
        if(FT_Load_Glyph(face,glyph,FT_LOAD_DEFAULT)||
           FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL))continue;
        {
            FT_Bitmap *bm=&face->glyph->bitmap;
            int left=x+face->glyph->bitmap_left, top=baseline-face->glyph->bitmap_top;
            for(yy=0;yy<(int)bm->rows;yy++){
                const unsigned char *row=bm->buffer+
                    (bm->pitch>=0?yy*bm->pitch:(bm->rows-1-yy)*(-bm->pitch));
                for(xx=0;xx<(int)bm->width;xx++)
                    if(row[xx])blend(s,left+xx,top+yy,r,g,b,row[xx]);
            }
        }
        x+=(int)(face->glyph->advance.x>>6);
    }
}
static void circle(struct fb *s,int cx,int cy,int radius,uint8_t r,uint8_t g,uint8_t b)
{
    int x,y;for(y=-radius;y<=radius;y++)for(x=-radius;x<=radius;x++)
        if(x*x+y*y<=radius*radius)pixel(s,cx+x,cy+y,r,g,b);
}
/* Smooth, slowly drifting layered curves, intentionally low-CPU. */
static int wave_y(int x,unsigned phase,int layer)
{
    unsigned t=((unsigned)x/(18u+(unsigned)layer*5u)+phase/(3u+(unsigned)layer))&127u;
    int tri=t<32u?(int)t:t<64u?64-(int)t:t<96u?-(int)(t-64u):-(int)(128u-t);
    return tri;
}
static void wave(struct fb *s,int base,unsigned phase,int layer)
{
    int x,prev=base+wave_y(0,phase,layer)*(8+layer*2)/32;
    for(x=1;x<(int)s->var.xres;x++){
        int y=base+wave_y(x,phase,layer)*(8+layer*2)/32;
        int lo=y<prev?y:prev,hi=y>prev?y:prev,yy;
        for(yy=lo;yy<=hi;yy++)blend(s,x,yy,35+layer*5,105+layer*12,205,100);
        prev=y;
    }
}
static void background(struct fb *s,unsigned frame)
{
    int y,x,w=(int)s->var.xres,h=(int)s->var.yres;
    for(y=0;y<h;y++){
        uint8_t r=(uint8_t)(4+y*5/(h?h:1));
        uint8_t g=(uint8_t)(10+y*17/(h?h:1));
        uint8_t b=(uint8_t)(43+y*54/(h?h:1));
        rect(s,0,y,w,1,r,g,b);
    }
    for(x=0;x<5;x++)wave(s,h-70-x*(h/20),frame*(unsigned)(1+x/2),x);
}
static void render(struct fb *s,struct font_set *f,int cat,int sel,unsigned frame)
{
    int w=(int)s->var.xres,h=(int)s->var.yres,i,top,left,row,margin;
    int scale=w>=1280?2:1, title=w>=1280?36:29, body=w>=1280?23:18;
    margin=w>=1280?52:30;
    background(s,frame);
    text(s,f,margin,45,"GAMEPLAYOS",title,248,251,255);
    text(s,f,margin+2,75,"XMB  /  CONSOLE EXPERIENCE",13*scale,120,200,248);
    rect(s,margin,88,w-2*margin,1,75,145,205);
    top=h>=700?175:112;left=w/12;
    for(i=0;i<CATEGORIES;i++){
        int cx=left+i*(w-2*left)/(CATEGORIES-1),rad=i==cat?21*scale:14*scale;
        circle(s,cx,top,rad+5,24,86,165);
        circle(s,cx,top,rad,i==cat?75:60,i==cat?210:135,i==cat?250:190);
        circle(s,cx,top,6*scale,235,250,255);
        text(s,f,cx-(int)strlen(category[i])*5*scale,
             top+rad+25*scale,category[i],13*scale,
             i==cat?250:165,i==cat?253:205,255);
    }
    rect(s,margin,h>=700?265:160,w-2*margin,1,65,125,180);
    text(s,f,margin+24,h>=700?300:190,category[cat],22*scale,190,232,255);
    row=h>=700?76:52;top=h>=700?340:222;
    for(i=0;i<ITEMS;i++){
        int yy=top+i*row;
        if(i==sel){
            rect(s,margin+8,yy-11,w-2*(margin+8),row-4,18,82,151);
            rect(s,margin+8,yy-11,5,row-4,100,225,255);
            text(s,f,margin+31,yy+body,item[cat][i],body+1,255,255,255);
        }else text(s,f,margin+31,yy+body,item[cat][i],body,190,218,242);
    }
    rect(s,margin,h-48,w-2*margin,1,65,125,180);
    text(s,f,margin+2,h-25,"ARROWS  SELECT",13*scale,145,190,225);
    text(s,f,w-margin-130*scale,h-25,"GAMEPLAYOS",13*scale,145,190,225);
}
static int fb_open(struct fb *s)
{
    memset(s,0,sizeof(*s));s->fd=open("/dev/fb0",O_RDWR);if(s->fd<0)return -1;
    if(ioctl(s->fd,FBIOGET_FSCREENINFO,&s->fix)<0||
       ioctl(s->fd,FBIOGET_VSCREENINFO,&s->var)<0)return -1;
    s->bpp=(s->var.bits_per_pixel+7u)/8u;
    if((s->var.bits_per_pixel!=16&&s->var.bits_per_pixel!=24&&s->var.bits_per_pixel!=32)||!s->fix.smem_len)return -1;
    s->len=s->fix.smem_len;s->mem=mmap(NULL,s->len,PROT_READ|PROT_WRITE,MAP_SHARED,s->fd,0);
    if(s->mem==MAP_FAILED){s->mem=NULL;return -1;}return 0;
}
int main(void)
{
    struct fb screen;struct font_set fonts;struct pollfd pf[MAX_INPUTS];
    int fd[MAX_INPUTS],n=0,i,cat=0,sel=0;unsigned frame=0;
    if(fb_open(&screen)<0)for(;;)pause();
    if(fonts_open(&fonts)<0)for(;;)pause();
    for(i=0;i<MAX_INPUTS;i++){
        char path[32];snprintf(path,sizeof(path),"/dev/input/event%d",i);
        fd[i]=open(path,O_RDONLY|O_NONBLOCK);
        if(fd[i]>=0){pf[n].fd=fd[i];pf[n].events=POLLIN;pf[n].revents=0;n++;}
    }
    render(&screen,&fonts,cat,sel,frame);
    for(;;){
        int changed=0,res=poll(pf,(nfds_t)n,FRAME_MS);
        if(res>0)for(i=0;i<n;i++)if(pf[i].revents&POLLIN){
            struct input_event ev;
            while(read(pf[i].fd,&ev,sizeof(ev))==(ssize_t)sizeof(ev)){
                if(ev.type!=EV_KEY||ev.value!=1)continue;
                switch(ev.code){
                case KEY_LEFT:cat=(cat+CATEGORIES-1)%CATEGORIES;sel=0;changed=1;break;
                case KEY_RIGHT:cat=(cat+1)%CATEGORIES;sel=0;changed=1;break;
                case KEY_UP:sel=(sel+ITEMS-1)%ITEMS;changed=1;break;
                case KEY_DOWN:sel=(sel+1)%ITEMS;changed=1;break;
                case KEY_ENTER:case KEY_KPENTER:changed=1;break;
                default:break;
                }
            }
        }
        frame++;
        if(changed||(frame%2u)==0u)render(&screen,&fonts,cat,sel,frame);
    }
}
