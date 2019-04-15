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

#include "inet/common/newqueue/MarkovClassifier.h"

namespace inet {
namespace queue {

Define_Module(MarkovClassifier);

MarkovClassifier::~MarkovClassifier()
{
    cancelAndDelete(transitionTimer);
    cancelAndDelete(waitTimer);
}

// TODO: extract common code with MarkovScheduler
void MarkovClassifier::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        inputGate = gate("in");
        for (int i = 0; i < gateSize("out"); i++) {
            auto outputGate = gate("out", i);
            auto output = check_and_cast<IPacketCollector *>(outputGate->getPathEndGate()->getOwnerModule());
            outputGates.push_back(outputGate);
            collectors.push_back(output);
        }
        provider = check_and_cast<IPacketProvider *>(inputGate->getPathStartGate()->getOwnerModule());
        state = par("initialState");
        int numStates = gateSize("out");
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
        for (auto outputGate : outputGates)
            checkPopPacketSupport(outputGate);
        checkPopPacketSupport(inputGate);
    }
}

void MarkovClassifier::handleMessage(cMessage *message)
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
        collectors[state]->handleCanPopPacket(outputGates[state]);
        scheduleWaitTimer();
    }
    else
        throw cRuntimeError("Unknown message");
}

int MarkovClassifier::classifyPacket(Packet *packet)
{
    return state;
}

void MarkovClassifier::scheduleWaitTimer()
{
    scheduleAt(simTime() + waitIntervals[state], waitTimer);
}

bool MarkovClassifier::canPopSomePacket(cGate *gate)
{
    return gate->getIndex() == state;
}

Packet *MarkovClassifier::canPopPacket(cGate *gate)
{
    return canPopSomePacket(gate) ? provider->canPopPacket(inputGate->getPathStartGate()) : nullptr;
}

Packet *MarkovClassifier::popPacket(cGate *gate)
{
    if (gate->getIndex() != state)
        throw cRuntimeError("Cannot pop from gate");
    auto packet = provider->popPacket(inputGate->getPathEndGate());
    animateSend(packet, gate);
    return packet;
}

void MarkovClassifier::handleCanPopPacket(cGate *gate)
{
    for (auto output : collectors)
        output->handleCanPopPacket(outputGates[state]);
}

} // namespace queue
} // namespace inet

