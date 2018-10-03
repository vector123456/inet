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

#include "inet/common/newqueue/CompoundPacketQueue.h"
#include "inet/common/Simsignals.h"
#include "inet/common/StringFormat.h"

namespace inet {
namespace queue {

Define_Module(CompoundPacketQueue);

void CompoundPacketQueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        displayStringTextFormat = par("displayStringTextFormat");
        frameCapacity = par("frameCapacity");
        dataCapacity = b(par("dataCapacity"));
        inputGate = gate("in");
        outputGate = gate("out");
        consumer = check_and_cast<IPacketConsumer *>(gate("in")->getPathEndGate()->getOwnerModule());
        provider = check_and_cast<IPacketProvider *>(gate("out")->getPathStartGate()->getOwnerModule());
        collection = check_and_cast<IPacketCollection *>(provider);
    }
    else if (stage == INITSTAGE_LAST) {
        checkPushPacketSupport(inputGate);
        checkPopPacketSupport(outputGate);
    }
}

int CompoundPacketQueue::getNumPackets()
{
    return collection->getNumPackets();
}

Packet *CompoundPacketQueue::getPacket(int index)
{
    return collection->getPacket(index);
}

void CompoundPacketQueue::pushPacket(Packet *packet, cGate *gate)
{
    emit(packetPushedSignal, packet);
    if ((frameCapacity != -1 && getNumPackets() >= frameCapacity) ||
        (dataCapacity != b(-1) && getTotalLength() + packet->getTotalLength() > dataCapacity))
    {
        EV_INFO << "Dropping packet " << packet->getName() << " because the queue is full." << endl;
        PacketDropDetails details;
        details.setReason(QUEUE_OVERFLOW);
        details.setLimit(frameCapacity);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
    else {
        animateSend(packet, inputGate);
        consumer->pushPacket(packet, inputGate->getPathEndGate());
        updateDisplayString();
    }
}

Packet *CompoundPacketQueue::popPacket(cGate *gate)
{
    auto packet = provider->popPacket(outputGate->getPathStartGate());
    animateSend(packet, outputGate->getPathStartGate());
    updateDisplayString();
    emit(packetPoppedSignal, packet);
    return packet;
}

void CompoundPacketQueue::removePacket(Packet *packet)
{
    collection->removePacket(packet);
    updateDisplayString();
    emit(packetRemovedSignal, packet);
}

b CompoundPacketQueue::getTotalLength()
{
    b length = b(0);
    int numPackets = getNumPackets();
    for (int i = 0; i < numPackets; i++)
        length += getPacket(i)->getTotalLength();
    return length;
}

void CompoundPacketQueue::updateDisplayString()
{
    auto text = StringFormat::formatString(displayStringTextFormat, [&] (char directive) {
        static std::string result;
        switch (directive) {
            case 'p':
                result = std::to_string(collection->getNumPackets());
                break;
            case 'l':
                result = getTotalLength().str();
                break;
            case 'n':
                result = !collection->isEmpty() ? collection->getPacket(0)->getFullName() : "";
                break;
            default:
                throw cRuntimeError("Unknown directive: %c", directive);
        }
        return result.c_str();
    });
    getDisplayString().setTagArg("t", 0, text);
}

} // namespace queue
} // namespace inet

