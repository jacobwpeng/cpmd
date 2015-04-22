/*
 * =============================================================================
 *
 *       Filename:  cpm_protocol.cc
 *        Created:  04/15/15 19:02:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_protocol.h"

#include <cstring>

namespace cpm {
    int ProtocolMessage::size() const {
        assert (len >= 0);
        return offsetof(ProtocolMessage, data) + len;
    }

    alpha::Slice ProtocolMessage::Serialize() const {
        return alpha::Slice(reinterpret_cast<const char*>(this), size());
    }

    template<typename T>
    T& EnsureLength(T* cls, cpm::ProtocolMessage* m, size_t size) {
        if (m->len < size) {
            m->len = size;
        }
        return *cls;
    }

#define MinSizeWithField(Type, Field)                                          \
    offsetof(Type, Field) + sizeof(Type::Field)

#define DefineBuilderConstructor(Type, MessageType)                            \
    Type::Builder::Builder(ProtocolMessage* m)                                 \
    :m_(m) {                                                                   \
        m_->magic = kProtocolMessageMagic;                                     \
        m_->type = MessageType;                                                \
        m_->len = 0;                                                           \
    }                                           

#define DefineFieldSetter(Type, Field, FieldName, ArgType)                     \
    Type::Builder& Type::Builder::Set##FieldName(ArgType Field) {              \
        m_->as<Type*>()->Field = Field;                                        \
        return EnsureLength(this, m_, MinSizeWithField(Type, Field));          \
    }

#define DefineStringFieldSetter(Type, Field, FieldName)                        \
    Type::Builder& Type::Builder::Set##FieldName(alpha::Slice Field) {         \
    assert (sizeof(Type::Field) > Field.size());                               \
    ::strncpy(m_->as<Type*>()->Field, Field.data(), Field.size());             \
    return EnsureLength(this, m_, MinSizeWithField(Type, Field));              \
    }

#define DefineLastFieldSetter(Type, Field, FieldName)                          \
    Type::Builder& Type::Builder::Set##FieldName(alpha::Slice Field) {         \
        static_assert(MinSizeWithField(Type, Field)                            \
                == sizeof(Type), #Field" must be the last element of "#Type);  \
        assert (sizeof(Type::Field) > Field.size());                           \
        ::strncpy(m_->as<Type*>()->Field, Field.data(), Field.size());         \
        m_->len = offsetof(Type, Field) + Field.size();                        \
        return *this;                                                          \
    }

    DefineBuilderConstructor(ResolveRequest, MessageType::kResolveRequest);
    DefineFieldSetter(ResolveRequest, type, Type, ResolveRequestType)
    DefineFieldSetter(ResolveRequest, self_addr, SelfAddress, Address::NodeAddressType)
    DefineFieldSetter(ResolveRequest, cpmd_port, Port, int32_t)
    DefineFieldSetter(ResolveRequest, peer_addr, PeerAddress, Address::NodeAddressType)

    DefineBuilderConstructor(ResolveResponse, MessageType::kResolveResponse);
    DefineFieldSetter(ResolveResponse, code, Code, cpm::ResolveServerError)
    DefineFieldSetter(ResolveResponse, type, Type, ResolveResponseType)
    DefineFieldSetter(ResolveResponse, node_addr, NodeAddress, Address::NodeAddressType)
    DefineFieldSetter(ResolveResponse, node_port, NodePort, int32_t)
    DefineStringFieldSetter(ResolveResponse, node_ip, NodeIp)
    DefineLastFieldSetter(ResolveResponse, node_path, NodePath)

#undef MinSizeWithField
#undef DefineBuilderConstructor
#undef DefineFieldSetter
#undef DefineStringFieldSetter
#undef DefineLastFieldSetter
}
