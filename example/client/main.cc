/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  03/27/15 15:18:36
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>
#include "cpmd.h"
#include "alpha/logger.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " filename\n";
        return -1;
    }
    alpha::Logger::Init(argv[0]);
    cpmd::PortMapperClient* client = cpmd::PortMapperClient::Create(argv[0]);
    if (client == nullptr) {
        std::cout << "Create PortMapperClient failed\n";
        return -2;
    } else {
        cpmd::Address self_addr;
        cpmd::Status status = client->Whereis(argv[0], &self_addr);
        if (!status.ok()) {
            std::cout << "NameLookup failed, " << status.ToString() << '\n';
            return -3;
        }

        FILE* fp = ::fopen(argv[1], "r");
        if (fp == nullptr) {
            std::cout << "fopen failed\n";
            return -4;
        }
        cpmd::Message send;
        send.addr = self_addr.HashCode();
        bool done = false;
        while (done) {
            const size_t kEachCount = 100;
            size_t n = ::fread(send.data, 1, kEachCount, fp);
            if (n != kEachCount) {
                if (::feof(fp)) {
                    done = true;
                } else {
                    std::cout << "fread failed\n";
                    return -5;
                }
            }
            if (n != 0) {
                send.len = n;
                status = client->SendMessage(send);

                cpmd::Message* recv = nullptr;
                status = client->ReceiveMessage(&recv);
                if (!status.ok()) {
                    std::cout << "ReceiveMessage failed\n";
                    return -6;
                }
                assert (recv->len == send.len);
                assert (::memcmp(recv->data, send.data, send.len) == 0);
            }
        }
    }
}
