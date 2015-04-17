/*
 * ==============================================================================
 *
 *       Filename:  cpm_address.h
 *        Created:  04/11/15 11:31:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_ADDRESS_H__
#define  __CPM_ADDRESS_H__

#include <alpha/slice.h>
#include <cstdint>

namespace cpm {
    class Address {
        public:
            using NodeAddressType = uint32_t;
            using ClientAddressType = uint32_t;
            static const NodeAddressType kLocalNodeAddress = 0;

            static Address Default();
            static Address Create(alpha::Slice readable_address);

            NodeAddressType NodeAddress() const;
            ClientAddressType ClientAddress() const;

            void Clear();

        private:
            static uint32_t Hash(alpha::Slice address);
            static alpha::Slice GetNodeAddress(alpha::Slice address, alpha::Slice* left);
            static Address CreateDirectly(NodeAddressType node_addr, 
                    ClientAddressType client_addr);
            friend class Message;
            friend class Server;
            Address() = default;
            NodeAddressType node_addr_;
            ClientAddressType client_addr_;
    };
}
#endif   /* ----- #ifndef __CPM_ADDRESS_H__  ----- */

