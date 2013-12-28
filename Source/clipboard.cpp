#ifndef _WIN32 // Only defined for Linux
/*
 *
 *  Source code copied from (https://sourceforge.net/projects/xclip/):
 *  xclip.c - command line interface to X server selections
 *  Copyright (C) 2001 Kim Saunders
 *  Copyright (C) 2007-2008 Peter Åstrand
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <memory.h>

#include "clipboard.h"
#include "Debug.h"

/* xcout() contexts */
#define XCLIB_XCOUT_NONE	0	/* no context */
#define XCLIB_XCOUT_SENTCONVSEL	1	/* sent a request */
#define XCLIB_XCOUT_INCR	2	/* in an incr loop */
#define XCLIB_XCOUT_FALLBACK	3	/* UTF8_STRING failed, need fallback to XA_STRING */

namespace Controller {
static Atom sseln = XA_PRIMARY;	/* X selection to work with */
static Atom target = XA_STRING;
/* Retrieves the contents of a selections. Arguments are:
 *
 * A display that has been opened.
 *
 * A window
 *
 * An event to process
 *
 * The selection to return
 *
 * The target(UTF8_STRING or XA_STRING) to return
 *
 * A pointer to a char array to put the selection into.
 *
 * A pointer to a long to record the length of the char array
 *
 * A pointer to an int to record the context in which to process the event
 *
 * Return value is 1 if the retrieval of the selection data is complete,
 * otherwise it's 0.
 */
static int
xcout(Display * dpy,
      Window win,
      XEvent evt, Atom sel, Atom target, unsigned char **txt, unsigned long *len,
      unsigned int *context)
{
    /* a property for other windows to put their selection into */
    static Atom pty;
    static Atom inc;
    Atom pty_type;
    Atom atomUTF8String;
    int pty_format;

    /* buffer for XGetWindowProperty to dump data into */
    unsigned char *buffer;
    unsigned long pty_size, pty_items;

    /* local buffer of text to return */
    unsigned char *ltxt = *txt;

    if (!pty) {
	pty = XInternAtom(dpy, "XCLIP_OUT", False);
    }

    if (!inc) {
	inc = XInternAtom(dpy, "INCR", False);
    }

    switch (*context) {
	/* there is no context, do an XConvertSelection() */
    case XCLIB_XCOUT_NONE:
	/* initialise return length to 0 */
	if (*len > 0) {
	    free(*txt);
	    *len = 0;
	}

	/* send a selection request */
	XConvertSelection(dpy, sel, target, pty, win, CurrentTime);
	*context = XCLIB_XCOUT_SENTCONVSEL;
	return (0);

    case XCLIB_XCOUT_SENTCONVSEL:
	atomUTF8String = XInternAtom(dpy, "UTF8_STRING", False);
	if (evt.type != SelectionNotify)
	    return (0);

	/* fallback to XA_STRING when UTF8_STRING failed */
	if (target == atomUTF8String && evt.xselection.property == None) {
	    *context = XCLIB_XCOUT_FALLBACK;
	    return (0);
	}

	/* find the size and format of the data in property */
	XGetWindowProperty(dpy,
			   win,
			   pty,
			   0,
			   0,
			   False,
			   AnyPropertyType, &pty_type, &pty_format, &pty_items, &pty_size, &buffer);
	XFree(buffer);

	if (pty_type == inc) {
	    /* start INCR mechanism by deleting property */
	    XDeleteProperty(dpy, win, pty);
	    XFlush(dpy);
	    *context = XCLIB_XCOUT_INCR;
	    return (0);
	}

	/* if it's not incr, and not format == 8, then there's
	 * nothing in the selection (that xclip understands,
	 * anyway)
	 */
	if (pty_format != 8) {
	    *context = XCLIB_XCOUT_NONE;
	    return (0);
	}

	/* not using INCR mechanism, just read the property */
	XGetWindowProperty(dpy,
			   win,
			   pty,
			   0,
			   (long) pty_size,
			   False,
			   AnyPropertyType, &pty_type, &pty_format, &pty_items, &pty_size, &buffer);

	/* finished with property, delete it */
	XDeleteProperty(dpy, win, pty);

	/* copy the buffer to the pointer for returned data */
	ltxt = (unsigned char *) malloc(pty_items);
	memcpy(ltxt, buffer, pty_items);

	/* set the length of the returned data */
	*len = pty_items;
	*txt = ltxt;

	/* free the buffer */
	XFree(buffer);

	*context = XCLIB_XCOUT_NONE;

	/* complete contents of selection fetched, return 1 */
	return (1);

    case XCLIB_XCOUT_INCR:
	/* To use the INCR method, we basically delete the
	 * property with the selection in it, wait for an
	 * event indicating that the property has been created,
	 * then read it, delete it, etc.
	 */

	/* make sure that the event is relevant */
	if (evt.type != PropertyNotify)
	    return (0);

	/* skip unless the property has a new value */
	if (evt.xproperty.state != PropertyNewValue)
	    return (0);

	/* check size and format of the property */
	XGetWindowProperty(dpy,
			   win,
			   pty,
			   0,
			   0,
			   False,
			   AnyPropertyType,
			   &pty_type,
			   &pty_format, &pty_items, &pty_size, (unsigned char **) &buffer);

	if (pty_format != 8) {
	    /* property does not contain text, delete it
	     * to tell the other X client that we have read
	     * it and to send the next property
	     */
	    XFree(buffer);
	    XDeleteProperty(dpy, win, pty);
	    return (0);
	}

	if (pty_size == 0) {
	    /* no more data, exit from loop */
	    XFree(buffer);
	    XDeleteProperty(dpy, win, pty);
	    *context = XCLIB_XCOUT_NONE;

	    /* this means that an INCR transfer is now
	     * complete, return 1
	     */
	    return (1);
	}

	XFree(buffer);

	/* if we have come this far, the propery contains
	 * text, we know the size.
	 */
	XGetWindowProperty(dpy,
			   win,
			   pty,
			   0,
			   (long) pty_size,
			   False,
			   AnyPropertyType,
			   &pty_type,
			   &pty_format, &pty_items, &pty_size, (unsigned char **) &buffer);

	/* allocate memory to accommodate data in *txt */
	if (*len == 0) {
	    *len = pty_items;
	    ltxt = (unsigned char *) malloc(*len);
	}
	else {
	    *len += pty_items;
	    ltxt = (unsigned char *) realloc(ltxt, *len);
	}

	/* add data to ltxt */
	memcpy(&ltxt[*len - pty_items], buffer, pty_items);

	*txt = ltxt;
	XFree(buffer);

	/* delete property to get the next item */
	XDeleteProperty(dpy, win, pty);
	XFlush(dpy);
	return (0);
    }

    return (0);
}

std::string GetClipBoardString() {
    Window win;			/* Window */
	Display *dpy;			/* connection to X11 display */

    /* Connect to the X server. */
    if ((dpy = XOpenDisplay(nullptr))) {
    }
    else {
		LPLOG("Failed to open display");
		return "";
    }

    /* Create a window to trap events */
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);

