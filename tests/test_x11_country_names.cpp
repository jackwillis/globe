// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
using namespace globe;
// The window named `title` among the root's children and their children (the sidebar panes are
// children of the one Globe window).
static Window find_in(Display* d, Window parent_window, const char* title, int depth) {
    Window root, parent, *children = nullptr;
    unsigned count = 0;
    Window found = 0;
    if (XQueryTree(d, parent_window, &root, &parent, &children, &count)) {
        for (unsigned i = 0; i < count && !found; ++i) {
            char* name = nullptr;
            if (XFetchName(d, children[i], &name) && name) {
                if (std::strcmp(name, title) == 0)
                    found = children[i];
                XFree(name);
            }
            if (!found && depth > 0)
                found = find_in(d, children[i], title, depth - 1);
        }
        if (children)
            XFree(children);
    }
    return found;
}
Window find(Display* d, const char* title) {
    return find_in(d, DefaultRootWindow(d), title, 1);
}
#include <X11/Xutil.h>
#include <cstdint>
std::uint64_t pixels(Display* d, Window w) {
    XSync(d, False);
    auto* image = XGetImage(d, w, 0, 0, 320, 240, AllPlanes, ZPixmap);
    CHECK(image);
    if (!image)
        return 0;
    std::uint64_t hash = 1469598103934665603ULL;
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x)
            hash = (hash ^ XGetPixel(image, x, y)) * 1099511628211ULL;
    XDestroyImage(image);
    return hash;
}
int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    const auto pid = fork();
    if (!pid) {
        execl(argv[1], argv[1], "--size", "320x240", static_cast<char*>(nullptr));
        _exit(127);
    }
    auto* d = XOpenDisplay(nullptr);
    if (!d)
        return 3;
    Window globe = 0, tools = 0;
    for (int i = 0; i < 200 && (!globe || !tools); ++i) {
        globe = find(d, "Poodle Earth");
        tools = find(d, "Poodle Earth tools");
        usleep(10000);
    }
    CHECK(globe && tools);
    if (!globe || !tools)
        return 4;
    XMoveWindow(d, globe, 0, 0);
    XMoveWindow(d, tools, 350, 0);
    XMapRaised(d, tools);
    XFlush(d);
    usleep(2000000);
    auto toggle = [&] {
        XEvent e{};
        e.xbutton.type = ButtonPress;
        e.xbutton.display = d;
        e.xbutton.window = tools;
        e.xbutton.button = Button1;
        e.xbutton.x = 20;
        e.xbutton.y = kToolbarHeight + 10;
        XSendEvent(d, tools, False, ButtonPressMask, &e);
        XFlush(d);
        usleep(150000);
    };
    const auto on = pixels(d, globe);
    toggle();
    const auto off = pixels(d, globe);
    CHECK(on != off);
    toggle();
    CHECK(pixels(d, globe) == on);
    // The City Names row is the second toggle: it changes the settled frame and restores it.
    auto toggle_cities = [&] {
        XEvent e{};
        e.xbutton.type = ButtonPress;
        e.xbutton.display = d;
        e.xbutton.window = tools;
        e.xbutton.button = Button1;
        e.xbutton.x = 20;
        e.xbutton.y = kToolbarHeight + 38;
        XSendEvent(d, tools, False, ButtonPressMask, &e);
        XFlush(d);
        usleep(150000);
    };
    // With the country names off no country label can take a city's place, so the toggle shows.
    toggle();
    const auto cities_on = pixels(d, globe);
    toggle_cities();
    CHECK(pixels(d, globe) != cities_on);
    toggle_cities();
    CHECK(pixels(d, globe) == cities_on);
    toggle();
    CHECK(pixels(d, globe) == on);
    XEvent press{};
    press.xbutton.type = ButtonPress;
    press.xbutton.display = d;
    press.xbutton.window = globe;
    press.xbutton.button = Button1;
    press.xbutton.x = 160;
    press.xbutton.y = 120;
    XSendEvent(d, globe, False, ButtonPressMask, &press);
    XFlush(d);
    usleep(40000);
    XEvent move{};
    move.xmotion.type = MotionNotify;
    move.xmotion.display = d;
    move.xmotion.window = globe;
    move.xmotion.state = Button1Mask;
    move.xmotion.x = 210;
    move.xmotion.y = 130;
    XSendEvent(d, globe, False, PointerMotionMask, &move);
    XFlush(d);
    usleep(300000);
    const auto moving = pixels(d, globe);
    toggle();
    CHECK(pixels(d, globe) == moving);
    toggle();
    CHECK(pixels(d, globe) == moving);
    press.xbutton.type = ButtonRelease;
    press.xbutton.x = 210;
    press.xbutton.y = 130;
    XSendEvent(d, globe, False, ButtonReleaseMask, &press);
    XFlush(d);
    usleep(2000000);
    const auto settled = pixels(d, globe);
    toggle();
    CHECK(pixels(d, globe) != settled);
    toggle();
    CHECK(pixels(d, globe) == settled);
    XEvent close{};
    close.xclient.type = ClientMessage;
    close.xclient.display = d;
    close.xclient.window = globe;
    close.xclient.message_type = XInternAtom(d, "WM_PROTOCOLS", False);
    close.xclient.format = 32;
    close.xclient.data.l[0] = static_cast<long>(XInternAtom(d, "WM_DELETE_WINDOW", False));
    XSendEvent(d, globe, False, NoEventMask, &close);
    XFlush(d);
    int status = 0;
    waitpid(pid, &status, 0);
    CHECK(status == 0);
    XCloseDisplay(d);
    std::printf("country-name settle/drag/toggle pixel smoke: %d failures\n", test::failures());
    return test::failures() ? 1 : 0;
}
