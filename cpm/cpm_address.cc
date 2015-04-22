/*
 * ==============================================================================
 *
 *       Filename:  cpm_address.cc
 *        Created:  04/11/15 11:33:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_address.h"
#include <cassert>
#include <string>
#include <type_traits>

static_assert (std::is_pod<cpm::Address>::value, "Address must be POD type");

namespace cpm {
    Address Address::Default() {
        Address addr;
        addr.Clear();
        return addr;
    }

    Address Address::Create(alpha::Slice readable_address) {
        alpha::Slice client_addr;
        Address addr;
        if (readable_address.find("/") == alpha::Slice::npos) {
            // 本地客户端地址
            addr.node_addr_ = 0;
            addr.client_addr_ = Hash(readable_address);
            return addr;
        } else if (readable_address.ToString().back() == '/') {
            // 节点地址
            addr.node_addr_ = Hash(readable_address);
            addr.client_addr_ = 0;
            return addr;
        } else {
            auto node_addr = GetNodeAddress(readable_address, &client_addr);
            addr.node_addr_ = Hash(node_addr);
            addr.client_addr_ = Hash(client_addr);
            return addr;
        }
    }

    alpha::Slice Address::GetNodeAddress(alpha::Slice address, alpha::Slice* left) {
        assert (!address.empty());
        assert (left != nullptr);
        std::string addr = address.ToString();
        assert (!addr.empty() && addr.back() != '/');
        auto pos = addr.rfind('/');
        *left = alpha::Slice(address.data() + pos + 1, address.size() - pos - 1);
        return alpha::Slice(address.data(), pos + 1);
    }

    Address Address::CreateDirectly(NodeAddressType node_addr, 
            ClientAddressType client_addr) {
        Address addr;
        addr.node_addr_ = node_addr;
        addr.client_addr_ = client_addr;
        return addr;
    }

    uint32_t Address::Hash(alpha::Slice addr) {
        // MurmurHash 32-bit
        static const uint32_t c1 = 0xcc9e2d51;
        static const uint32_t c2 = 0x1b873593;
        static const uint32_t r1 = 15;
        static const uint32_t r2 = 13;
        static const uint32_t m = 5;
        static const uint32_t n = 0xe6546b64;

        auto key = addr.data();
        uint32_t len = addr.size();
        static const uint32_t seed = 0xEE6B27EB;
     
        uint32_t hash = seed;
     
        const int nblocks = len / 4;
        const uint32_t *blocks = (const uint32_t *) key;
        int i;
        for (i = 0; i < nblocks; i++) {
            uint32_t k = blocks[i];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
     
            hash ^= k;
            hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
        }
     
        const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
        uint32_t k1 = 0;
     
        switch (len & 3) {
            case 3:
                k1 ^= tail[2] << 16;
            case 2:
                k1 ^= tail[1] << 8;
            case 1:
                k1 ^= tail[0];

                k1 *= c1;
                k1 = (k1 << r1) | (k1 >> (32 - r1));
                k1 *= c2;
                hash ^= k1;
        }
     
        hash ^= len;
        hash ^= (hash >> 16);
        hash *= 0x85ebca6b;
        hash ^= (hash >> 13);
        hash *= 0xc2b2ae35;
        hash ^= (hash >> 16);
        return hash;
    }

    uint32_t Address::NodeAddress() const {
        return node_addr_;
    }

    uint32_t Address::ClientAddress() const {
        return client_addr_;
    }

    void Address::Clear() { 
        node_addr_ = client_addr_ = 0;
    }

    std::string Address::ToString() const {
        char buf[100];
        snprintf(buf, sizeof(buf), "Node(%u), Client(%u)", node_addr_, client_addr_);
        return buf;
    }
}
