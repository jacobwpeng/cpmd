/*
 * =============================================================================
 *
 *       Filename:  cpm_resolve_server_main.cc
 *        Created:  04/15/15 15:08:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include "cpm_resolve_server.h"

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);
    if (argc != 2) {
        LOG_ERROR << "Usage: " << argv[0] << " conf";
        return EXIT_FAILURE;
    }

    alpha::EventLoop loop;
    cpm::ResolveServer resolve_server(&loop, argv[1]);
    if (resolve_server.Run()) {
        loop.Run();
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
