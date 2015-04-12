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
#include "cpm.h"

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);
    cpm::Client * client = nullptr;
    char* basename = ::basename(argv[0]);
    auto status = cpm::Client::Create(&client, basename);
    const std::string msg = "Long live the queen!";
    if (status != cpm::Status::kOk) {
        LOG_ERROR << "Create failed, status = " << static_cast<int>(status);
        return EXIT_FAILURE;
    } else {
        auto self_addr = cpm::Address::Create(basename);
        cpm::Message m = cpm::Message::Default();
        m.SetRemoteAddress(self_addr);
        m.SetData(msg);
        int num = 0;
        while (1) {
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

        while (1) {
            cpm::Message* m = nullptr;
            status = client->ReceiveMessage(&m);
            if (status == cpm::Status::kOk) {
                LOG_INFO << "Receive " << 1 << " message";
                assert (m->Data() == msg);
                --num;
                if (num == 0) {
                    break;
                }
            } else if (status == cpm::Status::kBusEmpty) {
                LOG_INFO << "Bus empty";
                /* wait */
            } else {
                LOG_ERROR << "ReceiveMessage failed, status = " << static_cast<int>(status);
                return EXIT_FAILURE;
            }
        }
        return EXIT_SUCCESS;
    }
}
