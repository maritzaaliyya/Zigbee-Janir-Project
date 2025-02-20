#include "CurtainOpenerApp.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "CurtainStateTag.h"
#include "inet/common/Ptr.h"
#include <iomanip>
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/transportlayer/udp/Udp.h"
#include <inet/common/ModuleAccess.h>
#include "inet/networklayer/contract/IInterfaceTable.h"
#include <inet/common/ModuleAccess.h>

namespace inet {

Define_Module(CurtainOpenerApp);

CurtainOpenerApp::~CurtainOpenerApp()
{
    cancelAndDelete(selfMsg);
}

void CurtainOpenerApp::initialize(int stage)
{
    ClockUserModuleMixin::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        localPort = par("localPort");
        destPort = par("destPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        dontFragment = par("dontFragment");
        if (stopTime >= CLOCKTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new ClockEvent("sendTimer");
        CurtainClose = false;
        udp = check_and_cast<Udp*>(getModuleByPath("udp"));
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        auto socket = check_and_cast<UdpSocket*>(udp->getSubmodule("socket"));
        if(!ift)
            throw cRuntimeError("Interface table not found.");
        int interfaceId = ift->getInterfaceById(0)->getInterfaceId();
        socket->bind(localPort);
    }
}

void CurtainOpenerApp::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    ApplicationBase::finish();
}

void CurtainOpenerApp::setSocketOptions()
{
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket.setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket.setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    check_and_cast<UdpSocket*>(udp->getSubmodule("socket"))->setCallback(this);

    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        NetworkInterface *ie = ift->findInterfaceByName(multicastInterface);
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
    if (joinLocalMulticastGroups) {
        MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
    }
    socket.setCallback(this);
}

L3Address CurtainOpenerApp::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void CurtainOpenerApp::sendPacket()
{
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);
    const auto& payload = makeShared<ApplicationPacket>();
    payload->addTag<>()->setIsOn(CurtainClose);
    payload->setChunkLength(B(par("messageLength")));
    payload->setSequenceNumber(numSent);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    check_and_cast<UdpSocket*>(udp->getSubmodule("socket"))->sendTo(packet, destAddr, destPort);
    numSent++;
}

void CurtainOpenerApp::processStart()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        destAddresses.push_back(result);
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        processSend();
    }
    else {
        if (stopTime >= CLOCKTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleClockEventAt(stopTime, selfMsg);
        }
    }
    L3AddressResolver().tryResolve(par("controllerAddress").stringValue(), controllerAddress);


            if (controllerAddress.isUnspecified()) {
                throw cRuntimeError("Cannot resolve controller address");
            }


        // Schedule the first check for on/off time
        scheduleOnOffCheck();
}

void CurtainOpenerApp::processSend()
{
    sendPacket();
    clocktime_t d = par("sendInterval");
    if (stopTime < CLOCKTIME_ZERO || getClockTime() + d < stopTime) {
        selfMsg->setKind(SEND);
        scheduleClockEventAfter(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleClockEventAt(stopTime, selfMsg);
    }
}

void CurtainOpenerApp::processStop()
{
    socket.close();
}

void CurtainOpenerApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "onOffCheck") == 0) {
                    checkOnOffTime();
                    scheduleOnOffCheck();// Reschedule the check
                    delete msg;
                } else {
                    ASSERT(msg == selfMsg);
                            switch (selfMsg->getKind()) {
                                case START:
                                    processStart();
                                    break;

                                case SEND:
                                    processSend();
                                    break;

                                case STOP:
                                    processStop();
                                    break;

                                default:
                                    throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
                            }
                }
    }
    else
        socket.processMessage(msg);
}

void CurtainOpenerApp::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    processPacket(packet);
}

void CurtainOpenerApp::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void CurtainOpenerApp::socketClosed(UdpSocket *socket)
{
    if (operationalState == State::STOPPING_OPERATION)
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void CurtainOpenerApp::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();

    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void CurtainOpenerApp::processPacket(Packet *pk)
{
    emit(packetReceivedSignal, pk);
        EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
        delete pk;
        numReceived++;
}


void CurtainOpenerApp::handleStartOperation(LifecycleOperation *operation)
{
    clocktime_t start = std::max(startTime, getClockTime());
    if ((stopTime < CLOCKTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleClockEventAt(start, selfMsg);
    }
}

void CurtainOpenerApp::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    check_and_cast<UdpSocket*>(udp->getSubmodule("socket"))->close();
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void CurtainOpenerApp::handleCrashOperation(LifecycleOperation *operation)
{
    cancelClockEvent(selfMsg);
    check_and_cast<UdpSocket*>(udp->getSubmodule("socket"))->destroy();
    socket.destroy(); // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}

void CurtainOpenerApp::scheduleOnOffCheck()
{
    // Schedule a self-message to check the time and turn the lamp on/off
    scheduleAt(simTime() + simTime().inUnit(SIMTIME_S), new cMessage("onOffCheck"));// Check every second
}

void CurtainOpenerApp::checkOnOffTime()
{
    // Get current simulation time
    time_t now = simTime().inUnit(SIMTIME_S);  // or simTime().raw() if using raw time

    // Convert to a tm struct for easy manipulation
    std::tm* now_tm = localtime(&now); // Use localtime for the simulation's timezone



    // Define on/off times
    int onHour = 17;
    int onMin = 30;
    int offHour = 5;
    int offMin = 30;


    // Check if current time is between on/off times
    bool shouldBeOn = (now_tm->tm_hour > onHour || (now_tm->tm_hour == onHour && now_tm->tm_min >= onMin)) ||
                      (now_tm->tm_hour < offHour || (now_tm->tm_hour == offHour && now_tm->tm_min < offMin));



    if (shouldBeOn && !CurtainClose) {
          EV_INFO << "Closing curtain at " <<  std::put_time(now_tm, "%H:%M:%S") << "\n";

        CurtainClose = true;

    } else if (!shouldBeOn && CurtainClose) {
        EV_INFO << "Opening Curtain at " << std::put_time(now_tm, "%H:%M:%S") << "\n";

        CurtainClose = false;


    }
}

} // namespace inet
