//
// Copyright (C) 2006 Andras Babos and Andras Varga
// Copyright (C) 2012 OpenSim Ltd.
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <string>
#include <map>
#include <stdlib.h>
#include <memory.h>

#include "OSPFRouting.h"

#include "InterfaceTableAccess.h"
#include "IPv4Address.h"
#include "IPv4ControlInfo.h"
#include "IPvXAddressResolver.h"
#include "MessageHandler.h"
#include "OSPFArea.h"
#include "OSPFcommon.h"
#include "OSPFInterface.h"
#include "PatternMatcher.h"
#include "RoutingTableAccess.h"


Define_Module(OSPFRouting);


OSPFRouting::OSPFRouting()
{
    ospfRouter = NULL;
}

/**
 * Destructor.
 * Deletes the whole OSPF data structure.
 */
OSPFRouting::~OSPFRouting()
{
    delete ospfRouter;
}


/**
 * OMNeT++ init method.
 * Runs at stage 2 after interfaces are registered(stage 0) and the routing
 * table is initialized(stage 1). Loads OSPF configuration information from
 * the config XML.
 * @param stage [in] The initialization stage.
 */
void OSPFRouting::initialize(int stage)
{
    // we have to wait for stage 2 until interfaces get registered(stage 0)
    // and routerId gets assigned(stage 3)
    if (stage == 4)
    {
        rt = RoutingTableAccess().get();
        ift = InterfaceTableAccess().get();

        // Get routerId
        ospfRouter = new OSPF::Router(rt->getRouterId(), this);

        // read the OSPF AS configuration
        cXMLElement *ospfConfig = par("ospfConfig").xmlValue();
        if (!loadConfigFromXML(ospfConfig))
            error("Error reading AS configuration from %s", ospfConfig->getSourceLocation());

        ospfRouter->addWatches();
    }
}


/**
 * Forwards OSPF messages to the message handler object of the OSPF data structure.
 * @param msg [in] The OSPF message.
 */
void OSPFRouting::handleMessage(cMessage *msg)
{
    ospfRouter->getMessageHandler()->messageReceived(msg);
}

/**
 * Insert a route learn by BGP in OSPF routingTable as an external route.
 * Used by the BGPRouting module.
 */
void OSPFRouting::insertExternalRoute(const std::string & ifName, const OSPF::IPv4AddressRange &netAddr)
{
    int ifIndex = resolveInterfaceName(ifName);
    simulation.setContext(this);
    OSPFASExternalLSAContents newExternalContents;
    newExternalContents.setRouteCost(OSPF_BGP_DEFAULT_COST);
    newExternalContents.setExternalRouteTag(OSPF_EXTERNAL_ROUTES_LEARNED_BY_BGP);
    const IPv4Address netmask = netAddr.mask;
    newExternalContents.setNetworkMask(netmask);
    ospfRouter->updateExternalRoute(netAddr.address, newExternalContents, ifIndex);
}

/**
 * Return true if the route is in OSPF external LSA Table, false else.
 * Used by the BGPRouting module.
 */
bool OSPFRouting::checkExternalRoute(const IPv4Address &route)
{
    for (unsigned long i=1; i < ospfRouter->getASExternalLSACount(); i++)
    {
        OSPF::ASExternalLSA* externalLSA = ospfRouter->getASExternalLSA(i);
        IPv4Address externalAddr = externalLSA->getHeader().getLinkStateID();
        if (externalAddr == route) //FIXME was this meant???
            return true;
    }
    return false;
}

/**
 * Looks up the interface name in IInterfaceTable, and returns interfaceId a.k.a ifIndex.
 */
int OSPFRouting::resolveInterfaceName(const std::string& name) const
{
    InterfaceEntry* ie = ift->getInterfaceByName(name.c_str());
    if (!ie)
        throw cRuntimeError("Error reading XML config: IInterfaceTable contains no interface named '%s'", name.c_str());

    return ie->getInterfaceId();
}

/**
 * Loads a list of OSPF Areas connected to this router from the config XML.
 * @param routerNode [in]  XML node describing this router.
 * @param areaList   [out] A hash of OSPF Areas connected to this router. The hash key is the Area ID.
 */
