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

#include "inet/common/newqueue/MarkovScheduler.h"

namespace inet {
namespace queue {

Define_Module(MarkovScheduler);

MarkovScheduler::~MarkovScheduler()
{
    cancelAndDelete(transitionTimer);
    cancelAndDelete(waitTimer);
}

void MarkovScheduler::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        outputGate = gate("out");
        for (int i = 0; i < gateSize("in"); i++) {
            auto inputGate = gate("in", i);
            auto input = check_and_cast<IPacketProducer *>(inputGate->getPathStartGate()->getOwnerModule());
            inputGates.push_back(inputGate);
            producers.push_back(input);
        }
        consumer = check_and_cast<IPacketConsumer *>(outputGate->getPathEndGate()->getOwnerModule());
        state = par("initialState");
        int numStates = gateSize("in");
        cStringTokenizer transitionProbabilitiesTokenizer(par("transitionProbabilities"));
        for (int i = 0; i < numStates; i++) {
            transitionProbabilities.push_back({});
            for (int j = 0; j < numStates; j++)
                transitionProbabilities[i].push_back(atof(transitionProbabilitiesTokenizer.nextToken()));
        }
        cStringTokenizer waitIntervalsTokenizer(par("waitIntervals"));
        for (int i = 0; i < numStates; i++)
            waitIntervals.push_back(SimTime::parse(waitIntervalsTokenizer.nextToken()));
        waitTimer = new cMessage("WaitTimer");
        scheduleWaitTimer();
        WATCH(state);
    }
    else if (stage == INITSTAGE_LAST) {
        for (auto inputGate : inputGates)
            checkPushPacketSupport(inputGate);
        checkPushPacketSupport(outputGate);
    }
}

void MarkovScheduler::handleMessage(cMessage *message)
{
    if (message == waitTimer) {
        double v = uniform(0, 1);
        double sum = 0;
        int numStates = (int)transitionProbabilities.size();
        for (int i = 0; i < numStates; i++) {
            sum += transitionProbabilities[state][i];
            if (sum >= v || i == numStates - 1) {
                state = i;
                break;
            }
        }
        producers[state]->handleCanPushPacket(inputGates[state]);
        scheduleWaitTimer();
    }
    else
        throw cRuntimeError("Unknown message");
}

int MarkovScheduler::schedulePacket()
{
    return state;
}

void MarkovScheduler::scheduleWaitTimer()
{
    scheduleAt(simTime() + waitIntervals[state], waitTimer);
}

bool MarkovScheduler::canPushSomePacket(cGate *gate)
{
    return gate->getIndex() == state;
}

bool MarkovScheduler::canPushPacket(Packet *packet, cGate *gate)
{
    return canPushSomePacket(gate);
}

void MarkovScheduler::pushPacket(Packet *packet, cGate *gate)
{
    if (gate->getIndex() != state)
        throw cRuntimeError("Cannot push to gate");
    animateSend(packet, outputGate);
    consumer->pushPacket(packet, outputGate->getPathEndGate());
}

void MarkovScheduler::handleCanPushPacket(cGate *gate)
{
    for (auto input : producers)
        input->handleCanPushPacket(inputGates[state]);
}

} // namespace queue
} // namespace inet

