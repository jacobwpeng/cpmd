/*
 * ==============================================================================
 *
 *       Filename:  cpmd.h
 *        Created:  03/22/15 15:53:43
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPMD_H__
#define  __CPMD_H__

#include <cstdint>
#include <string>
#include <cpmd_protocol.h>
#include <alpha/slice.h>

namespace cpmd {
    const static std::size_t kMaxMessageDataSize = (1 << 16) - 100;

    struct Message {
        uint32_t addr;
        int32_t len;
        char data[kMaxMessageDataSize];
    };

    class Status {
        private:
            enum class Code {
                kOk = 0,
                kInitError,
                kInvalidAddress,
                kNoSpaceLeft,
                kNoMessage,
                kCommunicateError,
                kUnknwonError
            };

        public:
            Status(Status::Code code = Code::kOk);
            static Status OK();
            static Status InitError(const alpha::Slice& msg);
            static Status InvalidAddress(const alpha::Slice& msg);
            static Status NoSpace(const alpha::Slice& msg);
            static Status NoMessage();
            static Status CommunicateError(const alpha::Slice& msg);
            static Status Error(Status::Code code, const alpha::Slice& msg);

            bool ok() const { return code_ == Code::kOk; }
            bool IsInvalidAddress() const { return code_ == Code::kInvalidAddress; }
            bool IsNoMessage() const { return code_ == Code::kNoMessage; }

            std::string ToString() const;

        private:
            Code code_;
            std::string msg_;
    };

    struct Options {
        Options();
        int port;
        std::size_t buffer_size;
    };

    class Address {
        public:
            Address();
            uint32_t HashCode() const;
            bool Match(const alpha::Slice& full_address) const;
            static Address Create(const alpha::Slice& addr);
        private:
            explicit Address(uint32_t hash_code);
            friend class PortMapperClientImpl;
            uint32_t hash_code_;
    };

    class PortMapperClient {
        public:
            static PortMapperClient* Create(const char* name, Options options = Options());
            static void Destroy(PortMapperClient* client);
            virtual ~PortMapperClient();
            virtual Status Send(const Address& addr, const char* data, int len) = 0;
            virtual Status Receive(Address* addr, char** pdata, int* plen) = 0;

            virtual Status SendMessage(const Message& m) = 0;
            virtual Status ReceiveMessage(Message** m) = 0;

            virtual Status Whereis(const char* name, Address* addr) = 0;
    };
}

#endif   /* ----- #ifndef __CPMD_H__  ----- */