void OSPFRouting::getAreaListFromXML(const cXMLElement& routerNode, std::map<std::string, int>& areaList) const
{
    cXMLElementList routerConfig = routerNode.getChildren();
    for (cXMLElementList::iterator routerConfigIt = routerConfig.begin(); routerConfigIt != routerConfig.end(); routerConfigIt++) {
        std::string nodeName = (*routerConfigIt)->getTagName();
        if ((nodeName == "PointToPointInterface") ||
            (nodeName == "BroadcastInterface") ||
            (nodeName == "NBMAInterface") ||
            (nodeName == "PointToMultiPointInterface"))
        {
            std::string areaId = getStrAttrOrPar(**routerConfigIt, "areaID");
            if (areaList.find(areaId) == areaList.end()) {
                areaList[areaId] = 1;
            }
        }
    }
}


/**
 * Loads basic configuration information for a given area from the config XML.
 * Reads the configured address ranges, and whether this Area should be handled as a stub Area.
 * @param asConfig [in] XML node describing the configuration of the whole Autonomous System.
 * @param areaID   [in] The Area to be added to the OSPF data structure.
 */
void OSPFRouting::loadAreaFromXML(const cXMLElement& asConfig, const std::string& areaID)
{
    std::string areaXPath("Area[@id='");
    areaXPath += areaID;
    areaXPath += "']";

    cXMLElement* areaConfig = asConfig.getElementByPath(areaXPath.c_str());
    if (areaConfig == NULL) {
        error("No configuration for Area ID: %s at %s", areaID.c_str(), asConfig.getSourceLocation());
    }
    else {
        EV << "    loading info for Area id = " << areaID << "\n";
    }

    OSPF::Area* area = new OSPF::Area(IPv4Address(areaID.c_str()));
    cXMLElementList areaDetails = areaConfig->getChildren();
    for (cXMLElementList::iterator arIt = areaDetails.begin(); arIt != areaDetails.end(); arIt++) {
        std::string nodeName = (*arIt)->getTagName();
        if (nodeName == "AddressRange") {
            OSPF::IPv4AddressRange addressRange;
            addressRange.address = ipv4AddressFromAddressString(getRequiredAttribute(**arIt, "address"));
            addressRange.mask = ipv4NetmaskFromAddressString(getRequiredAttribute(**arIt, "mask"));
            addressRange.address = addressRange.address & addressRange.mask;
            std::string status = getRequiredAttribute(**arIt, "status");
            area->addAddressRange(addressRange, status == "Advertise");
        }
        else if (nodeName == "Stub") {
            if (areaID == "0.0.0.0")
                error("The backbone cannot be configured as a stub at %s", (*arIt)->getSourceLocation());
            area->setExternalRoutingCapability(false);
            area->setStubDefaultCost(atoi(getRequiredAttribute(**arIt, "defaultCost")));
        }
        else
            error("Invalid node '%s' at %s", nodeName.c_str(), (*arIt)->getSourceLocation());
    }
    // Add the Area to the router
    ospfRouter->addArea(area);
}

int OSPFRouting::getIntAttrOrPar(const cXMLElement& ifConfig, const char *name) const
{
    const char* attrStr = ifConfig.getAttribute(name);
    if (attrStr && *attrStr)
        return atoi(attrStr);
    return par(name).longValue();
}

const char *OSPFRouting::getStrAttrOrPar(const cXMLElement& ifConfig, const char *name) const
{
    const char* attrStr = ifConfig.getAttribute(name);
    if (attrStr && *attrStr)
        return attrStr;
    return par(name).stringValue();
}

const char *OSPFRouting::getRequiredAttribute(const cXMLElement& node, const char *attr) const
{
    const char *s = node.getAttribute(attr);
    if (!(s && *s))
        error("required attribute %s of <%s> missing at %s",
              attr, node.getTagName(), node.getSourceLocation());
    return s;
}


/**
 * Loads OSPF configuration information for a router interface.
 * Handles POINTTOPOINT, BROADCAST, NBMA and POINTTOMULTIPOINT interfaces.
 * @param ifConfig [in] XML node describing the configuration of an OSPF interface.
 */
