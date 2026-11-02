#include "popover.h"
#include <stdio.h>
#include <string.h>
static int text_width(XFontStruct *font,const char *text,int length) {
    return font?XTextWidth(font,text,length):length*6;
}
static void line(Display *d,Drawable drawable,GC gc,XFontStruct *font,int x,int y,int width,const char *text) {
    int length=(int)strlen(text);
    while(length>0 && text_width(font,text,length)>width)length--;
    if(length)XDrawString(d,drawable,gc,x,y,text,length);
}
static void description(Display *d,Drawable drawable,GC gc,XFontStruct *font,int width,const char *text) {
    int row=0;
    while(*text&&row<3) {
        int n=0,last_space=0;
        while(text[n]&&text_width(font,text,n+1)<=width) {if(text[n]==' ')last_space=n;n++;}
        if(n==0)break;
        if(text[n]&&last_space)n=last_space;
        XDrawString(d,drawable,gc,12,42+row*14,text,n);
        text+=n;while(*text==' ')text++;row++;
    }
}
void popover_draw(Popover *popover,Display *d,GC gc,XFontStruct *font,const Places *places) {
    const Place *p;char buffer[160];int width=(int)popover->width,height=(int)popover->height;
    if(!popover->visible||popover->selected<0||(size_t)popover->selected>=places->count)return;
    p=places->items+popover->selected;
    XSetForeground(d,gc,BlackPixel(d,DefaultScreen(d)));
    XFillRectangle(d,popover->backing,gc,0,0,popover->width,popover->height);
    XSetForeground(d,gc,WhitePixel(d,DefaultScreen(d)));
    line(d,popover->backing,gc,font,12,20,width-48,p->label);
    line(d,popover->backing,gc,font,width-20,20,18,"x");
    if(width<260||height<228) {
        line(d,popover->backing,gc,font,12,44,width-24,"Enlarge the window for place details.");
        XCopyArea(d,popover->backing,popover->window,gc,0,0,popover->width,popover->height,0,0);
        return;
    }
    description(d,popover->backing,gc,font,width-24,p->description);
    snprintf(buffer,sizeof(buffer),"Population: %s",p->population);
    line(d,popover->backing,gc,font,12,96,width-24,buffer);
    snprintf(buffer,sizeof(buffer),"Location: %.4f %c, %.4f %c",p->latitude<0?-p->latitude:p->latitude,p->latitude<0?'S':'N',p->longitude<0?-p->longitude:p->longitude,p->longitude<0?'W':'E');
    line(d,popover->backing,gc,font,12,114,width-24,buffer);
    snprintf(buffer,sizeof(buffer),"Snapshot: %s",places->date);
    line(d,popover->backing,gc,font,12,132,width-24,buffer);
    snprintf(buffer,sizeof(buffer),"Wikidata Q%u, revision %u",p->qid,p->revision);
    line(d,popover->backing,gc,font,12,150,width-24,buffer);
    line(d,popover->backing,gc,font,12,170,width-24,"Entity population; geographic areas vary.");
    if(width>=48 && height>=50) {
        unsigned button_width=(popover->width-36)/2;
        XDrawRectangle(d,popover->backing,gc,12,height-48,button_width,24);
        XDrawRectangle(d,popover->backing,gc,24+(int)button_width,height-48,button_width,24);
        line(d,popover->backing,gc,font,18,height-32,(int)button_width-12,p->wikipedia[0]?"Copy Wikipedia":"No Wikipedia link");
        line(d,popover->backing,gc,font,30+(int)button_width,height-32,(int)button_width-12,"Copy Wikidata");
    }
    line(d,popover->backing,gc,font,12,height-8,width-24,
         popover->copied==1?"Wikipedia URL copied to clipboard":popover->copied==2?"Wikidata URL copied to clipboard":"Offline Wikidata snapshot | CC0");
    XCopyArea(d,popover->backing,popover->window,gc,0,0,popover->width,popover->height,0,0);
}
int popover_show(Popover *popover,Display *d,Window canvas,GC gc,XFontStruct *font,
                 const Places *places,int selected,int anchor_x,int anchor_y,unsigned width,unsigned height) {
    unsigned pw=width>388?380:width>8?width-8:1;
    unsigned ph=height>244?236:height>8?height-8:1;
    int x=anchor_x+12,y=anchor_y+12;
    if(selected<0||(size_t)selected>=places->count)return 0;
    if(x+(int)pw+2>(int)width)x=anchor_x-(int)pw-12;
    if(y+(int)ph+2>(int)height)y=(int)height-(int)ph-2;
    if(x<0)x=0;
    if(y<0)y=0;
    if(!popover->window) {
        popover->window=XCreateSimpleWindow(d,canvas,x,y,pw,ph,1,WhitePixel(d,DefaultScreen(d)),BlackPixel(d,DefaultScreen(d)));
        XStoreName(d,popover->window,"Planet place details");
        XSelectInput(d,popover->window,ExposureMask|ButtonPressMask|KeyPressMask);
    }
    if(!popover->backing||popover->width!=pw||popover->height!=ph) {
        if(popover->backing)XFreePixmap(d,popover->backing);
        popover->backing=XCreatePixmap(d,canvas,pw,ph,(unsigned)DefaultDepth(d,DefaultScreen(d)));
        if(!popover->backing)return 0;
    }
    {
        char title[128];
        snprintf(title,sizeof(title),"Planet place details: %s (Q%u)",places->items[selected].label,places->items[selected].qid);
        XStoreName(d,popover->window,title);
    }
    popover->width=pw;popover->height=ph;popover->selected=selected;popover->visible=1;popover->copied=0;
    XMoveResizeWindow(d,popover->window,x,y,pw,ph);XMapRaised(d,popover->window);
    popover_draw(popover,d,gc,font,places);return 1;
}
void popover_close(Popover *popover,Display *d) {
    if(popover->visible)XUnmapWindow(d,popover->window);
    popover->visible=0;popover->selected=-1;
}
void popover_free(Popover *popover,Display *d) {
    if(popover->backing)XFreePixmap(d,popover->backing);
    /* Its child window is destroyed with the application parent. */
    memset(popover,0,sizeof(*popover));popover->selected=-1;
}
int popover_action(const Popover *popover,int x,int y) {
    int width=(int)popover->width,height=(int)popover->height;
    if(!popover->visible)return 0;
    if(x>=width-28&&y>=0&&y<28)return 1;
    if(width<260||height<228)return 0;
    if(width>=48&&height>=50&&y>=height-48&&y<=height-24) {
        int button_width=(width-36)/2;
        if(x>=12&&x<=12+button_width)return 2;
        if(x>=24+button_width&&x<=24+2*button_width)return 3;
    }
    return 0;
}
