/*
 * ==============================================================================
 *
 *       Filename:  cpm_options.h
 *        Created:  04/11/15 17:50:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_OPTIONS_H__
#define  __CPM_OPTIONS_H__

namespace cpm {
    class Options {
        public:
            Options();

            Options& SetServerPort(int port);
            Options& SetBufferSize(int size);

            int port() const;
            int buffer_size() const;

        private:
            int port_;
            int buffer_size_;
    };
}

#endif   /* ----- #ifndef __CPM_OPTIONS_H__  ----- */
