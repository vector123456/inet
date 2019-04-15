//
// Copyright (C) 2012 Opensim Ltd.
// Author: Tamas Borbely
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_WRRSCHEDULER_H
#define __INET_WRRSCHEDULER_H

#include "inet/common/newqueue/base/PacketSchedulerBase.h"
#include "inet/common/newqueue/contract/IPacketCollection.h"

namespace inet {
namespace queue {

/**
 * This module implements a Weighted Round Robin Scheduler.
 */
class INET_API WrrScheduler : public PacketSchedulerBase, public IPacketCollection
{
  protected:
    int *weights = nullptr; // array of weights (has numInputs elements)
    int *buckets = nullptr; // array of tokens in buckets (has numInputs elements)

    std::vector<IPacketCollection *> collections;

  protected:
    virtual void initialize(int stage) override;
    virtual int schedulePacket() override;

  public:
    virtual ~WrrScheduler();

    virtual int getMaxNumPackets() override { return -1; }
    virtual int getNumPackets() override;

    virtual b getMaxTotalLength() override { return b(-1); }
    virtual b getTotalLength() override { return b(-1); }

    virtual bool isEmpty() override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) override { throw cRuntimeError("Invalid operation"); }
    virtual void removePacket(Packet *packet) override { throw cRuntimeError("Invalid operation"); }
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_WRRSCHEDULER_H

