import inet.node.inet.SensorNode;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;

network ZigbeeNetwork
{
    @display("bgb=600,400;bgi=showcases/floorplan,s");

    submodules:
        curtain: SensorNode {
            @display("p=100,50;i=misc/square");
        }
        lamp1: SensorNode {
            @display("p=50,250;i=status/bulb");
        }
        lamp2: SensorNode {
            @display("p=350,300;i=status/bulb");
        }
        lamp3: SensorNode {
            @display("p=200,50;i=status/bulb");
        }
        controller: SensorNode {
            @display("p=250,200;i=misc/sensorgateway");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=400,50");
        }

        radioMedium: Ieee802154NarrowbandScalarRadioMedium {
            @display("p=500,300");
        }
}
