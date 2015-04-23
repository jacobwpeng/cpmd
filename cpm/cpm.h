/*
 * ==============================================================================
 *
 *       Filename:  cpm.h
 *        Created:  04/11/15 11:29:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <memory>
#include <alpha/slice.h>
#include "cpm_message.h"
#include "cpm_options.h"
//#include "cpm_status.h"
//#include "cpm_client.h"

#ifndef  __CPM_H__
namespace cpm {
    enum class Status {
        kOk = 0,
        kAlreadyInitialized = 1, //重复初始化
        kSocketError = 2, //与cpmd通信时出错
        kSocketTimeout = 3, //从cpmd接收回复时超时
        kInvalidBufferSize = 4, //cpmd拒绝请求的buffer大小
        kInvalidReply = 5, //与cpmd通信时接收到非法返回包
        kCreateBusFailed = 6, //无法创建和cpmd交换消息的bus
        kBusFull = 7,
        kBusEmpty = 8,
        kTruncatedMessage = 9, //bus中接收到被截断的消息
        kInvalidMessage = 10, //bus中接收到超长的消息
    };
    
    class Client {
        public:
            static Status Create(Client** client, alpha::Slice name, 
                    Options options = Options());
            static void Destroy(Client* client);
            virtual ~Client() = default;

            virtual Status SendMessage(const Message* m) = 0;
            virtual Status ReceiveMessage(Message** m) = 0;
    };
}
#define  __CPM_H__



#endif   /* ----- #ifndef __CPM_H__  ----- */
