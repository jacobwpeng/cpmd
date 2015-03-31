/*
 * =============================================================================
 *
 *       Filename:  port_mapper_main.cc
 *        Created:  03/30/15 14:43:06
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <functional>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include "port_mapper_command_server.h"
#include "port_mapper_message_dispatcher.h"

int main(int argc, char* argv[]) {
    using namespace std::placeholders;
    alpha::Logger::Init(argv[0]);
    alpha::EventLoop loop;

    cpmd::MessageDispatcher message_dispatcher;
    loop.set_period_functor(std::bind(&cpmd::MessageDispatcher::Routine, 
                &message_dispatcher, _1));
    cpmd::CommandServer command_server(&loop, &message_dispatcher);
    int ret = command_server.Init("");
    if (ret != 0) {
        LOG_ERROR << "Init command server failed, ret = " << ret;
        return ret;
    }

    command_server.Run();
    loop.Run();
    return 0;
}
