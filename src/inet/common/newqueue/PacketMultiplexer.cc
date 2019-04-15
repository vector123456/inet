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
#include "inet/common/newqueue/PacketMultiplexer.h"

namespace inet {
namespace queue {

Define_Module(PacketMultiplexer);

void PacketMultiplexer::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        for (int i = 0; i < gateSize("in"); i++) {
            auto inputGate = gate("in", i);
            auto input = check_and_cast<IPacketProducer *>(getConnectedModule(inputGate));
            inputGates.push_back(inputGate);
            producers.push_back(input);
        }
        outputGate = gate("out");
        consumer = dynamic_cast<IPacketConsumer *>(getConnectedModule(outputGate));
    }
    else if (stage == INITSTAGE_LAST) {
        for (auto inputGate : inputGates)
            checkPushPacketSupport(inputGate);
        if (consumer != nullptr)
            checkPushPacketSupport(outputGate);
    }
}

void PacketMultiplexer::pushPacket(Packet *packet, cGate *gate)
{
    EV_INFO << "Forwarding pushed packet " << packet->getName() << "." << endl;
    pushOrSendPacket(packet, outputGate, consumer);
}

void PacketMultiplexer::handleCanPushPacket(cGate *gate)
{
    for (int i = 0; i < (int)inputGates.size(); i++)
        // NOTE: notifying a listener may prevent others from pushing
        if (consumer->canPushSomePacket(outputGate))
            producers[i]->handleCanPushPacket(inputGates[i]);
}

} // namespace queue
} // namespace inet

