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

#ifndef __INET_PACKETPRODUCER_H
#define __INET_PACKETPRODUCER_H

#include "inet/common/newqueue/base/PacketCreatorBase.h"
#include "inet/common/newqueue/contract/IPacketProducer.h"
#include "inet/common/newqueue/contract/IPacketQueueingElement.h"

namespace inet {
namespace queue {

class INET_API PacketProducer : public PacketCreatorBase, public IPacketProducer, public IPacketQueueingElement
{
  protected:
    cGate *outputGate = nullptr;
    IPacketConsumer *consumer = nullptr;

    cPar *productionIntervalParameter = nullptr;
    cMessage *productionTimer = nullptr;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual void scheduleProductionTimer();
    virtual void producePacket();

  public:
    virtual ~PacketProducer() { cancelAndDelete(productionTimer); }

    virtual IPacketConsumer *getConsumer(cGate *gate) override { return consumer; }

    virtual bool supportsPushPacket(cGate *gate) override { return outputGate == gate; }
    virtual bool supportsPopPacket(cGate *gate) override { return false; }

    virtual void handleCanPushPacket(cGate *gate) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETPRODUCER_H

