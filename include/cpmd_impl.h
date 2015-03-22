/*
 * ==============================================================================
 *
 *       Filename:  cpmd_impl.h
 *        Created:  03/22/15 16:06:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPMD_IMPL_H__
#define  __CPMD_IMPL_H__

#include "cpmd.h"

namespace cpmd {
    class PortMapperClientImpl : public PortMapperClient {
        public:
            PortMapperClientImpl();
            virtual ~PortMapperClientImpl();
            virtual Status Send(const Address* addr, const char* data, std::size_t len);
            virtual Status Receive(Address** addr, const char** data, std::size_t* len);
            virtual Status Whereis(const char* name, Address** addr);
    };
}

#endif   /* ----- #ifndef __CPMD_IMPL_H__  ----- */
