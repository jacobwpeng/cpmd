/*
 * ==============================================================================
 *
 *       Filename:  cpm_main.cc
 *        Created:  04/11/15 21:58:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <unistd.h>
#include <vector>
#include <gflags/gflags.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include "cpm_server.h"

DEFINE_bool(daemon, false, "Run as daemon");

bool HandlerSignals(alpha::EventLoop* loop, std::initializer_list<int> signals,
        const alpha::EventLoop::Functor& functor) {
    bool ok = true;
    std::for_each(signals.begin(), signals.end(), [loop, &ok, &functor](int signo){
            ok = ok && loop->TrapSignal(signo, functor);
    });
    return ok;
}

int main(int argc, char* argv[]) {
    const char* name = argv[0];
    gflags::SetUsageMessage("Cplusplus Port Mapper Daemon");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_daemon) {
        daemon(0, 0);
        alpha::Logger::Init(name);
    } else {
        alpha::Logger::Init(name, alpha::Logger::LogToStderr);
    }
    alpha::EventLoop loop;
    cpm::Server server(&loop, "/tmp");
    //忽略以下信号
    HandlerSignals(&loop, {SIGPIPE}, [&loop]{ loop.Quit(); });
    //以下信号退出
    HandlerSignals(&loop, {SIGINT, SIGQUIT, SIGTERM}, [&loop]{ loop.Quit(); });

    if (server.Run()) {
        loop.Run();
    }
    return EXIT_SUCCESS;
}
