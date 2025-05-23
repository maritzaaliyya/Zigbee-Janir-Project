import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.networklayer.common.InterfaceTable;

module CurtainOpenerApp like IApp
{
    parameters:
        string controllerAddress;
        string interfaceTableModule = default("^.interfaceTable");   // The path to the InterfaceTable module
        string clockModule = default(""); // Relative path of a module that implements IClock; optional
        int localPort = default(-1);  // Local port (-1: use ephemeral port)
        string destAddresses = default(""); // List of IP addresses, separated by spaces ("": don't send)
        string localAddress = default("");
        string packetName = default("SmartLampAppData");
        int destPort;
        volatile int messageLength @unit(B); // Length of messages to generate, in bytes
        double startTime @unit(s) = default(sendInterval); // Application start time (start of the first packet)
        double stopTime @unit(s) = default(-1s);  // Time of finishing sending, -1s means forever
        volatile double sendInterval @unit(s); // Should usually be a random value, e.g. exponential(1)
        int timeToLive = default(-1); // If not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        bool dontFragment = default(false); // If true, asks IP to not fragment the message during routing
        int dscp = default(-1); // If not -1, set the DSCP field (on IPv4/IPv6) of sent packets to this value
        int tos = default(-1); // If not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        string multicastInterface = default("");  // If not empty, set the multicast output interface option on the socket (interface name expected)
        bool receiveBroadcast = default(false); // If true, makes the socket receive broadcast packets
        bool joinLocalMulticastGroups = default(false); // If true, makes the socket receive packets from all multicast groups set on local interfaces
        @display("i=block/app");
        @lifecycleSupport;
        double stopOperationExtraTime @unit(s) = default(-1s);    // Extra time after lifecycle stop operation finished
        double stopOperationTimeout @unit(s) = default(2s);    // Timeout value for lifecycle stop operation
        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);
        @statistic[packetReceived](title="packets received"; source=packetReceived; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[throughput](title="throughput"; unit=bps; source="throughput(packetReceived)"; record=vector);
        @statistic[packetSent](title="packets sent"; source=packetSent; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[rcvdPkLifetime](title="received packet lifetime"; source="dataAge(packetReceived)"; unit=s; record=stats,vector; interpolationmode=none);
        @statistic[rcvdPkSeqNo](title="received packet sequence number"; source="appPkSeqNo(packetReceived)"; record=vector; interpolationmode=none);
    
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
        input ipIn @labels(UdpHeader,Ipv4ControlInfo/up,Ipv6ControlInfo/up);
        output ipOut @labels(UdpHeader,Ipv4ControlInfo/down,Ipv6ControlInfo/down);

    submodules:
        udp: Udp {
            @display("p=20,20");
        }

    connections:
        udp.appOut --> socketOut;
        socketIn --> udp.appIn;
        udp.ipOut --> ipOut;
        ipIn --> udp.ipIn;
}
