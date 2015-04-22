/*
 * =============================================================================
 *
 *       Filename:  cpm_node.cc
 *        Created:  04/22/15 10:40:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_node.h"
#include <alpha/logger.h>
#include "cpm_message.h"
#include "cpm_message_cache_queue.h"

namespace cpm {
    Node::Node(cpm::Address::NodeAddressType addr)
        :connecting_(false), resolve_state_(ResolveState::kNotResolved)
         , node_address_(addr) {
    }

    Node::~Node() = default;

    void Node::SendMessage(const Message& new_message, Message* droped) {
        if (auto conn = weak_conn_.lock()) {
            if (!conn->closed()) {
                if (SendMessageDirectly(new_message, conn)) {
                    return;
                }
            }
        }

        CacheMessage(new_message, droped);
    }

    void Node::CacheMessage(const Message& new_message, Message* droped) {
        if (unlikely(messages_to_deliver_ == nullptr)) {
            messages_to_deliver_.reset (new MessageCacheQueue(kMaxCachedMessageNum));
        }
        messages_to_deliver_->Push(new_message, droped);
    }

    void Node::DropMessage() {
        if (messages_to_deliver_ && !messages_to_deliver_->Empty()) {
            auto n = messages_to_deliver_->Size();
            LOG_INFO << "Drop " << n << " messages to node(" << node_address_ << ")";
        }
    }

    bool Node::Connected() const {
        auto conn = weak_conn_.lock();
        return conn && !conn->closed();
    }

    bool Node::Connecting() const {
        return connecting_;
    }

    bool Node::Resolving() const {
        return resolve_state_ == ResolveState::kResolving;
    }

    bool Node::Resolved() const {
        return resolve_state_ == ResolveState::kResolved;
    }

    bool Node::NotResolved() const {
        return resolve_state_ == ResolveState::kNotResolved;
    }

    bool Node::HasCachedMessage() const {
        return messages_to_deliver_ && !messages_to_deliver_->Empty();
    }

    alpha::NetAddress Node::NetAddress() const {
        assert (net_address_);
        return *net_address_;
    }

    void Node::SetNetAddress(const alpha::NetAddress& net_address) {
        net_address_.reset (new alpha::NetAddress(net_address));
        SetResolveState(ResolveState::kResolved);
    }

    void Node::SetConnection(alpha::TcpConnectionPtr & conn) {
        assert (conn && !conn->closed());
        if (HasCachedMessage()) {
            bool done = FlushMessage(conn);
            assert (done);
        }
        weak_conn_ = conn;
    }

    void Node::SetConnecting(bool connecting) {
        connecting_ = connecting;
    }

    void Node::SetResolveState(ResolveState state) {
        resolve_state_ = state;
    }

    void Node::ClearConnection() {
        weak_conn_.reset();
    }

    bool Node::FlushMessage(alpha::TcpConnectionPtr & conn) {
        assert (messages_to_deliver_);
        while (auto  m = messages_to_deliver_->Front()) {
            bool ok = SendMessageDirectly(*m, conn);
            if (!ok) break;
            messages_to_deliver_->PopFront();
        }
        LOG_INFO_IF(!HasCachedMessage()) << "messages_to_deliver_->Size() = "
            << messages_to_deliver_->Size();
        return !HasCachedMessage();
    }

    bool Node::SendMessageDirectly(const Message& new_message, 
            alpha::TcpConnectionPtr& conn) {
        return conn->Write(new_message.Serialize());
    }
}
