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

#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include "cpm_server.h"

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);
    alpha::EventLoop loop;
    cpm::Server server(&loop, "/tmp");
    if (server.Run()) {
        loop.Run();
    }
    return EXIT_SUCCESS;
}
