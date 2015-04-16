/*
 * =============================================================================
 *
 *       Filename:  cpm_resolve_server_conf.h
 *        Created:  04/15/15 14:54:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPM_RESOLVE_SERVER_CONF_H__
#define  __CPM_RESOLVE_SERVER_CONF_H__

#include <string>
#include <memory>
#include <map>
#include <alpha/slice.h>
#include <alpha/net_address.h>
#include "cpm_address.h"

namespace cpm {
    class ResolveServerConf {
        private:
            class Node {
                public:
                    Node(alpha::Slice readable_addr, alpha::Slice ip, int port);
                    cpm::Address::NodeAddressType node_addr() const {
                        return node_addr_;
                    }

                    std::string readable_addr() const {
                        return readable_addr_;
                    }

                    alpha::NetAddress net_addr() const {
                        return net_addr_;
                    }

                private:
                    cpm::Address::NodeAddressType node_addr_;
                    std::string readable_addr_;
                    alpha::NetAddress net_addr_;
            };
        public:
            static std::unique_ptr<ResolveServerConf> CreateFromFile(const char* file);
            const alpha::NetAddress* service_address() const {
                return service_address_.get();
            }

            const alpha::NetAddress* monitor_address() const {
                return monitor_address_.get();
            }

            const Node* GetNodeInfoByNetAddress(
                    const alpha::NetAddress& net_address) const;
            const Node* GetNodeInfoByNodeAddress(
                    cpm::Address::NodeAddressType node_address) const;

        private:
            ResolveServerConf() = default;
            std::unique_ptr<alpha::NetAddress> service_address_;
            std::unique_ptr<alpha::NetAddress> monitor_address_;
            std::map<cpm::Address::NodeAddressType, Node> nodes_;
            std::map<std::pair<std::string, int>, Node*> cache_nodes_;
    };
}

#endif   /* ----- #ifndef __CPM_RESOLVE_SERVER_CONF_H__  ----- */
