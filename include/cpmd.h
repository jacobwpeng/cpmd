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

namespace cpmd {
    class Status {
        private:
            enum class Code {
                kOk = 0,
                kTimeout = 1,
                kInvalidAddress = 2
            };

        public:
            Status(Status::Code code = Code::kOk);
            static Status OK();
            static Status Timeout(const char* msg);
            static Status InvalidAddress(const char* msg);

            bool ok() const { return code_ == Code::kOk; }
            bool IsTimeout() const { return code_ == Code::kTimeout; }
            bool IsInvalidAddress() const { return code_ == Code::kInvalidAddress; }

            std::string ToString() const;

        private:
            Code code_;
            const char* msg_;
    };

    struct Options {
    };

    struct Address {
    };

    class PortMapperClient {
        public:
            static PortMapperClient* Create(const char* name, Options options = Options());
            static void Destroy(PortMapperClient* client);
            virtual ~PortMapperClient();
            virtual Status Send(const Address* addr, const char* data, std::size_t len) = 0;
            virtual Status Receive(Address** addr, const char** data, std::size_t* len) = 0;
            virtual Status Whereis(const char* name, Address** addr) = 0;
    };
}

#endif   /* ----- #ifndef __CPMD_H__  ----- */
