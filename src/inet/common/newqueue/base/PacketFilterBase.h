//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#ifndef __INET_PACKETFILTERBASE_H
#define __INET_PACKETFILTERBASE_H

#include "inet/common/newqueue/base/PacketQueueingElementBase.h"
#include "inet/common/newqueue/contract/IPacketFilter.h"
#include "inet/common/newqueue/contract/IPacketQueueingElement.h"

namespace inet {
namespace queue {

class INET_API PacketFilterBase : public PacketQueueingElementBase, public IPacketFilter, public IPacketQueueingElement
{
  protected:
    cGate *inputGate = nullptr;
    IPacketProducer *producer = nullptr;
    IPacketProvider *provider = nullptr;

    cGate *outputGate = nullptr;
    IPacketConsumer *consumer = nullptr;
    IPacketCollector *collector = nullptr;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual bool matchesPacket(Packet *packet) = 0;

  public:
    virtual IPacketConsumer *getConsumer(cGate *gate) override { return this; }
    virtual IPacketProvider *getProvider(cGate *gate) override { return this; }

    virtual bool supportsPushPacket(cGate *gate) override { return true; }
    virtual int getNumPushablePackets(cGate *gate) override { return -1; }
    virtual bool canPushSomePacket(cGate *gate) override { return true; }
    virtual bool canPushPacket(Packet *packet, cGate *gate) override { return true; }
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPopPacket(cGate *gate) override { return true; }
    virtual int getNumPoppablePackets(cGate *gate) override { return -1; }
    virtual bool canPopSomePacket(cGate *gate) override;
    virtual Packet *canPopPacket(cGate *gate) override { throw cRuntimeError("Invalid operation"); }
    virtual Packet *popPacket(cGate *gate) override;

    virtual void handleCanPushPacket(cGate *gate) override;
    virtual void handleCanPopPacket(cGate *gate) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETFILTERBASE_H

