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

#include "inet/common/newqueue/PriorityScheduler.h"

namespace inet {
namespace queue {

Define_Module(PriorityScheduler);

void PriorityScheduler::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        for (auto provider : providers)
            collections.push_back(dynamic_cast<IPacketCollection *>(provider));
}

int PriorityScheduler::getNumPackets()
{
    int size = 0;
    for (auto collection : collections)
        size += collection->getNumPackets();
    return size;
}

Packet *PriorityScheduler::getPacket(int index)
{
    for (auto collection : collections) {
        auto numPackets = collection->getNumPackets();
        if (index < numPackets)
            return collection->getPacket(index);
        else
            index -= numPackets;
    }
    throw cRuntimeError("Index out of range");
}

void PriorityScheduler::removePacket(Packet *packet)
{
    for (auto collection : collections) {
        for (int j = 0; j < collection->getNumPackets(); j++) {
            if (collection->getPacket(j) == packet) {
                collection->removePacket(packet);
                return;
            }
        }
    }
    throw cRuntimeError("Cannot find packet");
}

int PriorityScheduler::schedulePacket()
{
    for (int i = 0; i < (int)providers.size(); i++)
        if (providers[i]->canPopSomePacket(inputGates[i]->getPathStartGate()))
            return i;
    return -1;
}

} // namespace queue
} // namespace inet

