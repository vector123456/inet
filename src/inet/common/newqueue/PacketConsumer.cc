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
#include "inet/common/newqueue/PacketConsumer.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(PacketConsumer);

void PacketConsumer::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        inputGate = gate("in");
        producer = check_and_cast<IPacketProducer *>(getConnectedModule(inputGate));
        consumptionIntervalParameter = &par("consumptionInterval");
        consumptionTimer = new cMessage("ConsumptionTimer");
        WATCH(numPacket);
        WATCH(totalLength);
    }
    else if (stage == INITSTAGE_LAST) {
        checkPushPacketSupport(inputGate);
        scheduleConsumptionTimer();
    }
}

void PacketConsumer::handleMessage(cMessage *message)
{
    ASSERT(message == consumptionTimer);
    producer->handleCanPushPacket(inputGate);
}

void PacketConsumer::scheduleConsumptionTimer()
{
    simtime_t interval = consumptionIntervalParameter->doubleValue();
    if (interval != 0 || consumptionTimer->getArrivalModule() == nullptr)
        scheduleAt(simTime() + interval, consumptionTimer);
}

void PacketConsumer::pushPacket(Packet *packet, cGate *gate)
{
    if (consumptionTimer->isScheduled() && consumptionTimer->getArrivalTime() > simTime())
        throw cRuntimeError("Another packet is already being consumed");
    else {
        emit(packetPushedSignal, packet);
        consumePacket(packet);
        scheduleConsumptionTimer();
    }
}

void PacketConsumer::consumePacket(Packet *packet)
{
    EV_INFO << "Consuming packet " << packet->getName() << "." << endl;
    numPacket++;
    totalLength += packet->getTotalLength();
    PacketDropDetails details;
    details.setReason(OTHER_PACKET_DROP);
    emit(packetDroppedSignal, packet, &details);
    delete packet;
}

} // namespace queue
} // namespace inet