void OSPFRouting::loadInterfaceParameters(const cXMLElement& ifConfig)
{
    OSPF::Interface* intf = new OSPF::Interface;
    std::string ifName = getRequiredAttribute(ifConfig, "ifName");
    int ifIndex = resolveInterfaceName(ifName);
    std::string interfaceType = ifConfig.getTagName();

    EV << "        loading " << interfaceType << " " << ifName << " ifIndex[" << ifIndex << "]\n";

    intf->setIfIndex(ifIndex);
    if (interfaceType == "PointToPointInterface") {
        intf->setType(OSPF::Interface::POINTTOPOINT);
    } else if (interfaceType == "BroadcastInterface") {
        intf->setType(OSPF::Interface::BROADCAST);
    } else if (interfaceType == "NBMAInterface") {
        intf->setType(OSPF::Interface::NBMA);
    } else if (interfaceType == "PointToMultiPointInterface") {
        intf->setType(OSPF::Interface::POINTTOMULTIPOINT);
    } else {
        delete intf;
        error("Loading %s ifIndex[%d] aborted at %s", interfaceType.c_str(), ifIndex, ifConfig.getSourceLocation());
    }

    OSPF::AreaID areaID = IPv4Address(getStrAttrOrPar(ifConfig, "areaID"));
    intf->setAreaID(areaID);

    intf->setOutputCost(getIntAttrOrPar(ifConfig, "interfaceOutputCost"));

    intf->setRetransmissionInterval(getIntAttrOrPar(ifConfig, "retransmissionInterval"));

    intf->setTransmissionDelay(getIntAttrOrPar(ifConfig, "interfaceTransmissionDelay"));

    if (interfaceType == "BroadcastInterface" || interfaceType == "NBMAInterface")
        intf->setRouterPriority(getIntAttrOrPar(ifConfig, "routerPriority"));

    intf->setHelloInterval(getIntAttrOrPar(ifConfig, "helloInterval"));

    intf->setRouterDeadInterval(getIntAttrOrPar(ifConfig, "routerDeadInterval"));

    std::string authenticationType = getStrAttrOrPar(ifConfig, "authenticationType");
    if (authenticationType == "SimplePasswordType") {
        intf->setAuthenticationType(OSPF::SIMPLE_PASSWORD_TYPE);
    } else if (authenticationType == "CrytographicType") {
        intf->setAuthenticationType(OSPF::CRYTOGRAPHIC_TYPE);
    } else if (authenticationType == "NullType") {
        intf->setAuthenticationType(OSPF::NULL_TYPE);
    } else {
        throw cRuntimeError("Invalid AuthenticationType '%s' at %s", authenticationType.c_str(), ifConfig.getSourceLocation());
    }

    std::string key = getStrAttrOrPar(ifConfig, "authenticationKey");
    OSPF::AuthenticationKeyType keyValue;
    memset(keyValue.bytes, 0, 8 * sizeof(char));
    int keyLength = key.length();
    if ((keyLength > 4) && (keyLength <= 18) && (keyLength % 2 == 0) && (key[0] == '0') && (key[1] == 'x')) {
        for (int i = keyLength; (i > 2); i -= 2) {
            keyValue.bytes[(i - 2) / 2] = hexPairToByte(key[i - 1], key[i]);
        }
    }
    intf->setAuthenticationKey(keyValue);

    if (interfaceType == "NBMAInterface")
        intf->setPollInterval(getIntAttrOrPar(ifConfig, "pollInterval"));

    cXMLElementList ifDetails = ifConfig.getChildren();

    for (cXMLElementList::iterator ifElemIt = ifDetails.begin(); ifElemIt != ifDetails.end(); ifElemIt++) {
        std::string nodeName = (*ifElemIt)->getTagName();
        if ((interfaceType == "NBMAInterface") && (nodeName == "NBMANeighborList")) {
            cXMLElementList neighborList = (*ifElemIt)->getChildren();
            for (cXMLElementList::iterator neighborIt = neighborList.begin(); neighborIt != neighborList.end(); neighborIt++) {
                std::string neighborNodeName = (*neighborIt)->getTagName();
                if (neighborNodeName == "NBMANeighbor") {
                    OSPF::Neighbor* neighbor = new OSPF::Neighbor;
                    neighbor->setAddress(ipv4AddressFromAddressString(getRequiredAttribute(**neighborIt, "networkInterfaceAddress")));
                    neighbor->setPriority(atoi(getRequiredAttribute(**neighborIt, "neighborPriority")));
                    intf->addNeighbor(neighbor);
                }
            }
        }
        if ((interfaceType == "PointToMultiPointInterface") && (nodeName == "PointToMultiPointNeighborList")) {
            cXMLElementList neighborList = (*ifElemIt)->getChildren();
            for (cXMLElementList::iterator neighborIt = neighborList.begin(); neighborIt != neighborList.end(); neighborIt++) {
                std::string neighborNodeName = (*neighborIt)->getTagName();
                if (neighborNodeName == "PointToMultiPointNeighbor") {
                    OSPF::Neighbor* neighbor = new OSPF::Neighbor;
                    neighbor->setAddress(ipv4AddressFromAddressString((*neighborIt)->getNodeValue()));
                    intf->addNeighbor(neighbor);
                }
            }
        }
    }
    // add the interface to it's Area
    OSPF::Area* area = ospfRouter->getAreaByID(areaID);
    if (area != NULL) {
        area->addInterface(intf);
        intf->processEvent(OSPF::Interface::INTERFACE_UP); // notification should come from the blackboard...
    } else {
        delete intf;
        error("Loading %s ifIndex[%d] in Area %s aborted at %s", interfaceType.c_str(), ifIndex, areaID.str().c_str(), ifConfig.getSourceLocation());
    }
}


