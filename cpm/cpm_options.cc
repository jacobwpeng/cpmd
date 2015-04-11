/*
 * ==============================================================================
 *
 *       Filename:  cpm_options.cc
 *        Created:  04/11/15 17:52:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_options.h"

namespace cpm {
    static const int kDefaultServerPort = 8123;
    static const int kDefaultBufferSize = 1 << 20; //1 MiB
    Options::Options()
        :port_(8123), buffer_size_(kDefaultBufferSize) {
    }

    Options& Options::SetServerPort(int port) {
        port_ = port;
        return *this;
    }

    Options& Options::SetBufferSize(int size) {
        buffer_size_ = size;
        return *this;
    }

    int Options::port() const {
        return port_;
    }

    int Options::buffer_size() const {
        return buffer_size_;
    }
}
