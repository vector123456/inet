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

#include "inet/common/ModuleAccess.h"
#include "inet/common/newqueue/PacketProvider.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(PacketProvider);

void PacketProvider::initialize(int stage)
{
    PacketCreatorBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        outputGate = gate("out");
        collector = check_and_cast<IPacketCollector *>(getConnectedModule(outputGate));
        providingIntervalParameter = &par("providingInterval");
        providingTimer = new cMessage("ProvidingTimer");
    }
    else if (stage == INITSTAGE_LAST) {
        checkPopPacketSupport(outputGate);
        scheduleProvidingTimer();
    }
}

void PacketProvider::handleMessage(cMessage *message)
{
    ASSERT(message == providingTimer);
    collector->handleCanPopPacket(outputGate);
}

void PacketProvider::scheduleProvidingTimer()
{
    simtime_t interval = providingIntervalParameter->doubleValue();
    if (interval != 0 || providingTimer->getArrivalModule() == nullptr)
        scheduleAt(simTime() + interval, providingTimer);
}

Packet *PacketProvider::canPopPacket(cGate *gate)
{
    if (providingTimer->isScheduled())
        return nullptr;
    else {
        if (nextPacket == nullptr)
            nextPacket = createPacket();
        return nextPacket;
    }
}

Packet *PacketProvider::popPacket(cGate *gate)
{
    if (providingTimer->isScheduled()  && providingTimer->getArrivalTime() > simTime())
        throw cRuntimeError("Another packet is already being provided");
    else {
        auto packet = providePacket(gate);
        animateSend(packet, outputGate);
        emit(packetPoppedSignal, packet);
        scheduleProvidingTimer();
        return packet;
    }
}

Packet *PacketProvider::providePacket(cGate *gate)
{
    Packet *packet;
    if (nextPacket == nullptr)
        packet = createPacket();
    else {
        packet = nextPacket;
        nextPacket = nullptr;
    }
    EV_INFO << "Providing packet " << packet->getName() << "." << endl;
    numPacket++;
    totalLength += packet->getTotalLength();
    return packet;
}

} // namespace queue
} // namespace inet