/**
 * Loads the configuration information of a route outside of the Autonomous System(external route).
 * @param externalRouteConfig [in] XML node describing the parameters of an external route.
 */
void OSPFRouting::loadExternalRoute(const cXMLElement& externalRouteConfig)
{
    std::string ifName = getRequiredAttribute(externalRouteConfig, "ifName");
    int ifIndex = resolveInterfaceName(ifName);
    OSPFASExternalLSAContents asExternalRoute;
    OSPF::RoutingTableEntry externalRoutingEntry; // only used here to keep the path cost calculation in one place
    OSPF::IPv4AddressRange networkAddress;

    EV << "        loading ExternalInterface " << ifName << " ifIndex[" << ifIndex << "]\n";

    networkAddress.address = ipv4AddressFromAddressString(getRequiredAttribute(externalRouteConfig, "advertisedExternalNetworkAddress"));
    networkAddress.mask = ipv4NetmaskFromAddressString(getRequiredAttribute(externalRouteConfig, "advertisedExternalNetworkMask"));
    networkAddress.address = networkAddress.address & networkAddress.mask;
    asExternalRoute.setNetworkMask(networkAddress.mask);

    int routeCost = getIntAttrOrPar(externalRouteConfig, "externalInterfaceOutputCost");
    asExternalRoute.setRouteCost(routeCost);

    std::string metricType = getStrAttrOrPar(externalRouteConfig, "externalInterfaceOutputType");
    if (metricType == "Type2") {
        asExternalRoute.setE_ExternalMetricType(true);
        externalRoutingEntry.setType2Cost(routeCost);
        externalRoutingEntry.setPathType(OSPF::RoutingTableEntry::TYPE2_EXTERNAL);
    } else if (metricType == "Type1") {
        asExternalRoute.setE_ExternalMetricType(false);
        externalRoutingEntry.setCost(routeCost);
        externalRoutingEntry.setPathType(OSPF::RoutingTableEntry::TYPE1_EXTERNAL);
    } else {
        throw cRuntimeError("Invalid 'externalInterfaceOutputType' at interface '%s' at ", ifName.c_str(), externalRouteConfig.getSourceLocation());
    }

    asExternalRoute.setForwardingAddress(ipv4AddressFromAddressString(getRequiredAttribute(externalRouteConfig, "forwardingAddress")));

    long externalRouteTagVal = 0;   // default value
    const char *externalRouteTag = externalRouteConfig.getAttribute("externalRouteTag");
    if (externalRouteTag && *externalRouteTag)
    {
        char *endp = NULL;
        externalRouteTagVal = strtol(externalRouteTag, &endp, 0);
        if(*endp)
            throw cRuntimeError("Invalid externalRouteTag='%s' at %s", externalRouteTag, externalRouteConfig.getSourceLocation());
    }
    asExternalRoute.setExternalRouteTag(externalRouteTagVal);

    // add the external route to the OSPF data structure
    ospfRouter->updateExternalRoute(networkAddress.address, asExternalRoute, ifIndex);
}


