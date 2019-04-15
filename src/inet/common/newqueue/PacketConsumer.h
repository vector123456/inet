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

#ifndef __INET_PACKETCONSUMER_H
#define __INET_PACKETCONSUMER_H

#include "inet/common/newqueue/base/PacketConsumerBase.h"
#include "inet/common/newqueue/contract/IPacketProducer.h"
#include "inet/common/newqueue/contract/IPacketQueueingElement.h"

namespace inet {
namespace queue {

class INET_API PacketConsumer : public PacketConsumerBase, public IPacketQueueingElement
{
  protected:
    cGate *inputGate = nullptr;
    IPacketProducer *producer = nullptr;

    cPar *consumptionIntervalParameter = nullptr;
    cMessage *consumptionTimer = nullptr;

    int numPacket = 0;
    b totalLength = b(0);

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual void scheduleConsumptionTimer();
    virtual void consumePacket(Packet *packet);

  public:
    virtual ~PacketConsumer() { cancelAndDelete(consumptionTimer); }

    virtual bool supportsPushPacket(cGate *gate) override { return gate == inputGate; }
    virtual bool supportsPopPacket(cGate *gate) override { return false; }

    virtual bool canPushSomePacket(cGate *gate) override { return !consumptionTimer->isScheduled(); }
    virtual bool canPushPacket(Packet *packet, cGate *gate) override { return canPushSomePacket(gate); }
    virtual void pushPacket(Packet *packet, cGate *gate) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETCONSUMER_H

