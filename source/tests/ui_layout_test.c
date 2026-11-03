/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include <assert.h>
#include "../ui_layout.h"

static void contained(int sw,int sh,int w,int h) {
 struct ImageFit f=fit_image(sw,sh,584,104,w,h);
 assert(f.width>0&&f.height>0);
 assert(f.x>=584&&f.y>=104);
 assert(f.x+f.width<=584+w&&f.y+f.height<=104+h);
 assert(f.width==w||f.height==h);
 /* Integer scaling may round down by less than one output pixel. */
 int64_t error=(int64_t)f.width*sh-(int64_t)f.height*sw;
 assert(error<sw&&error>-sh);
 assert(f.x-584<=(w-f.width)/2+1);
 assert(f.y-104<=(h-f.height)/2+1);
}
int main(void) {
 contained(1672,941,392,232); /* Actual embedded art and viewport. */
 contained(941,1672,392,232); /* Portrait art must also remain complete. */
 contained(1920,1080,1600,900);
 contained(4000,400,392,232);
 contained(400,4000,392,232);
 struct ImageFit empty=fit_image(0,941,584,104,392,232);
 assert(empty.width==0&&empty.height==0);
 return 0;
}