/**
 * Loads the configuration of a host getRoute(a host directly connected to the router).
 * @param hostRouteConfig [in] XML node describing the parameters of a host route.
 */
void OSPFRouting::loadHostRoute(const cXMLElement& hostRouteConfig)
{
    OSPF::HostRouteParameters hostParameters;
    OSPF::AreaID hostArea;

    std::string ifName = getRequiredAttribute(hostRouteConfig, "ifName");
    hostParameters.ifIndex = resolveInterfaceName(ifName);

    EV << "        loading HostInterface " << ifName << " ifIndex[" << static_cast<short> (hostParameters.ifIndex) << "]\n";

    hostArea = ipv4AddressFromAddressString(getStrAttrOrPar(hostRouteConfig, "areaID"));
    hostParameters.address = ipv4AddressFromAddressString(getRequiredAttribute(hostRouteConfig, "attachedHost"));
    hostParameters.linkCost = getIntAttrOrPar(hostRouteConfig, "linkCost");

    // add the host route to the OSPF data structure.
    OSPF::Area* area = ospfRouter->getAreaByID(hostArea);
    if (area != NULL) {
        area->addHostRoute(hostParameters);
    } else {
        error("Loading HostInterface ifIndex[%d] in Area %s aborted at %s", hostParameters.ifIndex, hostArea.str().c_str(), hostRouteConfig.getSourceLocation());
    }
}


/**
 * Loads the configuration of an OSPf virtual link(virtual connection between two backbone routers).
 * @param virtualLinkConfig [in] XML node describing the parameters of a virtual link.
 */
void OSPFRouting::loadVirtualLink(const cXMLElement& virtualLinkConfig)
{
    OSPF::Interface* intf = new OSPF::Interface;
    std::string endPoint = getRequiredAttribute(virtualLinkConfig, "endPointRouterID");
    OSPF::Neighbor* neighbor = new OSPF::Neighbor;

    EV << "        loading VirtualLink to " << endPoint << "\n";

    intf->setType(OSPF::Interface::VIRTUAL);
    neighbor->setNeighborID(ipv4AddressFromAddressString(endPoint.c_str()));
    intf->addNeighbor(neighbor);

    intf->setTransitAreaID(ipv4AddressFromAddressString(getRequiredAttribute(virtualLinkConfig, "transitAreaID")));

    intf->setRetransmissionInterval(getIntAttrOrPar(virtualLinkConfig, "retransmissionInterval"));

    intf->setTransmissionDelay(getIntAttrOrPar(virtualLinkConfig, "interfaceTransmissionDelay"));

    intf->setHelloInterval(getIntAttrOrPar(virtualLinkConfig, "helloInterval"));

    intf->setRouterDeadInterval(getIntAttrOrPar(virtualLinkConfig, "routerDeadInterval"));

    std::string authenticationType = getStrAttrOrPar(virtualLinkConfig, "authenticationType");
    if (authenticationType == "SimplePasswordType") {
        intf->setAuthenticationType(OSPF::SIMPLE_PASSWORD_TYPE);
    } else if (authenticationType == "CrytographicType") {
        intf->setAuthenticationType(OSPF::CRYTOGRAPHIC_TYPE);
    } else if (authenticationType == "NullType") {
        intf->setAuthenticationType(OSPF::NULL_TYPE);
    } else {
        throw cRuntimeError("Invalid AuthenticationType '%s' at %s", authenticationType.c_str(), virtualLinkConfig.getSourceLocation());
    }

    std::string key = getStrAttrOrPar(virtualLinkConfig, "authenticationKey");
    OSPF::AuthenticationKeyType keyValue;
    memset(keyValue.bytes, 0, 8 * sizeof(char));
    int keyLength = key.length();
    if ((keyLength > 4) && (keyLength <= 18) && (keyLength % 2 == 0) && (key[0] == '0') && (key[1] == 'x')) {
        for (int i = keyLength; (i > 2); i -= 2) {
            keyValue.bytes[(i - 2) / 2] = hexPairToByte(key[i - 1], key[i]);
        }
    }
    intf->setAuthenticationKey(keyValue);

    // add the virtual link to the OSPF data structure.
    OSPF::Area* transitArea = ospfRouter->getAreaByID(intf->getAreaID());
    OSPF::Area* backbone = ospfRouter->getAreaByID(OSPF::BACKBONE_AREAID);

    if ((backbone != NULL) && (transitArea != NULL) && (transitArea->getExternalRoutingCapability())) {
        backbone->addInterface(intf);
    } else {
        delete intf;
        error("Loading VirtualLink to %s through Area %s aborted at ", endPoint.c_str(), intf->getAreaID().str().c_str(), virtualLinkConfig.getSourceLocation());
    }
}


