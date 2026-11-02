#ifndef PLANET_POPOVER_H
#define PLANET_POPOVER_H
#include <X11/Xlib.h>
#include "places.h"
typedef struct { Window window;Pixmap backing;unsigned width,height;int selected,visible,copied; } Popover;
int popover_show(Popover *,Display *,Window,GC,XFontStruct *,const Places *,int,int,int,unsigned,unsigned);
void popover_draw(Popover *,Display *,GC,XFontStruct *,const Places *);
void popover_close(Popover *,Display *);
void popover_free(Popover *,Display *);
/* 1 close, 2 Wikipedia link, 3 Wikidata link, 0 no action. */
int popover_action(const Popover *,int,int);
#endif
