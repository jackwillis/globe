#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "places.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
static void settle(Display *d) {
    struct timespec delay={0,150000000};XSync(d,False);nanosleep(&delay,NULL);
}
static unsigned long snapshot(Display *d,Window w) {
    XWindowAttributes a;XImage *image;unsigned long hash=5381;int x,y;
    XGetWindowAttributes(d,w,&a);
    image=XGetImage(d,w,0,0,(unsigned)a.width,(unsigned)a.height,AllPlanes,ZPixmap);
    assert(image);
    for(y=0;y<a.height;y++)for(x=0;x<a.width;x++)hash=hash*33+XGetPixel(image,x,y);
    XDestroyImage(image);return hash;
}
static void capture(Display *d,Window window,const char *path) {
    XWindowAttributes a;XImage *image;FILE *out;int x,y;
    if(!path)return;
    XGetWindowAttributes(d,window,&a);
    image=XGetImage(d,window,0,0,(unsigned)a.width,(unsigned)a.height,AllPlanes,ZPixmap);assert(image);
    out=fopen(path,"wb");assert(out);fprintf(out,"P6\n%d %d\n255\n",a.width,a.height);
    for(y=0;y<a.height;y++)for(x=0;x<a.width;x++) {
        unsigned long pixel=XGetPixel(image,x,y);
        unsigned char rgb[3];
        rgb[0]=(unsigned char)((uint64_t)(pixel&image->red_mask)*255/image->red_mask);
        rgb[1]=(unsigned char)((uint64_t)(pixel&image->green_mask)*255/image->green_mask);
        rgb[2]=(unsigned char)((uint64_t)(pixel&image->blue_mask)*255/image->blue_mask);
        assert(fwrite(rgb,1,3,out)==3);
    }
    assert(!fclose(out));XDestroyImage(image);
}
static void button(Display *d,Window w,int type,unsigned n,int x,int y) {
    XEvent e;memset(&e,0,sizeof(e));e.xbutton.type=type;e.xbutton.display=d;
    e.xbutton.window=w;e.xbutton.button=n;e.xbutton.x=x;e.xbutton.y=y;
    XSendEvent(d,w,False,type==ButtonPress?ButtonPressMask:ButtonReleaseMask,&e);
}
static void clipboard_check(Display *d,Atom target,const char *expected) {
    Atom clipboard=XInternAtom(d,"CLIPBOARD",False),property=XInternAtom(d,"PLANET_TEST_LINK",False);
    Window reader=XCreateSimpleWindow(d,DefaultRootWindow(d),0,0,1,1,0,0,0);
    XEvent event;int found=0,format;Atom actual;unsigned long count,remaining;unsigned char *bytes=NULL;
    XConvertSelection(d,clipboard,target,property,reader,CurrentTime);settle(d);
    while(XPending(d)) {
        XNextEvent(d,&event);
        if(event.type==SelectionNotify&&event.xselection.requestor==reader) {
            assert(event.xselection.property==property);found=1;
        }
    }
    assert(found);
    assert(XGetWindowProperty(d,reader,property,0,256,True,AnyPropertyType,&actual,&format,&count,&remaining,&bytes)==Success);
    assert(actual==target&&format==8&&remaining==0&&count==strlen(expected));
    assert(!memcmp(bytes,expected,count));XFree(bytes);XDestroyWindow(d,reader);
}
int main(void) {
    Display *d=XOpenDisplay(NULL);Window root,parent,*children,w=0,toolbar=0,canvas=0;unsigned n,i;
    unsigned long initial,dragged,zoomed,toolbar_before;XEvent e;
    assert(d);settle(d);
    assert(XQueryTree(d,DefaultRootWindow(d),&root,&parent,&children,&n));
    for(i=0;i<n;i++) {
        char *name=NULL;
        if(XFetchName(d,children[i],&name) && name) {
            if(strncmp(name,"Planet -",8)==0)w=children[i];
            XFree(name);
        }
    }
    XFree(children);assert(w);
    assert(XQueryTree(d,w,&root,&parent,&children,&n));
    for(i=0;i<n;i++) {
        char *name=NULL;
        if(XFetchName(d,children[i],&name) && name) {
            if(!strcmp(name,"Planet controls"))toolbar=children[i];
            if(!strcmp(name,"Planet globe"))canvas=children[i];
            XFree(name);
        }
    }
    XFree(children);assert(toolbar&&canvas);
    {
        XWindowAttributes a,b,c;
        XGetWindowAttributes(d,w,&a);XGetWindowAttributes(d,toolbar,&b);XGetWindowAttributes(d,canvas,&c);
        assert(b.y==0 && b.height==28 && c.y==28 && c.height==a.height-28);
        assert(c.width==a.width && b.width==a.width);
    }
    capture(d,w,getenv("X11_SCREENSHOT"));
    toolbar_before=snapshot(d,toolbar);
    initial=snapshot(d,canvas);settle(d);assert(snapshot(d,canvas)==initial);
    {
        Places places={0};int px,py,index=-1;Window details=0;XWindowAttributes a;
        assert(places_load(&places,"data/places.plnt"));
        for(i=0;i<places.count;i++)if(places.items[i].qid==85)index=(int)i;
        assert(index>=0&&place_project(places.items+index,0,-15,1,640,480,&px,&py));
        button(d,canvas,ButtonPress,Button1,px,py);
        button(d,canvas,ButtonRelease,Button1,px,py);settle(d);
        assert(XQueryTree(d,canvas,&root,&parent,&children,&n));
        for(i=0;i<n;i++) {
            char *name=NULL;
            if(XFetchName(d,children[i],&name)&&name) {
                if(!strcmp(name,"Planet place details: Cairo (Q85)"))details=children[i];
                XFree(name);
            }
        }
        XFree(children);assert(details);
        XGetWindowAttributes(d,details,&a);
        assert(a.map_state==IsViewable&&a.x>=0&&a.y>=0&&a.x+a.width+2<=640&&a.y+a.height+2<=480);
        capture(d,w,getenv("X11_POPOVER_SCREENSHOT"));
        button(d,details,ButtonPress,Button1,30,a.height-36);settle(d);
        clipboard_check(d,XInternAtom(d,"UTF8_STRING",False),places.items[index].wikipedia);
        button(d,details,ButtonPress,Button1,24+(a.width-36)/2+20,a.height-36);settle(d);
        {
            char url[128];
            snprintf(url,sizeof(url),"https://www.wikidata.org/w/index.php?title=Q85&oldid=%u",places.items[index].revision);
            clipboard_check(d,XA_STRING,url);
        }
        button(d,details,ButtonPress,Button1,a.width-16,14);settle(d);
        XGetWindowAttributes(d,details,&a);assert(a.map_state==IsUnmapped);
        assert(snapshot(d,canvas)==initial); /* Clicking did not rotate the globe. */
        button(d,canvas,ButtonPress,Button1,px+1,py);
        /* Small pointer jitter still counts as a click rather than a drag. */
        memset(&e,0,sizeof(e));e.xmotion.type=MotionNotify;e.xmotion.display=d;
        e.xmotion.window=canvas;e.xmotion.state=Button1Mask;e.xmotion.x=px+2;e.xmotion.y=py;
        XSendEvent(d,canvas,False,Button1MotionMask,&e);
        button(d,canvas,ButtonRelease,Button1,px+2,py);settle(d);
        XGetWindowAttributes(d,details,&a);assert(a.map_state==IsViewable);
        memset(&e,0,sizeof(e));e.xkey.type=KeyPress;e.xkey.display=d;e.xkey.window=details;
        e.xkey.keycode=XKeysymToKeycode(d,XK_Escape);
        XSendEvent(d,details,False,KeyPressMask,&e);settle(d);
        XGetWindowAttributes(d,details,&a);assert(a.map_state==IsUnmapped);
        assert(snapshot(d,canvas)==initial);
        button(d,toolbar,ButtonPress,Button1,414,14);settle(d);
        assert(snapshot(d,canvas)!=initial);
        button(d,canvas,ButtonPress,Button1,px,py);
        button(d,canvas,ButtonRelease,Button1,px,py);settle(d);
        XGetWindowAttributes(d,details,&a);assert(a.map_state==IsUnmapped);
        button(d,toolbar,ButtonPress,Button1,414,14);settle(d);
        assert(snapshot(d,canvas)==initial);
        assert(snapshot(d,toolbar)==toolbar_before);
        places_free(&places);
    }
    button(d,canvas,ButtonPress,Button1,200,200);
    memset(&e,0,sizeof(e));e.xmotion.type=MotionNotify;e.xmotion.display=d;
    e.xmotion.window=canvas;e.xmotion.state=Button1Mask;e.xmotion.x=280;e.xmotion.y=230;
    XSendEvent(d,canvas,False,Button1MotionMask,&e);
    button(d,canvas,ButtonRelease,Button1,280,230);settle(d);
    dragged=snapshot(d,canvas);assert(dragged!=initial);
    assert(snapshot(d,toolbar)==toolbar_before);
    button(d,canvas,ButtonPress,Button4,280,230);settle(d);
    zoomed=snapshot(d,canvas);assert(zoomed!=dragged);
    assert(snapshot(d,toolbar)==toolbar_before);
    settle(d);assert(snapshot(d,canvas)==zoomed);
    /* Canvas exposure and wheel/blank-area input in the toolbar cannot change it. */
    memset(&e,0,sizeof(e));e.xexpose.type=Expose;e.xexpose.display=d;e.xexpose.window=canvas;
    XSendEvent(d,canvas,False,ExposureMask,&e);settle(d);
    assert(snapshot(d,toolbar)==toolbar_before);
    button(d,toolbar,ButtonPress,Button4,610,14);settle(d);
    button(d,toolbar,ButtonPress,Button1,610,14);
    button(d,toolbar,ButtonRelease,Button1,610,14);settle(d);
    assert(snapshot(d,canvas)==zoomed);
    /* Theme and all three layer switches must change the actual window. */
    for(i=0;i<4;i++) {
        unsigned long before=snapshot(d,canvas);
        button(d,toolbar,ButtonPress,Button1,(int)i*96+30,14);
        button(d,toolbar,ButtonRelease,Button1,(int)i*96+30,14);settle(d);
        assert(snapshot(d,canvas)!=before);
    }
    XResizeWindow(d,w,317,193);settle(d);
    {
        XWindowAttributes attributes;XGetWindowAttributes(d,w,&attributes);
        assert(attributes.width==317&&attributes.height==193);
        XGetWindowAttributes(d,canvas,&attributes);
        assert(attributes.y==28 && attributes.height==165 && attributes.width==317);
        initial=snapshot(d,canvas);settle(d);assert(snapshot(d,canvas)==initial);
    }
    XResizeWindow(d,w,1,20);settle(d);
    {
        XWindowAttributes attributes;XGetWindowAttributes(d,canvas,&attributes);
        assert(attributes.y==28 && attributes.height==1 && attributes.width==1);
    }
    XResizeWindow(d,w,640,508);settle(d);
    XCloseDisplay(d);puts("Wikidata popovers, clipboard, markers, toolbar, mouse and resize passed");return 0;
}