/**
 * Loads the configuration of the OSPF data structure from the config XML.
 * @param filename [in] The path of the XML config file.
 * @return True if the configuration was succesfully loaded.
 * @throws an getError() otherwise.
 */
bool OSPFRouting::loadConfigFromXML(cXMLElement *asConfig)
{
    if (strcmp(asConfig->getTagName(), "OSPFASConfig"))
        error("Cannot read OSPF configuration, unaccepted '%s' note at %s", asConfig->getTagName(), asConfig->getSourceLocation());

    cModule *myNode = findContainingNode(this);

    ASSERT(myNode);
    std::string nodeFullPath = myNode->getFullPath();
    std::string nodeShortenedFullPath = nodeFullPath.substr(nodeFullPath.find('.') + 1);

    // load information on this router
    cXMLElementList routers = asConfig->getElementsByTagName("Router");
    cXMLElement* routerNode = NULL;
    for (cXMLElementList::iterator routerIt = routers.begin(); routerIt != routers.end(); routerIt++) {
        const char* nodeName = getRequiredAttribute(*(*routerIt), "name");
        inet::PatternMatcher pattern(nodeName, true, true, true);
        if (pattern.matches(nodeFullPath.c_str()) || pattern.matches(nodeShortenedFullPath.c_str()))   // match Router@name and fullpath of my node
        {
            routerNode = *routerIt;
            break;
        }
    }
    if (routerNode == NULL) {
        error("No configuration for Router '%s' at '%s'", nodeFullPath.c_str(), asConfig->getSourceLocation());
    }

    EV << "OSPFRouting: Loading info for Router " << nodeFullPath << "\n";

    std::map<std::string, int> areaList;
    getAreaListFromXML(*routerNode, areaList);

    // load area information
    for (std::map<std::string, int>::iterator areaIt = areaList.begin(); areaIt != areaList.end(); areaIt++) {
        loadAreaFromXML(*asConfig, areaIt->first);
    }
    // if the router is an area border router then it MUST be part of the backbone(area 0)
    if ((areaList.size() > 1) && (areaList.find("0.0.0.0") == areaList.end())) {
        loadAreaFromXML(*asConfig, "0.0.0.0");
    }

    // load interface information
    cXMLElementList routerConfig = routerNode->getChildren();
    for (cXMLElementList::iterator routerConfigIt = routerConfig.begin(); routerConfigIt != routerConfig.end(); routerConfigIt++) {
        std::string nodeName = (*routerConfigIt)->getTagName();
        if ((nodeName == "PointToPointInterface") ||
            (nodeName == "BroadcastInterface") ||
            (nodeName == "NBMAInterface") ||
            (nodeName == "PointToMultiPointInterface"))
        {
            loadInterfaceParameters(*(*routerConfigIt));
        }
        else if (nodeName == "ExternalInterface") {
            loadExternalRoute(*(*routerConfigIt));
        }
        else if (nodeName == "HostInterface") {
            loadHostRoute(*(*routerConfigIt));
        }
        else if (nodeName == "VirtualLink") {
            loadVirtualLink(*(*routerConfigIt));
        }
        else if (nodeName == "RFC1583Compatible") {
            ospfRouter->setRFC1583Compatibility(true);
        } else {
            throw cRuntimeError("Invalid '%s' node in Router '%s' at %s",
                    nodeName.c_str(), nodeFullPath.c_str(), (*routerConfigIt)->getSourceLocation());
        }

    }
    return true;
}

