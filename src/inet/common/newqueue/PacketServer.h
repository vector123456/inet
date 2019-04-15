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

#ifndef __INET_PACKETSERVER_H
#define __INET_PACKETSERVER_H

#include "inet/common/newqueue/base/PacketServerBase.h"

namespace inet {
namespace queue {

class INET_API PacketServer : public PacketServerBase
{
  protected:
    cMessage *processingTimer = nullptr;
    Packet *packet = nullptr;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message);
    virtual void scheduleProcessingTimer();
    virtual bool canStartProcessingPacket();
    virtual void startProcessingPacket();
    virtual void endProcessingPacket();

  public:
    virtual ~PacketServer() { cancelAndDelete(processingTimer); }

    virtual IPacketProvider *getProvider(cGate *gate) override { return provider; }
    virtual IPacketConsumer *getConsumer(cGate *gate) override { return consumer; }

    virtual void handleCanPushPacket(cGate *gate) override;
    virtual void handleCanPopPacket(cGate *gate) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETSERVER_H

