/*
 * =============================================================================
 *
 *       Filename:  cpm_resolve_server_conf.cc
 *        Created:  04/15/15 15:05:19
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_resolve_server_conf.h"
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <alpha/logger.h>

namespace cpm {
    ResolveServerConf::Node::Node(alpha::Slice readable_addr, alpha::Slice ip, int port)
        :readable_addr_(readable_addr.ToString()), net_addr_(ip, port) {
            node_addr_ = Address::Create(readable_addr).NodeAddress();
    }

    std::unique_ptr<ResolveServerConf> ResolveServerConf::CreateFromFile(const char* file) {
        using namespace boost::property_tree;
        ptree pt;
        std::unique_ptr<ResolveServerConf> conf(new ResolveServerConf);
        try {
            DLOG_INFO << "file = " << file;
            read_xml(file, pt, xml_parser::no_comments);

            // interface
            for (const auto & child : pt.get_child("interface")) {
                if (child.first != "server") {
                    LOG_WARNING << "Unknown node " << child.first << " in interface";
                    continue;
                }

                const auto& subtree = child.second;
                auto type = subtree.get<std::string>("<xmlattr>.type");
                if (type == "tcp") {
                    if (conf->service_address_) {
                        LOG_ERROR << "Multiple tcp type server in conf";
                        return nullptr;
                    }
                    auto ip = subtree.get<std::string>("<xmlattr>.ip");
                    auto port = subtree.get<int>("<xmlattr>.port");
                    conf->service_address_.reset(new alpha::NetAddress(ip, port));
                } else if (type == "http") {
                    if (conf->monitor_address_) {
                        LOG_ERROR << "Multiple http type server in conf";
                        return nullptr;
                    }
                    auto ip = subtree.get<std::string>("<xmlattr>.ip");
                    auto port = subtree.get<int>("<xmlattr>.port");
                    conf->monitor_address_.reset(new alpha::NetAddress(ip, port));
                } else {
                    LOG_WARNING << "Unknown server type = " << type;
                    continue;
                }
            }

            if (!conf->service_address_) {
                LOG_ERROR << "No service address(type = tcp) in conf";
                return nullptr;
            }

            for (const auto& child : pt.get_child("nodes")) {
                if (child.first != "node") {
                    LOG_WARNING << "Unknown node " << child.first << " in nodes";
                    continue;
                }

                const auto & subtree = child.second;
                auto readable_addr = subtree.get<std::string>("<xmlattr>.addr");
                auto ip = subtree.get<std::string>("<xmlattr>.ip");
                auto port = subtree.get<int>("<xmlattr>.port");
                Node node(readable_addr, ip, port);
                LOG_INFO << "readable_address = " << readable_addr
                    << ", node_address = " << node.node_addr();
                auto it = conf->nodes_.find(node.node_addr());
                if (it != conf->nodes_.end()) {
                    LOG_ERROR << "Multiple node with same addr, addr = "
                        << readable_addr << ", " << node.net_addr();
                    return nullptr;
                }
                auto res = conf->nodes_.emplace(node.node_addr(), node);
                assert (res.second);
                conf->cache_nodes_.emplace(std::make_pair(ip, port), &res.first->second);
            }

        } catch(ptree_error& e) {
            LOG_ERROR << "CreateFromFile failed, file = " << file << ", " << e.what();
            return nullptr;
        }
        return std::move(conf);
    }

    const ResolveServerConf::Node* ResolveServerConf::GetNodeInfoByNetAddress(
            const alpha::NetAddress& net_address) const {
        auto it = cache_nodes_.find(std::make_pair(net_address.ip(), net_address.port()));
        return it == cache_nodes_.end() ? nullptr : it->second;
    }

    const ResolveServerConf::Node* ResolveServerConf::GetNodeInfoByNodeAddress(
            cpm::Address::NodeAddressType node_address) const {
        auto it = nodes_.find(node_address);
        return it == nodes_.end() ? nullptr : &it->second;
    }
}
