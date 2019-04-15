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

#ifndef __INET_MARKOVSCHEDULER_H
#define __INET_MARKOVSCHEDULER_H

#include "inet/common/newqueue/base/PacketConsumerBase.h"
#include "inet/common/newqueue/base/PacketSchedulerBase.h"
#include "inet/common/newqueue/contract/IPacketProducer.h"

namespace inet {
namespace queue {

class INET_API MarkovScheduler : public PacketConsumerBase, public IPacketQueueingElement, public IPacketProducer
{
  protected:
    std::vector<cGate *> inputGates;
    std::vector<IPacketProducer *> producers;

    cGate *outputGate = nullptr;
    IPacketConsumer *consumer = nullptr;

    std::vector<std::vector<double>> transitionProbabilities;
    std::vector<simtime_t> waitIntervals;

    int state;

    cMessage *transitionTimer = nullptr;
    cMessage *waitTimer = nullptr;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual int schedulePacket();
    virtual void scheduleWaitTimer();

  public:
    virtual ~MarkovScheduler();

    virtual IPacketConsumer *getConsumer(cGate *gate) override { return consumer; }

    virtual bool supportsPushPacket(cGate* gate) override { return true; }
    virtual bool supportsPopPacket(cGate *gate) override { return false; }

    virtual bool canPushSomePacket(cGate *gate) override;
    virtual bool canPushPacket(Packet *packet, cGate *gate) override;
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual void handleCanPushPacket(cGate *gate) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_MARKOVSCHEDULERBASE_H

