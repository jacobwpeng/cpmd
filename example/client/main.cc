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

#include <libgen.h>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <iostream>
#include <alpha/logger.h>
#include <alpha/time_util.h>
#include "cpmd.h"

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0]);
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " filename\n";
        return -1;
    }

    char* name = ::basename(argv[0]);

    cpmd::PortMapperClient* client = cpmd::PortMapperClient::Create(name);
    if (client == nullptr) {
        std::cout << "Create PortMapperClient failed\n";
        return -2;
    } else {
        cpmd::Address self_addr;
        cpmd::Status status = client->Whereis(name, &self_addr);
        if (!status.ok()) {
            std::cout << "NameLookup failed, " << status.ToString() << '\n';
            return -3;
        }
        std::cout << "self_addr = " << self_addr.HashCode() << '\n';

        FILE* fp = ::fopen(argv[1], "r");
        if (fp == nullptr) {
            std::cout << "fopen failed\n";
            return -4;
        }
        cpmd::Message send;
        send.addr = self_addr.HashCode();
        bool done = false;
        int total = 0;
        std::string original;
        while (!feof(fp)) {
            const size_t kEachCount = 100;
            size_t n = ::fread(send.data, 1, kEachCount, fp);
            if (n != 0) {
                send.len = n;
                status = client->SendMessage(send);
                assert (status.ok());
                total += n;
                original.append(send.data, n);
            }
        }
        ::fclose(fp);
        auto recv_total = 0;
        auto timeout = 1; // 2s
        auto last_receive_time = alpha::NowInSeconds();
        std::string received;
        do {
            cpmd::Message * recv = nullptr;
            status = client->ReceiveMessage(&recv);
            if (status.ok()) {
                recv_total += recv->len;
                received.append(recv->data, recv->len);
                last_receive_time = alpha::NowInSeconds();
            }
        } while (last_receive_time + timeout > alpha::NowInSeconds());
        assert (received.size() == original.size());
        assert (::memcmp(received.data(), original.data(), original.size()) == 0);
    }
}
