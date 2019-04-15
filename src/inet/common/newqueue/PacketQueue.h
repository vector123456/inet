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

#ifndef __INET_PACKETQUEUE_H
#define __INET_PACKETQUEUE_H

#include "inet/common/newqueue/base/PacketQueueBase.h"
#include "inet/common/newqueue/contract/IPacketBuffer.h"
#include "inet/common/newqueue/contract/IPacketCollector.h"
#include "inet/common/newqueue/contract/IPacketComparatorFunction.h"
#include "inet/common/newqueue/contract/IPacketDropperFunction.h"
#include "inet/common/newqueue/contract/IPacketProducer.h"

namespace inet {
namespace queue {

class INET_API PacketQueue : public PacketQueueBase, public IPacketBuffer::ICallback
{
  protected:
    cGate *inputGate = nullptr;
    IPacketProducer *producer = nullptr;

    cGate *outputGate = nullptr;
    IPacketCollector *collector = nullptr;

    int frameCapacity = -1;
    b dataCapacity = b(-1);
    const char *displayStringTextFormat = nullptr;

    cPacketQueue queue;
    IPacketBuffer *buffer = nullptr;

    IPacketDropperFunction *packetDropperFunction = nullptr;
    IPacketComparatorFunction *packetComparatorFunction = nullptr;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;
    virtual void updateDisplayString();
    virtual bool isOverloaded();

  public:
    virtual ~PacketQueue() { delete packetDropperFunction; }

    virtual int getMaxNumPackets() override { return frameCapacity; }
    virtual int getNumPackets() override;

    virtual b getMaxTotalLength() override { return dataCapacity; }
    virtual b getTotalLength() override { return b(queue.getBitLength()); }

    virtual bool isEmpty() override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) override;
    virtual void removePacket(Packet *packet) override;

    virtual bool supportsPushPacket(cGate *gate) override { return inputGate == gate; }
    virtual int getNumPushablePackets(cGate *gate) override { return -1; }
    virtual bool canPushSomePacket(cGate *gate) override { return packetDropperFunction ? true : getMaxNumPackets() == -1 || getNumPackets() < getMaxNumPackets(); }
    virtual bool canPushPacket(Packet *packet, cGate *gate) override { return canPushSomePacket(gate); }
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPopPacket(cGate *gate) override { return outputGate == gate; }
    virtual int getNumPoppablePackets(cGate *gate) override { return -1; }
    virtual bool canPopSomePacket(cGate *gate) override { return !isEmpty(); }
    virtual Packet *canPopPacket(cGate *gate) override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *popPacket(cGate *gate) override;

    virtual void handlePacketRemoved(Packet *packet) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETQUEUE_H

