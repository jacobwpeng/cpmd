/*
 * =============================================================================
 *
 *       Filename:  chat_server.cc
 *        Created:  04/23/15 15:21:35
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <unistd.h>
#include <map>
#include <alpha/logger.h>
#include <cpm/cpm.h>
#include "chat_protocol.h"

using MemberMap = std::map<std::string, cpm::Address>;

bool BroadcastMessage(cpm::Client* client, const MemberMap& members, cpm::Address from, 
        cpm::Message* m) {
    for (const auto& p : members) {
        if (p.second != from) {
            m->SetRemoteAddress(p.second);
            auto status = client->SendMessage(m);
            if (status != cpm::Status::kOk) {
                LOG_ERROR << "SendMessage failed, status = "
                    << static_cast<int>(status);
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0]);
    cpm::Client * client;
    auto status = cpm::Client::Create(&client, "chatroom");
    if (status != cpm::Status::kOk) {
        LOG_ERROR << "Create client failed, status = " << static_cast<int>(status);
        return EXIT_FAILURE;
    }
    MemberMap members;

    int idle = 0;
    while (true) {
        cpm::Message * m;
        status = client->ReceiveMessage(&m);
        if (status != cpm::Status::kOk && status != cpm::Status::kBusEmpty) {
            LOG_ERROR << "ReceiveMessage failed, status = " << static_cast<int>(status);
            return EXIT_FAILURE;
        } else if (status == cpm::Status::kBusEmpty) {
            ++idle;
        } else {
            idle = 0;
            alpha::Slice data = m->Data();
            auto chat_message = data.as<chat::Message>();
            std::string member_name = chat_message->name;
            auto words = chat_message->words;
            auto to_member = chat_message->to_name;
            auto from = m->SourceAddress();
            auto reply_message = cpm::Message::Default();
            auto chat_reply_message = chat::Message();
            MemberMap::iterator it;
            chat::Message::Builder builder;
            switch (chat_message->type) {
                case chat::MessageType::kRegister:
                    if (members.find(member_name) == members.end()) {
                        LOG_INFO << "New register, name = " << member_name;
                        members.emplace(member_name, from);
                    } else {
                        LOG_WARNING << "Already registered, name = " << member_name;
                    }
                    break;
                case chat::MessageType::kHeartBeat:
                    LOG_INFO << "Heart beat from " << member_name;
                    break;
                case chat::MessageType::kMessage:
                    LOG_INFO << "New words from " << member_name
                        << ", " << words;
                    builder.SetType(chat::MessageType::kMessage)
                        .SetName(member_name)
                        .SetWords(words);
                    chat_reply_message = builder.Finish();
                    reply_message.SetData(alpha::Slice(&chat_reply_message));
                    if (!BroadcastMessage(client, members, from, &reply_message)) {
                        return EXIT_FAILURE;
                    }
                    break;
                case chat::MessageType::kPrivateMessage:
                    LOG_INFO << "Private words from " << member_name
                        << " to " << to_member << ", " << words;
                    it = members.find(to_member);
                    builder.SetType(chat::MessageType::kPrivateMessage)
                        .SetName(member_name)
                        .SetWords(words);
                    if (it != members.end()) {
                        chat_reply_message = builder.Finish();
                        reply_message.SetData(alpha::Slice(&chat_reply_message));
                        status = client->SendMessage(&reply_message);
                        if (status != cpm::Status::kOk) {
                            LOG_ERROR << "SendMessage failed, status = " << 
                                static_cast<int>(status);
                            return EXIT_FAILURE;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        if (idle >= 1000) {
            ::usleep(20 * 1000);
        }
    }

    return EXIT_SUCCESS;
}
