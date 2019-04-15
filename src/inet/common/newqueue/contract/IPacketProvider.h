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

#ifndef __INET_IPACKETPROVIDER_H
#define __INET_IPACKETPROVIDER_H

#include "inet/common/packet/Packet.h"

namespace inet {
namespace queue {

/**
 * This class defines the interface for packet providers.
 */
class INET_API IPacketProvider
{
  public:
    virtual ~IPacketProvider() {}

    /**
     * Returns the number of poppable packets at the given gate. The value -1
     * means the number is undefined. The gate must support popping packets.
     */
    virtual int getNumPoppablePackets(cGate *gate = nullptr) = 0;

    /**
     * Returns false if the provider is empty at the given gate and no more
     * packets can be popped without raising an error. The gate must support
     * popping packets.
     */
    virtual bool canPopSomePacket(cGate *gate = nullptr) = 0;

    /**
     * Returns the packet that can be popped at the given gate. The returned
     * value may be nullptr if there is no such packet. The gate must support
     * popping packets.
     */
    virtual Packet *canPopPacket(cGate *gate = nullptr) = 0;

    /**
     * Pops a packet from the provider at the given gate. The provider must not
     * be empty at the given gate. The returned packet is never nullptr, and the
     * gate must support popping packets.
     */
    virtual Packet *popPacket(cGate *gate = nullptr) = 0;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_IPACKETPROVIDER_H

