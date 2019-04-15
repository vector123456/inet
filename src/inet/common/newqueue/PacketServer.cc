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

#include "inet/common/newqueue/PacketServer.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(PacketServer);

void PacketServer::initialize(int stage)
{
    PacketServerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        processingTimer = new cMessage("ProcessingTimer");
}

void PacketServer::handleMessage(cMessage *message)
{
    if (message == processingTimer) {
        endProcessingPacket();
        if (canStartProcessingPacket()) {
            startProcessingPacket();
            scheduleProcessingTimer();
        }
    }
    else
        throw cRuntimeError("Unknown message");
}

void PacketServer::scheduleProcessingTimer()
{
    simtime_t processingTime = par("processingTime");
    auto processingBitrate = bps(par("processingBitrate"));
    processingTime += s(packet->getTotalLength() / processingBitrate).get();
    scheduleAt(simTime() + processingTime, processingTimer);
}

bool PacketServer::canStartProcessingPacket()
{
    return provider->canPopSomePacket(inputGate->getPathStartGate());
}

void PacketServer::startProcessingPacket()
{
    packet = provider->popPacket(inputGate->getPathStartGate());
    EV_INFO << "Processing packet " << packet->getName() << " started." << endl;
}

void PacketServer::endProcessingPacket()
{
    EV_INFO << "Processing packet " << packet->getName() << " ended.\n";
    animateSend(packet, outputGate);
    consumer->pushPacket(packet, outputGate->getPathEndGate());
    packet = nullptr;
}

void PacketServer::handleCanPushPacket(cGate *gate)
{
}

void PacketServer::handleCanPopPacket(cGate *gate)
{
    if (!processingTimer->isScheduled() && canStartProcessingPacket()) {
        startProcessingPacket();
        scheduleProcessingTimer();
    }
}

} // namespace queue
} // namespace inet

