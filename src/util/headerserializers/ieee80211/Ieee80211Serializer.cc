//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include <Ieee80211Serializer.h>
#include <Ieee80211Frame_m.h>
#include <Ieee80211MgmtFrames_m.h>

namespace INETFw // load headers into a namespace, to avoid conflicts with platform definitions of the same stuff
{
#include "headers/bsdint.h"
#include "headers/in.h"
#include "headers/in_systm.h"
#include "headers/ethernet.h"
#include "headers/ieee80211.h"
};

#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN64)
//#include <netinet/in.h>  // htonl, ntohl, ...
#endif

#ifdef WITH_IPv4
#include "IPv4Serializer.h"
#endif

#ifdef WITH_IPv6
#include "IPv6Serializer.h"
#endif

#include "ARPSerializer.h"

#include "EthernetCRC.h"

using namespace INETFw;


int Ieee80211Serializer::serialize(Ieee80211Frame *pkt, unsigned char *buf, unsigned int bufsize)
{
    unsigned int packetLength = 0;

    if (NULL != dynamic_cast<Ieee80211ACKFrame *>(pkt))
    {
        Ieee80211ACKFrame *ackFrame = dynamic_cast<Ieee80211ACKFrame *>(pkt);
        struct ieee80211_frame_ack *frame = (struct ieee80211_frame_ack *) (buf);
        frame->i_fc[0] = 0xD4;
        frame->i_fc[1] = 0;
        frame->i_dur = (int)(ackFrame->getDuration().dbl()*1000);
        ackFrame->getReceiverAddress().getAddressBytes(frame->i_ra);

        packetLength = 4 + IEEE80211_ADDR_LEN;
    }
    else if (NULL != dynamic_cast<Ieee80211RTSFrame *>(pkt))
    {
        Ieee80211RTSFrame *rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(pkt);
        struct ieee80211_frame_rts *frame = (struct ieee80211_frame_rts *) (buf);
        frame->i_fc[0] = 0xB4;
        frame->i_fc[1] = 0;
        frame->i_dur = (int)(rtsFrame->getDuration().dbl()*1000);
        rtsFrame->getReceiverAddress().getAddressBytes(frame->i_ra);
        rtsFrame->getTransmitterAddress().getAddressBytes(frame->i_ta);
        packetLength = 4+ 2*IEEE80211_ADDR_LEN;
    }

    else if (NULL != dynamic_cast<Ieee80211CTSFrame *>(pkt))
    {
        Ieee80211CTSFrame *ctsFrame = dynamic_cast<Ieee80211CTSFrame *>(pkt);
        struct ieee80211_frame_cts *frame = (struct ieee80211_frame_cts *) (buf);
        frame->i_fc[0] = 0xC4;
        frame->i_fc[1] = 0;
        frame->i_dur = (int)(ctsFrame->getDuration().dbl()*1000);
        ctsFrame->getReceiverAddress().getAddressBytes(frame->i_ra);
        packetLength = 4 + IEEE80211_ADDR_LEN;
    }
    else if (NULL != dynamic_cast<Ieee80211DataOrMgmtFrame *>(pkt))
    {
        Ieee80211DataOrMgmtFrame *dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(pkt);

        struct ieee80211_frame *frame = (struct ieee80211_frame *) (buf);
        frame->i_fc[0] = 0x8;
        frame->i_fc[1] = 0; // TODO: Order, Protected Frame, MoreData, Power Mgt
        frame->i_fc[1] |= dataOrMgmtFrame->getRetry();
        frame->i_fc[1] <<= 1;
        frame->i_fc[1] |= dataOrMgmtFrame->getMoreFragments();
        frame->i_fc[1] <<= 1;
        frame->i_fc[1] |= dataOrMgmtFrame->getFromDS();
        frame->i_fc[1] <<= 1;
        frame->i_fc[1] |= dataOrMgmtFrame->getToDS();
        frame->i_dur = (int)(dataOrMgmtFrame->getDuration().dbl()*1000);
        dataOrMgmtFrame->getReceiverAddress().getAddressBytes(frame->i_addr1);
        dataOrMgmtFrame->getTransmitterAddress().getAddressBytes(frame->i_addr2);
        dataOrMgmtFrame->getAddress3().getAddressBytes(frame->i_addr3);
        frame->i_seq = dataOrMgmtFrame->getFragmentNumber();
        frame->i_seq <<= 12;
        frame->i_seq |= (dataOrMgmtFrame->getSequenceNumber() & 0xFFF);

        packetLength = 6 + 3*IEEE80211_ADDR_LEN;

        if (NULL != dynamic_cast<Ieee80211DataFrameWithSNAP *>(pkt))
        {
            Ieee80211DataFrameWithSNAP *dataFrame = dynamic_cast<Ieee80211DataFrameWithSNAP *>(pkt);
            if (dataFrame->getFromDS() && dataFrame->getToDS())
            {
                packetLength += IEEE80211_ADDR_LEN;
                dataFrame->getAddress4().getAddressBytes((uint8_t *) (buf));
            }

            struct snap_header *snap_hdr = (struct snap_header *) (buf + packetLength);

            snap_hdr->dsap = 0xAA;
            snap_hdr->ssap = 0xAA;
            snap_hdr->ctrl = 0x03;
            snap_hdr->snap = htons(dataFrame->getEtherType());
            snap_hdr->snap <<= 24;

            packetLength += 8;
            cMessage *encapPacket = dataFrame->getEncapsulatedPacket();

            switch (dataFrame->getEtherType())
            {
#ifdef WITH_IPv4
                case ETHERTYPE_IP:
                    packetLength += IPv4Serializer().serialize(check_and_cast<IPv4Datagram *>(encapPacket),
                                                                       buf+packetLength, bufsize-packetLength, true);
                    break;
#endif

#ifdef WITH_IPv6
                case ETHERTYPE_IPV6:
                    packetLength += IPv6Serializer().serialize(check_and_cast<IPv6Datagram *>(encapPacket),
                                                                       buf+packetLength, bufsize-packetLength);
                    break;
#endif

#ifdef WITH_IPv4
                case ETHERTYPE_ARP:
                    packetLength += ARPSerializer().serialize(check_and_cast<ARPPacket *>(encapPacket),
                                                                       buf+packetLength, bufsize-packetLength);
                    break;
#endif

                default:
                    throw cRuntimeError("Ieee80211Serializer: cannot serialize protocol %x", dataFrame->getEtherType());
            }
        }

        else if (NULL != dynamic_cast<Ieee80211AuthenticationFrame *>(pkt))
        {
            //type = ST_AUTHENTICATION;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211DeauthenticationFrame *>(pkt))
        {
            //type = ST_DEAUTHENTICATION;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211DisassociationFrame *>(pkt))
        {
            //type = ST_DISASSOCIATION;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211ProbeRequestFrame *>(pkt))
        {
            //type = ST_PROBEREQUEST;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211AssociationRequestFrame *>(pkt))
        {
            //type = ST_ASSOCIATIONREQUEST;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211ReassociationRequestFrame *>(pkt))
        {
            //type = ST_REASSOCIATIONREQUEST;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211AssociationResponseFrame *>(pkt))
        {
            //type = ST_ASSOCIATIONRESPONSE;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211ReassociationResponseFrame *>(pkt))
        {
            //type = ST_REASSOCIATIONRESPONSE;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211BeaconFrame *>(pkt))
        {
            //type = ST_BEACON;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211ProbeResponseFrame *>(pkt))
        {
            //type = ST_PROBERESPONSE;
            return 0;
        }

        else if (NULL != dynamic_cast<Ieee80211ActionFrame *>(pkt))
        {
            //type = ST_ACTION;
            return 0;
        }
    }
    else
        throw cRuntimeError("Ieee80211Serializer: cannot serialize the frame");

    uint32_t *fcs = (uint32_t *) (buf + packetLength);
    *fcs = ethernetCRC(buf, packetLength);
    return packetLength + 4;
}

cPacket* Ieee80211Serializer::parse(const unsigned char *buf, unsigned int bufsize)
{
    uint32_t crc = ethernetCRC(buf, bufsize);
    EV_DEBUG << "CRC: "<< crc << " (" << (0x2144DF1C == crc ) << ")"<< endl;
    uint8_t *type = (uint8_t *) (buf);
    switch(*type)
    {
        case 0xD4: // ST_ACK
        {
            struct ieee80211_frame_ack *frame = (struct ieee80211_frame_ack *) (buf);
            cPacket *pkt = new Ieee80211ACKFrame;
            Ieee80211ACKFrame *ackFrame = (Ieee80211ACKFrame*)pkt;
            ackFrame->setType(ST_ACK);
            ackFrame->setToDS(false);
            ackFrame->setFromDS(false);
            ackFrame->setRetry(false);
            ackFrame->setMoreFragments(false);
            ackFrame->setDuration(SimTime((double)frame->i_dur/1000.0));
            MACAddress temp;
            temp.setAddressBytes(frame->i_ra);
            ackFrame->setReceiverAddress(temp);
            return pkt;
        }
        case 0xB4: // ST_RTS
        {
            struct ieee80211_frame_rts *frame = (struct ieee80211_frame_rts *) (buf);
            cPacket *pkt = new Ieee80211RTSFrame;
            Ieee80211RTSFrame *rtsFrame = (Ieee80211RTSFrame*)pkt;
            rtsFrame->setType(ST_RTS);
            rtsFrame->setToDS(false);
            rtsFrame->setFromDS(false);
            rtsFrame->setRetry(false);
            rtsFrame->setMoreFragments(false);
            rtsFrame->setDuration(SimTime((double)frame->i_dur/1000.0));
            MACAddress temp;
            temp.setAddressBytes(frame->i_ra);
            rtsFrame->setReceiverAddress(temp);
            temp.setAddressBytes(frame->i_ta);
            rtsFrame->setTransmitterAddress(temp);
            return pkt;
        }
        case 0xC4: // ST_CTS
        {
            struct ieee80211_frame_cts *frame = (struct ieee80211_frame_cts *) (buf);
            cPacket *pkt = new Ieee80211CTSFrame;
            Ieee80211CTSFrame *ctsFrame = (Ieee80211CTSFrame*)pkt;
            ctsFrame->setType(ST_CTS);
            ctsFrame->setToDS(false);
            ctsFrame->setFromDS(false);
            ctsFrame->setRetry(false);
            ctsFrame->setMoreFragments(false);
            ctsFrame->setDuration(SimTime((double)frame->i_dur/1000.0));
            MACAddress temp;
            temp.setAddressBytes(frame->i_ra);
            ctsFrame->setReceiverAddress(temp);
            return pkt;
        }
        case 0x8: // ST_DATA
        {
            struct ieee80211_frame_addr4 *frame = (struct ieee80211_frame_addr4 *) (buf);
            cPacket *pkt = new Ieee80211DataFrameWithSNAP;
            Ieee80211DataFrameWithSNAP *dataFrame = (Ieee80211DataFrameWithSNAP*)pkt;
            dataFrame->setType(ST_DATA);
            dataFrame->setToDS(frame->i_fc[1]&0x1);
            dataFrame->setFromDS(frame->i_fc[1]&0x2);
            dataFrame->setMoreFragments(frame->i_fc[1]&0x4);
            dataFrame->setRetry(frame->i_fc[1]&0x8);
            dataFrame->setDuration(SimTime((double)frame->i_dur/1000.0));
            MACAddress temp;
            temp.setAddressBytes(frame->i_addr1);
            dataFrame->setReceiverAddress(temp);
            temp.setAddressBytes(frame->i_addr2);
            dataFrame->setTransmitterAddress(temp);
            temp.setAddressBytes(frame->i_addr3);
            dataFrame->setAddress3(temp);
            if (dataFrame->getFromDS() && dataFrame->getToDS())
            {
                temp.setAddressBytes(frame->i_addr4);
                dataFrame->setAddress4(temp);
            }
            dataFrame->setSequenceNumber(frame->i_seq & 0xFFF);
            uint16_t temp16 = frame->i_seq;
            temp16 >>= 12;
            dataFrame->setFragmentNumber(frame->i_seq >> 12);

            unsigned int packetLength;
            if (dataFrame->getFromDS() && dataFrame->getToDS())
                packetLength = 6 + 4*IEEE80211_ADDR_LEN;
            else
                packetLength = 6 + 3*IEEE80211_ADDR_LEN;

            struct snap_header *snap_hdr = (struct snap_header *) (buf + packetLength);
            uint64_t temp64 = snap_hdr->snap;
            temp64 >>= 24;
            dataFrame->setEtherType(ntohs(snap_hdr->snap >> 24));

            packetLength += 8;

            cPacket *encapPacket = NULL;

            switch (dataFrame->getEtherType())
            {
#ifdef WITH_IPv4
                case ETHERTYPE_IP:
                    encapPacket = new IPv4Datagram("ipv4-from-wire");
                    IPv4Serializer().parse(buf+packetLength, bufsize-packetLength, (IPv4Datagram *)encapPacket);
                    break;
#endif

#ifdef WITH_IPv6
                case ETHERTYPE_IPV6:
                    encapPacket = new IPv6Datagram("ipv6-from-wire");
                    IPv6Serializer().parse(buf+packetLength, bufsize-packetLength, (IPv6Datagram *)encapPacket);
                    break;
#endif

                case ETHERTYPE_ARP:
                    encapPacket = new ARPPacket("arp-from-wire");
                    ARPSerializer().parse(buf+packetLength, bufsize-packetLength, (ARPPacket *)encapPacket);
                    break;

                default:
                    throw cRuntimeError("Ieee80211Serializer: cannot parse protocol %x", dataFrame->getEtherType());
            }

            ASSERT(encapPacket);
            dataFrame->encapsulate(encapPacket);
            dataFrame->setName(encapPacket->getName());
            return pkt;
        }

        case 0xB0: // ST_AUTHENTICATION
        {
            // Ieee80211AuthenticationFrame
        }

        case 0xC0: //ST_ST_DEAUTHENTICATION
        {
            // Ieee80211DeauthenticationFrame
        }

        case 0xA0: // ST_DISASSOCIATION
        {
            // Ieee80211DisassociationFrame
        }

        case 0x40: // ST_PROBEREQUEST
        {
            // Ieee80211ProbeRequestFrame
        }

        case 0x00: // ST_ASSOCIATIONREQUEST
        {
            // Ieee80211AssociationRequestFrame
        }

        case 0x02: // ST_REASSOCIATIONREQUEST
        {
            // Ieee80211ReassociationRequestFrame
        }

        case 0x01: // ST_ASSOCIATIONRESPONSE
        {
            // Ieee80211AssociationResponseFrame
        }

        case 0x03: // ST_REASSOCIATIONRESPONSE
        {
            // Ieee80211ReassociationResponseFrame
        }

        case 0x80: // ST_BEACON
        {
            // Ieee80211BeaconFrame
        }

        case 0x50: //  ST_PROBERESPONSE
        {
            // Ieee80211ProbeResponseFrame
        }

        case 0xD0: // type = ST_ACTION
        {
            // Ieee80211ActionFrame
        }

        default:
            throw cRuntimeError("Ieee80211Serializer: cannot serialize the frame");
    }
}

