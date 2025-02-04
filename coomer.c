#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define KEY_Q 24
#define KEY_ESC 9
#define BUTTON_SCROLL_UP 4
#define BUTTON_SCROLL_DOWN 5

void nearest_upscale(unsigned long* dat1, int width1, int height1, unsigned long* dat2, int width2, int height2) {
	for (int i = 0;i < height2; ++i) {
		for (int j = 0;j < width2; ++j) {
			dat2[i * width2 + j] = dat1[(i * height1/height2 * width1) + (j * width1 / width2)];
		}
	}
}

void nearest_upscale_image(XImage* img1, XImage* img2, int x, int y, int swidth, int sheight) {
	int nwidth = img2->width;
	int nheight = img2->height;
	for (int i = 0;i < nheight; ++i) {
		for (int j = 0;j < nwidth; ++j) {
			img2->f.put_pixel(img2, j, i, img1->f.get_pixel(img1, j * swidth / nwidth + x,i * sheight / nheight + y));
		}
	}
}


typedef struct  {
	int x;
	int y;
	int width;
	int height;
} IRect; //Funny haha lol

int main() {
	Display* dpy = XOpenDisplay(NULL);

	Window root = XDefaultRootWindow(dpy);
	int width = XDisplayWidth(dpy, 0);
	int height = XDisplayHeight(dpy, 0);
	int depth = XDefaultDepth(dpy, 0);

	int irects_capacity = 10;
	int irects_idx = 0;
	IRect *irects = malloc(sizeof(*irects) * irects_capacity);
	irects[0] = (IRect){0,0,width,height};

	unsigned long* img_data = malloc(width * height * sizeof(*img_data));
	XImage* ximg = XGetImage(dpy, root, 0, 0, width, height, AllPlanes, ZPixmap);
	XImage* ximg_zoom = XCreateImage(dpy, XDefaultVisual(dpy, 0), depth, ZPixmap, 0, (char*)img_data, width, height, 32, 0);
	for (int i = 0;i < height; ++i) {
		for (int j = 0;j < width; ++j) {
			ximg_zoom->f.put_pixel(ximg_zoom, j, i, ximg->f.get_pixel(ximg, j, i));
		}
	}
	for (int i = 0;i < height/2; ++i) {
		for (int j = 0;j < width/2; ++j) {
			ximg_zoom->f.put_pixel(ximg_zoom, j, i, 0x1f1f1f);
		}
	}
	Pixmap pixmap = XCreatePixmap(dpy, root, width, height, depth);

	XGCValues xgcv;
	xgcv.function = GXcopy;
	GC fs_gc = XCreateGC(dpy, pixmap, GCFunction, &xgcv);

	XPutImage(dpy, pixmap, fs_gc, ximg, 0, 0, 0,0, width, height);


	XSetWindowAttributes win_attr = {};
	win_attr.background_pixmap = pixmap;
	win_attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask;


	Window fs_win = XCreateWindow(dpy, root, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWBackPixmap | CWEventMask, &win_attr);

	Atom atoms[2] = {XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False),None};
	Atom a_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	XChangeProperty(dpy, fs_win, a_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, 1);

	XMapWindow(dpy, fs_win);
	int _ = 1;
	XEvent event;
	for (;_;) {
		XNextEvent(dpy, &event);	
		if (event.type == Expose) {
		} else if (event.type == KeyPress) {
			if (event.xkey.keycode == KEY_ESC || event.xkey.keycode == KEY_Q) _ = 0;
		} else if (event.type == ButtonPress) {
			if (event.xbutton.button == BUTTON_SCROLL_UP) {
				IRect zoomed_irect = (IRect)irects[irects_idx];
				zoomed_irect.width = zoomed_irect.width * 3 / 4;
				zoomed_irect.height = zoomed_irect.height * 3 / 4;

				int bruh = 0;

				if (zoomed_irect.width * 100 < width || zoomed_irect.height * 100 < height) {
					zoomed_irect.width = width/100;;
					zoomed_irect.height = height/100;
					bruh = 1;
				}

				int xpos = event.xbutton.x;
				int ypos = event.xbutton.y;

				if (xpos * 3 > width * 2) {
					zoomed_irect.x = irects[irects_idx].x + (irects[irects_idx].width - zoomed_irect.width);
				} else if (xpos * 3 < width) {
					zoomed_irect.x = irects[irects_idx].x;
				} else {
					zoomed_irect.x = irects[irects_idx].x + (irects[irects_idx].width - zoomed_irect.width) / 2;
				}

				if (ypos * 3 > height * 2) {
					zoomed_irect.y = irects[irects_idx].y + (irects[irects_idx].height - zoomed_irect.height);
				} else if (ypos * 3 < height) {
					zoomed_irect.y = irects[irects_idx].y;
				} else {
					zoomed_irect.y = irects[irects_idx].y + (irects[irects_idx].height - zoomed_irect.height) / 2;
				}
				irects_idx -= bruh;
				irects_idx++;
				if (irects_idx >= irects_capacity) {
					irects_capacity *= 2;
					irects = realloc(irects, sizeof(*irects) * irects_capacity);
				}
				irects[irects_idx] = zoomed_irect;
				nearest_upscale_image(ximg, ximg_zoom, zoomed_irect.x, zoomed_irect.y, zoomed_irect.width, zoomed_irect.height);
				XPutImage(dpy, pixmap, fs_gc, ximg_zoom, 0, 0, 0, 0, width, height);
				XClearWindow(dpy,fs_win);
			}
			if (event.xbutton.button == BUTTON_SCROLL_DOWN) {
				if (irects_idx < 1) {
					irects_idx = 1;
				}
				IRect prev_zoomed_irect = irects[--irects_idx];
				nearest_upscale_image(ximg, ximg_zoom, prev_zoomed_irect.x, prev_zoomed_irect.y, prev_zoomed_irect.width, prev_zoomed_irect.height);
				XPutImage(dpy, pixmap, fs_gc, ximg_zoom, 0, 0, 0, 0, width, height);
				XClearWindow(dpy,fs_win);
			}
		}
	}

	free(irects);
	free(img_data);
	free(ximg->data);
	XUnmapWindow(dpy, fs_win);
	XDestroyWindow(dpy, fs_win);
	XClearWindow(dpy, root);
	XCloseDisplay(dpy);
	return 0;
}
