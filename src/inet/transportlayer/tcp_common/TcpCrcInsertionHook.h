//
// Copyright 2017 OpenSim Ltd.
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 3 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//

#ifndef __INET_TCPCRCINSERTIONHOOK_H
#define __INET_TCPCRCINSERTIONHOOK_H

#include "inet/common/INETDefs.h"

#include "inet/common/Protocol.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/transportlayer/common/CRC_m.h"
#include "inet/transportlayer/tcp_common/TcpCrcInsertionHook.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"


namespace inet {

namespace tcp {

class TcpCrcInsertion : public NetfilterBase::HookBase {
    CrcMode crcMode = (CrcMode)-1;
  public:
    TcpCrcInsertion() {}
    void setCrcMode(CrcMode crcModeP) { crcMode = crcModeP; }
    void insertCrc(const Protocol *networkProtocol, const L3Address& srcAddress, const L3Address& destAddress, const Ptr<TcpHeader>& tcpHeader, Packet *packet);
    uint16_t computeCrc(const Protocol *networkProtocol, const L3Address& srcAddress, const L3Address& destAddress, const std::vector<uint8_t>& tcpHeaderBytes, const std::vector<uint8_t>& tcpDataBytes);

  public:
    virtual Result datagramPreRoutingHook(Packet *packet) override { return ACCEPT; }
    virtual Result datagramForwardHook(Packet *packet) override { return ACCEPT; }
    virtual Result datagramPostRoutingHook(Packet *packet) override;
    virtual Result datagramLocalInHook(Packet *packet) override { return ACCEPT; }
    virtual Result datagramLocalOutHook(Packet *packet) override { return ACCEPT; }
};

} // namespace tcp

} // namespace inet

#endif // ifndef __INET_TCPCRCINSERTIONHOOK_H
