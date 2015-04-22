/*
 * =============================================================================
 *
 *       Filename:  cpm_node.h
 *        Created:  04/22/15 10:32:19
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPM_NODE_H__
#define  __CPM_NODE_H__

#include "cpm_address.h"
#include <memory>
#include <alpha/net_address.h>
#include <alpha/tcp_connection.h>

namespace cpm {
    class Message;
    class MessageCacheQueue;
    class Node {
        public:
            enum class ResolveState {
                kResolving = 1,
                kResolved = 2,
                kNotResolved = 3
            };
            Node(cpm::Address::NodeAddressType addr);
            ~Node();

            void SendMessage(const Message& new_message, Message* droped = nullptr);
            void CacheMessage(const Message& new_message, Message* droped = nullptr);
            void DropMessage();

            bool Connected() const;
            bool Connecting() const;
            bool Resolving() const;
            bool Resolved() const;
            bool NotResolved() const;
            bool HasCachedMessage() const;
            alpha::NetAddress NetAddress() const;
            void SetNetAddress(const alpha::NetAddress& net_address);
            void SetConnection(alpha::TcpConnectionPtr & conn);
            void SetConnecting(bool connecting);
            void SetResolveState(ResolveState state);
            void ClearConnection();

        private:
            static const int kMaxCachedMessageNum = 1 << 7;
            bool SendMessageDirectly(const Message& new_message, 
                    alpha::TcpConnectionPtr& conn);
            bool FlushMessage(alpha::TcpConnectionPtr & conn);
            bool connecting_;
            ResolveState resolve_state_;
            const cpm::Address::NodeAddressType node_address_;
            std::unique_ptr<alpha::NetAddress> net_address_;
            std::unique_ptr<MessageCacheQueue> messages_to_deliver_;
            alpha::TcpConnectionWeakPtr weak_conn_;
    };
}

#endif   /* ----- #ifndef __CPM_NODE_H__  ----- */
