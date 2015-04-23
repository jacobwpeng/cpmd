/*
 * =============================================================================
 *
 *       Filename:  chat_client.cc
 *        Created:  04/23/15 17:00:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <alpha/logger.h>
#include <cpm/cpm.h>
#include "chat_protocol.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " chatroom_address name\n";
        return EXIT_FAILURE;
    }
    alpha::Logger::Init(argv[0], alpha::Logger::LogToStderr);

    auto chatroom_address = cpm::Address::Create(argv[1]);
    std::string name = argv[2];

    cpm::Client* client;
    auto status = cpm::Client::Create(&client, name);
    if (status != cpm::Status::kOk) {
        std::cout << "Create client failed, status = " << static_cast<int>(status) << '\n';
        return EXIT_FAILURE;
    }

    chat::Message::Builder builder;
    builder.SetType(chat::MessageType::kRegister)
        .SetName(name);

    auto chat_message = builder.Finish();
    cpm::Message m = cpm::Message::Default();
    m.SetRemoteAddress(chatroom_address);
    m.SetData(&chat_message);

    status = client->SendMessage(&m);
    if (status != cpm::Status::kOk) {
        std::cout << "SendMessage failed, status = " << static_cast<int>(status) << '\n';
        return EXIT_FAILURE;
    }

    fcntl (STDIN_FILENO, F_SETFL, fcntl (STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

    struct pollfd fds[1];
    const int kTimeout = 500;
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    while (1) {
        int ret = ::poll(fds, 1, kTimeout);
        if (ret > 0) {
            assert (fds[0].revents & POLLIN);
            char words[chat::kMaxWordsSize];
            auto n = read(STDIN_FILENO, words, sizeof(words));
            if (n == -1) {
                perror("read");
                return EXIT_FAILURE;
            }

            builder.SetType(chat::MessageType::kMessage)
                .SetName(name)
                .SetWords(std::string(words, n));
            chat_message = builder.Finish();
            m.SetData(&chat_message);

            status = client->SendMessage(&m);
            if (status != cpm::Status::kOk) {
                std::cout << "SendMessage failed, status = " 
                    << static_cast<int>(status) << '\n';
                return EXIT_FAILURE;
            }
        } else if (ret == 0) {
            //timeout
        } else {
            ::perror("poll");
            return EXIT_FAILURE;
        }
        cpm::Message* received_message;
        status = client->ReceiveMessage(&received_message);
        while (status != cpm::Status::kBusEmpty) {
            auto data = received_message->Data();
            const chat::Message* chat_message = data.as<chat::Message>();
            if (chat_message->type == chat::MessageType::kMessage) {
                std::cout << chat_message->name << ": " << chat_message->words;
            }
            status = client->ReceiveMessage(&received_message);
        }
    }
    return EXIT_SUCCESS;
}
