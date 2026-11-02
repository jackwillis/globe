// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include <X11/Xlib.h>
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
    CHECK(argc == 2);
    if (argc != 2)
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
    if (main && toolbar) {
        motion(80, 60);
        motion(0, 0);
        motion(80, 60);
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
    CHECK(text.find("\"toolbar_commands\": 3") != std::string::npos);
    CHECK(text.find("\"cursor_changes\": 3") != std::string::npos);
    if (test::failures())
        std::fprintf(stderr, "%s\n", text.c_str());
    return TEST_MAIN_RESULT();
}
