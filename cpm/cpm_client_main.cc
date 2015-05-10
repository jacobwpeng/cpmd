/*
 * ==============================================================================
 *
 *       Filename:  cpm_client_main.cc
 *        Created:  04/11/15 23:55:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <libgen.h>
#include <cassert>
#include <alpha/logger.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "cpm.h"

int Usage(char* progname) {
    std::cout << "Usage: " << progname << " register_server_port[default = 8123]"
        << "\nname[default = " << basename(progname) << "]"
        << "\ntarget[default = " << basename(progname) << "]"
        << "\nmessage_num[default = 100]"
        << '\n';
    return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
    if (argc > 5) {
        return Usage(argv[0]);
    }

    int register_server_port = 8123;
    if (argc >= 2) {
        register_server_port = boost::lexical_cast<int>(argv[1]);
    }

    const char* name = basename(argv[0]);
    if (argc >= 3) {
        name = argv[2];
    }

    const char* target = basename(argv[0]);
    if (argc >= 4) {
        target = argv[3];
    }

    int message_num = 100;
    if (argc >= 5) {
        message_num = boost::lexical_cast<int>(argv[4]);
    }

    cpm::Options options;
    options.SetServerPort(register_server_port);
    alpha::Logger::Init(argv[0]);
    LOG_INFO << "register_server_port = " << register_server_port
        << ", name = " << name 
        << ", target = " << target;

    cpm::Client * client = nullptr;
    auto status = cpm::Client::Create(&client, name, options);
    const std::string msg = "Long live the queen!";
    if (status != cpm::Status::kOk) {
        LOG_ERROR << "Create failed, status = " << static_cast<int>(status);
        return EXIT_FAILURE;
    } else {
        char c;
        LOG_INFO << "Waiting for user input";
        std::cin >> c;
        auto target_address = cpm::Address::Create(target);
        cpm::Message m = cpm::Message::Default();
        m.SetRemoteAddress(target_address);
        m.SetData(msg);
        int num = 0;
        while (num < message_num) {
            status = client->SendMessage(&m);
            if (status != cpm::Status::kOk && status != cpm::Status::kBusFull) {
                LOG_ERROR << "SendMessage failed, status = " << static_cast<int>(status);
                return EXIT_FAILURE;
            } else if (status == cpm::Status::kBusFull) {
                break;
            } else {
                ++num;
            }
        }

        LOG_INFO << "Send " << num << " messages";

        int loop_times = 0;
        while (1) {
            cpm::Message* m = nullptr;
            status = client->ReceiveMessage(&m);
            if (status == cpm::Status::kOk) {
                assert (m->Data() == msg);
                --num;
                if (num == 0) {
                    break;
                }
            } else if (status == cpm::Status::kBusEmpty) {
                if (++loop_times % 100000 == 0) {
                    //LOG_INFO << "Bus empty, num = " << num 
                    //    << ", loop_times = " << loop_times;
                    ::usleep(100000);
                }
                /* wait */
            } else {
                LOG_ERROR << "ReceiveMessage failed, status = " << static_cast<int>(status);
                return EXIT_FAILURE;
            }
        }
        return EXIT_SUCCESS;
    }
}
