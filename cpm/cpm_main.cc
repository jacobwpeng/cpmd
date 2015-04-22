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
#include <gflags/gflags.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include "cpm_server.h"

DEFINE_bool(daemon, false, "Run as daemon");

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
    if (server.Run()) {
        loop.Run();
    }
    return EXIT_SUCCESS;
}
