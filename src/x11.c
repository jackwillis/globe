#define _POSIX_C_SOURCE 200809L
#include "planet.h"
#include "ximage.h"
#include "places.h"
#include "popover.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { TOOLBAR_HEIGHT=28 };
static void selection_reply(Display *d,const XSelectionRequestEvent *request,Atom clipboard,
                            Atom targets,Atom utf8,const char *text) {
    XEvent response;Atom property=request->property==None?request->target:request->property;
    memset(&response,0,sizeof(response));response.xselection.type=SelectionNotify;
    response.xselection.display=d;response.xselection.requestor=request->requestor;
    response.xselection.selection=request->selection;response.xselection.target=request->target;
    response.xselection.time=request->time;response.xselection.property=None;
    if(request->selection==clipboard && text[0]) {
        if(request->target==targets) {
            Atom supported[3];supported[0]=targets;supported[1]=utf8;supported[2]=XA_STRING;
            XChangeProperty(d,request->requestor,property,XA_ATOM,32,PropModeReplace,(unsigned char *)supported,3);
            response.xselection.property=property;
        } else if(request->target==utf8 ||request->target==XA_STRING) {
            XChangeProperty(d,request->requestor,property,request->target,8,PropModeReplace,(const unsigned char *)text,(int)strlen(text));
            response.xselection.property=property;
        }
    }
    XSendEvent(d,request->requestor,False,0,&response);
}
static int draw_toolbar(Display *d,Window toolbar,GC gc,Pixmap *backing,
                        unsigned *backing_width,unsigned width,const View *view,int have_borders,int have_places,int places_enabled) {
    static const char *themes[]={"Ocean","Atlas","Terminal"};
    char labels[6][32];int item;
    if(!*backing || *backing_width!=width) {
        if(*backing)XFreePixmap(d,*backing);
        *backing=XCreatePixmap(d,toolbar,width,TOOLBAR_HEIGHT,(unsigned)DefaultDepth(d,DefaultScreen(d)));
        *backing_width=width;
        if(!*backing)return 0;
    }
    snprintf(labels[0],32,"%s",themes[view->theme]);
    snprintf(labels[1],32,"Grid %s",view->grid?"on":"off");
    snprintf(labels[2],32,"Borders %s",!have_borders?"n/a":view->borders?"on":"off");
    snprintf(labels[3],32,"Coasts %s",view->coasts?"on":"off");
    snprintf(labels[4],32,"Places %s",!have_places?"n/a":places_enabled?"on":"off");
    snprintf(labels[5],32,"Reset");
    /* Paint offscreen, then copy once: no visible erase-and-redraw sequence. */
    XSetForeground(d,gc,BlackPixel(d,DefaultScreen(d)));
    XFillRectangle(d,*backing,gc,0,0,width,TOOLBAR_HEIGHT);
    XSetForeground(d,gc,WhitePixel(d,DefaultScreen(d)));
    for(item=0;item<6;item++) {
        XDrawRectangle(d,*backing,gc,item*96,0,95,TOOLBAR_HEIGHT-1);
        XDrawString(d,*backing,gc,item*96+5,18,labels[item],(int)strlen(labels[item]));
    }
    XCopyArea(d,*backing,toolbar,gc,0,0,width,TOOLBAR_HEIGHT,0,0);
    return 1;
}
int main(int argc,char **argv) {
    Display *d;Window w,toolbar,canvas;Pixmap toolbar_backing=0;unsigned toolbar_width=0;XEvent e;Atom close_atom;GC gc;Map map,borders;Frame frame={0};
    ViewCache cache={0};ImageBuffer image_buffer={0};
    View view={0,0,1,1};int have_borders,have_places,places_enabled=1;
    Places places={0};Popover popover={0};XFontStruct *font=NULL;
    Atom clipboard,targets,utf8;char clipboard_text[512]="";
    int press_x=0,press_y=0,drag_moved=0;
    double yaw=0,pitch=-15,zoom=1;int done=0,dirty=1,status=0,toolbar_dirty=1,dragging=0,last_x=0,last_y=0;
    unsigned width=640,height=508,canvas_height=480;
    if(argc>2){fprintf(stderr,"usage: planet-x11 [map.plnt]\n");return 2;}
    if(!map_load(&map,argc==2?argv[1]:"data/coastline.plnt")){fprintf(stderr,"cannot load map\n");return 1;}
    have_borders=map_load(&borders,"data/borders.plnt");
    have_places=places_load(&places,"data/places.plnt");popover.selected=-1;
    d=XOpenDisplay(NULL);
    if(!d){fprintf(stderr,"cannot open X display; check existing display authorization\n");places_free(&places);map_free(&borders);map_free(&map);return 1;}
    if(DefaultVisual(d,DefaultScreen(d))->class!=TrueColor){fprintf(stderr,"TrueColor visual required\n");XCloseDisplay(d);places_free(&places);map_free(&borders);map_free(&map);return 1;}
    w=XCreateSimpleWindow(d,DefaultRootWindow(d),0,0,width,height,0,0,0);
    XStoreName(d,w,"Planet - drag to rotate, wheel to zoom; click place markers for facts");
    toolbar=XCreateSimpleWindow(d,w,0,0,width,TOOLBAR_HEIGHT,0,0,0);
    canvas=XCreateSimpleWindow(d,w,0,TOOLBAR_HEIGHT,width,canvas_height,0,0,0);
    XStoreName(d,toolbar,"Planet controls");XStoreName(d,canvas,"Planet globe");
    XSelectInput(d,w,KeyPressMask|StructureNotifyMask);
    XSelectInput(d,toolbar,ExposureMask|ButtonPressMask);
    XSelectInput(d,canvas,ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask|Button1MotionMask);
    {
        XSizeHints hints;memset(&hints,0,sizeof(hints));
        hints.flags=PMinSize;hints.min_width=576;hints.min_height=128;
        XSetWMNormalHints(d,w,&hints);
    }
    close_atom=XInternAtom(d,"WM_DELETE_WINDOW",False);XSetWMProtocols(d,w,&close_atom,1);
    clipboard=XInternAtom(d,"CLIPBOARD",False);targets=XInternAtom(d,"TARGETS",False);utf8=XInternAtom(d,"UTF8_STRING",False);
    gc=XCreateGC(d,w,0,NULL);font=XQueryFont(d,XGContextFromGC(gc));XSetGraphicsExposures(d,gc,False);
    XMapWindow(d,toolbar);XMapWindow(d,canvas);XMapWindow(d,w);
    while(!done) {
        if(!dirty && !toolbar_dirty)XPeekEvent(d,&e); /* Sleep until input or exposure. */
        while(XPending(d)) {
            XNextEvent(d,&e);
            if(e.type==SelectionRequest){selection_reply(d,&e.xselectionrequest,clipboard,targets,utf8,clipboard_text);continue;}
            if(e.type==SelectionClear && e.xselectionclear.selection==clipboard)clipboard_text[0]=0;
            if(e.type==ButtonPress && e.xbutton.window==popover.window) {
                int action;
                if(e.xbutton.button!=Button1)continue;
                action=popover_action(&popover,e.xbutton.x,e.xbutton.y);
                if(action==1){popover_close(&popover,d);dirty=1;}
                if((action==2||action==3)&&popover.selected>=0) {
                    const Place *place=places.items+popover.selected;
                    if(action==2 && place->wikipedia[0])snprintf(clipboard_text,sizeof(clipboard_text),"%s",place->wikipedia);
                    else if(action==3)snprintf(clipboard_text,sizeof(clipboard_text),"https://www.wikidata.org/w/index.php?title=Q%u&oldid=%u",place->qid,place->revision);
                    else continue;
                    XSetSelectionOwner(d,clipboard,w,CurrentTime);
                    if(XGetSelectionOwner(d,clipboard)==w)popover.copied=action==2?1:2;
                    popover_draw(&popover,d,gc,font,&places);
                }
                continue;
            }
            if(e.type==ClientMessage && (Atom)e.xclient.data.l[0]==close_atom)done=1;
            if(e.type==DestroyNotify)done=1;
            if(e.type==Expose) {
                if(e.xexpose.window==toolbar)toolbar_dirty=1;
                if(e.xexpose.window==canvas)dirty=1;
                if(e.xexpose.window==popover.window)popover_draw(&popover,d,gc,font,&places);
            }
            if(e.type==ConfigureNotify && e.xconfigure.window==w) {
                unsigned new_width=(unsigned)e.xconfigure.width,new_height=(unsigned)e.xconfigure.height;
                if(new_width!=width || new_height!=height) {
                    width=new_width; height=new_height;popover_close(&popover,d);dragging=0;
                    canvas_height=height>TOOLBAR_HEIGHT?height-TOOLBAR_HEIGHT:1;
                    XMoveResizeWindow(d,toolbar,0,0,width,TOOLBAR_HEIGHT);
                    XMoveResizeWindow(d,canvas,0,TOOLBAR_HEIGHT,width,canvas_height);
                    dirty=1;toolbar_dirty=1;
                }
            }
            if(e.type==ButtonPress) {
                if(e.xbutton.button==Button1) {
                    if(e.xbutton.window==toolbar && e.xbutton.x>=0 && e.xbutton.x<576) {
                        int action=e.xbutton.x/96;
                        if(action==0)view.theme=(view.theme+1)%3;
                        if(action==1)view.grid=!view.grid;
                        if(action==2 && have_borders)view.borders=!view.borders;
                        if(action==3)view.coasts=!view.coasts;
                        if(action==4 && have_places)places_enabled=!places_enabled;
                        if(action==5){yaw=0;pitch=-15;zoom=1;}
                        popover_close(&popover,d);
                        dirty=1;toolbar_dirty=1;
                    } else if(e.xbutton.window==canvas) {
                        popover_close(&popover,d);dirty=1;dragging=1;drag_moved=0;
                        press_x=last_x=e.xbutton.x;press_y=last_y=e.xbutton.y;
                    }
                }
                if(e.xbutton.window==canvas && e.xbutton.button==Button4){zoom*=1.1;popover_close(&popover,d);dirty=1;}
                if(e.xbutton.window==canvas && e.xbutton.button==Button5){zoom/=1.1;popover_close(&popover,d);dirty=1;}
            }
            if(e.type==ButtonRelease && e.xbutton.button==Button1) {
                if(dragging&&!drag_moved&&places_enabled&&have_places) {
                    unsigned rw=width>2048?2048:width,rh=canvas_height>2048?2048:canvas_height;
                    int hit=places_pick(&places,yaw,pitch,zoom,rw,rh,
                        e.xbutton.x-(int)((width-rw)/2),e.xbutton.y-(int)((canvas_height-rh)/2));
                    if(hit>=0)popover_show(&popover,d,canvas,gc,font,&places,hit,e.xbutton.x,e.xbutton.y,width,canvas_height);
                    dirty=1;
                }
                dragging=0;
            }
            if(e.type==MotionNotify && dragging) {
                if(e.xmotion.state&Button1Mask) {
                    unsigned span;
                    if(abs(e.xmotion.x-press_x)>4||abs(e.xmotion.y-press_y)>4)drag_moved=1;
                    if(!drag_moved)continue;
                    span=width<canvas_height?width:canvas_height;
                    if(!span)span=1;
                    /* Match drag distance to the rendered globe diameter. */
                    double degrees_per_pixel=180.0/(0.9*(span>2048?2048:span)*zoom);
                    yaw+=(e.xmotion.x-last_x)*degrees_per_pixel;
                    pitch+=(e.xmotion.y-last_y)*degrees_per_pixel;
                    last_x=e.xmotion.x;last_y=e.xmotion.y;dirty=1;
                } else dragging=0;
            }
            if(e.type==KeyPress) {
                KeySym k=XLookupKeysym(&e.xkey,0);
                if(k==XK_Escape && popover.visible){popover_close(&popover,d);dirty=1;continue;}
                if(k==XK_Escape)done=1;
                popover_close(&popover,d);
                if((k==XK_p||k==XK_P)&&have_places){places_enabled=!places_enabled;toolbar_dirty=1;}
                if(k==XK_t||k==XK_T)view.theme=(view.theme+1)%3;
                if(k==XK_g||k==XK_G)view.grid=!view.grid;
                if((k==XK_b||k==XK_B)&&have_borders)view.borders=!view.borders;
                if(k==XK_c||k==XK_C)view.coasts=!view.coasts;
                if(k==XK_t||k==XK_T||k==XK_g||k==XK_G||k==XK_b||k==XK_B||k==XK_c||k==XK_C)toolbar_dirty=1;
                if(k==XK_Left)yaw-=5;
                if(k==XK_Right)yaw+=5;
                if(k==XK_Up)pitch-=5;
                if(k==XK_Down)pitch+=5;
                if(k==XK_equal||k==XK_plus||k==XK_KP_Add)zoom*=1.1;
                if(k==XK_minus||k==XK_KP_Subtract)zoom/=1.1;
                if(k==XK_r||k==XK_R){yaw=0;pitch=-15;zoom=1;}
                if(zoom<0.1)zoom=0.1;
                if(zoom>8)zoom=8;
                if(yaw>360)yaw-=360;
                if(yaw< -360)yaw+=360;
                if(pitch>360)pitch-=360;
                if(pitch< -360)pitch+=360;
                dirty=1;
            }
        }
        if(zoom<0.1)zoom=0.1;
        if(zoom>8)zoom=8;
        while(yaw>360)yaw-=360;
        while(yaw< -360)yaw+=360;
        while(pitch>360)pitch-=360;
        while(pitch< -360)pitch+=360;
        if(dirty&&!done) {
            /* Cap render allocation and letterbox oversized windows. */
            unsigned rw=width>2048?2048:width,rh=canvas_height>2048?2048:canvas_height;
            if(frame.width!=rw || frame.height!=rh) {
                frame_free(&frame);
                if(!frame_init(&frame,rw,rh)){fprintf(stderr,"frame allocation failed\n");status=1;break;}
            }
            if(!image_buffer_resize(&image_buffer,d,rw,rh)
               ||!map_view_fast(&frame,&map,have_borders?&borders:NULL,yaw,pitch,zoom,&view,&cache)) {
                fprintf(stderr,"frame preparation failed\n");status=1;break;
            }
            if(places_enabled&&have_places)places_render(&frame,&places,yaw,pitch,zoom,popover.selected);
            if(!image_convert(&image_buffer,&frame)){fprintf(stderr,"image conversion failed\n");status=1;break;}
            {
                unsigned left=(width-rw)/2,top=(canvas_height-rh)/2;
                /* Clear only gutters, never the image or the separate toolbar. */
                XSetForeground(d,gc,BlackPixel(d,DefaultScreen(d)));
                if(left)XFillRectangle(d,canvas,gc,0,0,left,canvas_height);
                if(width>left+rw)XFillRectangle(d,canvas,gc,(int)(left+rw),0,width-left-rw,canvas_height);
                if(top)XFillRectangle(d,canvas,gc,(int)left,0,rw,top);
                if(canvas_height>top+rh)XFillRectangle(d,canvas,gc,(int)left,(int)(top+rh),rw,canvas_height-top-rh);
                XPutImage(d,canvas,gc,image_buffer.image,0,0,(int)left,(int)top,rw,rh);
            }
            dirty=0;
        }
        if(toolbar_dirty && !done) {
            if(!draw_toolbar(d,toolbar,gc,&toolbar_backing,&toolbar_width,width,&view,have_borders,have_places,places_enabled)) {
                fprintf(stderr,"toolbar allocation failed\n");status=1;break;
            }
            toolbar_dirty=0;
        }
        if(!done)XSync(d,False);
    }
    popover_free(&popover,d);places_free(&places);
    if(font)XFreeFontInfo(NULL,font,1);
    image_buffer_free(&image_buffer);view_cache_free(&cache);frame_free(&frame);
    if(toolbar_backing)XFreePixmap(d,toolbar_backing);
    XFreeGC(d,gc);if(!done||e.type!=DestroyNotify)XDestroyWindow(d,w);
    XCloseDisplay(d);map_free(&borders);map_free(&map);return status;
}
