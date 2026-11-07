// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
using namespace globe;
Window find(Display* d, const char* title) {
    Window root, parent, *children = nullptr;
    unsigned count = 0;
    Window found = 0;
    if (XQueryTree(d, DefaultRootWindow(d), &root, &parent, &children, &count)) {
        for (unsigned i = 0; i < count; ++i) {
            char* name = nullptr;
            if (XFetchName(d, children[i], &name) && name) {
                if (std::strcmp(name, title) == 0)
                    found = children[i];
                XFree(name);
            }
        }
        if (children)
            XFree(children);
    }
    return found;
}
int main(int argc, char** argv) {
    CHECK(argc == 2 || argc == 3);
    if (argc != 2 && argc != 3)
        return 1;
    int out[2];
    CHECK(pipe(out) == 0);
    const pid_t pid = fork();
    if (pid == 0) {
        close(out[0]);
        dup2(out[1], STDOUT_FILENO);
        close(out[1]);
        execl(argv[1], argv[1], "--size", "160x120", "--stats", static_cast<char*>(nullptr));
        _exit(127);
    }
    close(out[1]);
    auto* d = XOpenDisplay(nullptr);
    CHECK(d);
    if (!d)
        return 1;
    Window main = 0, toolbar = 0;
    for (int i = 0; i < 200 && (!main || !toolbar); ++i) {
        main = find(d, "Globe");
        toolbar = find(d, "Globe tools");
        usleep(10000);
    }
    CHECK(main && toolbar);
    auto motion = [&](int x, int y) {
        XEvent e{};
        e.xmotion.type = MotionNotify;
        e.xmotion.display = d;
        e.xmotion.window = main;
        e.xmotion.x = x;
        e.xmotion.y = y;
        XSendEvent(d, main, False, PointerMotionMask, &e);
        XFlush(d);
        usleep(20000);
    };
    auto click = [&](Command command) {
        const auto r = toolbar_button(command);
        XEvent e{};
        e.xbutton.type = ButtonPress;
        e.xbutton.display = d;
        e.xbutton.window = toolbar;
        e.xbutton.button = Button1;
        e.xbutton.x = r.x + 2;
        e.xbutton.y = r.y + 2;
        XSendEvent(d, toolbar, False, ButtonPressMask, &e);
        XFlush(d);
        usleep(20000);
    };
    const bool repaint_test = argc == 3;
    if (main && toolbar && repaint_test) {
        usleep(1000000); // let the initial one-shot refinement finish
        XMoveWindow(d, main, 20, 20);
        XFlush(d);
        for (int i = 0; i < 10; ++i) {
            XEvent expose{};
            expose.xexpose.type = Expose;
            expose.xexpose.display = d;
            expose.xexpose.window = main;
            XSendEvent(d, main, False, ExposureMask, &expose);
            XFlush(d);
            usleep(20000);
        }
        XEvent close{};
        close.xclient.type = ClientMessage;
        close.xclient.display = d;
        close.xclient.window = main;
        close.xclient.message_type = XInternAtom(d, "WM_PROTOCOLS", False);
        close.xclient.format = 32;
        close.xclient.data.l[0] = static_cast<long>(XInternAtom(d, "WM_DELETE_WINDOW", False));
        XSendEvent(d, main, False, NoEventMask, &close);
        XFlush(d);
    } else if (main && toolbar) {
        motion(80, 60);
        motion(0, 0);
        motion(80, 60);
        XEvent press{};
        press.xbutton.display = d;
        press.xbutton.window = main;
        press.xbutton.button = Button1;
        press.xbutton.x = 90;
        press.xbutton.y = 60;
        press.xbutton.type = ButtonPress;
        XSendEvent(d, main, False, ButtonPressMask, &press);
        press.xbutton.type = ButtonRelease;
        XSendEvent(d, main, False, ButtonReleaseMask, &press);
        XFlush(d);
        usleep(700000); // the card waits out the double-click window
        const auto info = find(d, "Globe information");
        CHECK(info);
        if (info) {
            XEvent wheel{};
            wheel.xbutton.display = d;
            wheel.xbutton.window = info;
            wheel.xbutton.type = ButtonPress;
            wheel.xbutton.button = Button5;
            XSendEvent(d, info, False, ButtonPressMask, &wheel);
            XFlush(d);
            usleep(20000);
        }
        {
            // P opens the looks palette; clicking the Relief row switches the look.
            XEvent key{};
            key.xkey.type = KeyPress;
            key.xkey.display = d;
            key.xkey.window = main;
            key.xkey.keycode = XKeysymToKeycode(d, XK_p);
            XSendEvent(d, main, False, KeyPressMask, &key);
            XFlush(d);
            usleep(300000);
            const auto looks = find(d, "Globe looks");
            CHECK(looks);
            if (looks) {
                XEvent press{};
                press.xbutton.type = ButtonPress;
                press.xbutton.display = d;
                press.xbutton.window = looks;
                press.xbutton.button = Button1;
                press.xbutton.x = 20;
                press.xbutton.y = 8 + 30 + 10; // second row
                XSendEvent(d, looks, False, ButtonPressMask, &press);
                XFlush(d);
                usleep(100000);
            }
        }
        {
            // M starts measuring; two clicks give the distance card (the first only marks a point).
            XEvent key{};
            key.xkey.type = KeyPress;
            key.xkey.display = d;
            key.xkey.window = main;
            key.xkey.keycode = XKeysymToKeycode(d, XK_m);
            XSendEvent(d, main, False, KeyPressMask, &key);
            XFlush(d);
            usleep(100000);
            for (const int x : {60, 100}) {
                XEvent press{};
                press.xbutton.display = d;
                press.xbutton.window = main;
                press.xbutton.button = Button1;
                press.xbutton.x = x;
                press.xbutton.y = 60;
                press.xbutton.type = ButtonPress;
                XSendEvent(d, main, False, ButtonPressMask, &press);
                press.xbutton.type = ButtonRelease;
                XSendEvent(d, main, False, ButtonReleaseMask, &press);
                XFlush(d);
                usleep(700000); // each click waits out the double-click window
            }
        }
        click(Command::AlignNorth); // disabled in initial north-up state
        click(Command::ToggleGraticule);
        click(Command::ToggleGraticule);
        click(Command::Quit);
    } else {
        kill(pid, SIGTERM);
    }
    XCloseDisplay(d);
    std::string text;
    char buf[1024];
    ssize_t n;
    while ((n = read(out[0], buf, sizeof(buf))) > 0)
        text.append(buf, static_cast<std::size_t>(n));
    close(out[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    if (repaint_test) {
        CHECK(text.find("\"frames\": 1,") != std::string::npos);
        // The line layer alone, one composited frame per ring pass and for the limb pass, the end.
        const auto bands = kRefinePasses + 3;
        CHECK(text.find("\"refine_bands\": " + std::to_string(bands) + ",") != std::string::npos);
    } else {
        CHECK(text.find("\"toolbar_commands\": 3") != std::string::npos);
        CHECK(text.find("\"cursor_changes\": 9") != std::string::npos);
        CHECK(text.find("\"info_cards\": 2") != std::string::npos); // country card, then distance
        CHECK(text.find("\"info_scrolls\": 1") != std::string::npos);
        CHECK(text.find("\"terrain_style\": 1") != std::string::npos); // Relief, via the palette
    }
    if (test::failures())
        std::fprintf(stderr, "%s\n", text.c_str());
    return TEST_MAIN_RESULT();
}
