/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include <stdint.h>

struct ImageFit { int x,y,width,height; };
/* Contain, never cover: callers always use the complete source rectangle. */
static inline struct ImageFit fit_image(int sw,int sh,int x,int y,int w,int h) {
 struct ImageFit fit={x,y,0,0};
 if(sw<=0||sh<=0||w<=0||h<=0)return fit;
 if((int64_t)sw*h>(int64_t)sh*w){fit.width=w;fit.height=(int)((int64_t)sh*w/sw);}
 else {fit.height=h;fit.width=(int)((int64_t)sw*h/sh);}
 fit.x=x+(w-fit.width)/2;fit.y=y+(h-fit.height)/2;
 return fit;
}

/* Cover uses the complete source; clipping happens at the destination bounds. */
static inline struct ImageFit cover_image(int sw,int sh,int x,int y,int w,int h) {
 struct ImageFit fit={x,y,0,0};
 if(sw<=0||sh<=0||w<=0||h<=0)return fit;
 if((int64_t)sw*h>(int64_t)sh*w){fit.height=h;fit.width=(int)(((int64_t)sw*h+sh-1)/sh);}
 else {fit.width=w;fit.height=(int)(((int64_t)sh*w+sw-1)/sw);}
 fit.x=x+(w-fit.width)/2;fit.y=y+(h-fit.height)/2;
 return fit;
}