    /* get events about property changes */
    XSelectInput(dpy, win, PropertyChangeMask);
    unsigned char *sel_buf;	/* buffer for selection data */
    unsigned long sel_len = 0;	/* length of sel_buf */
    unsigned int context = XCLIB_XCOUT_NONE;
    XEvent evt;			/* X Event Structures */

	while (1) {
	    /* only get an event if xcout() is doing something */
	    if (context != XCLIB_XCOUT_NONE)
		XNextEvent(dpy, &evt);

	    /* fetch the selection, or part of it */
	    xcout(dpy, win, evt, sseln, target, &sel_buf, &sel_len, &context);

	    /* fallback is needed. set XA_STRING to target and restart the loop. */
	    if (context == XCLIB_XCOUT_FALLBACK) {
		context = XCLIB_XCOUT_NONE;
		target = XA_STRING;
		continue;
	    }

	    /* only continue if xcout() is doing something */
	    if (context == XCLIB_XCOUT_NONE)
		break;
	}

	std::string ret = "";
    if (sel_len) {
		LPLOG("len %d: %s", sel_len, sel_buf);
		ret = (char *)sel_buf;
		free(sel_buf);
    } else {
    	LPLOG("No selection");
    }

    /* Disconnect from the X server */
    XCloseDisplay(dpy);
    return ret;
}

};
#endif // _WIN32
