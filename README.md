# nodeMCU-plant-watcher-
Hello!
This is the code and schematics for a standalone ESP8266 NodeMCU controlled smart farm module.

The repo includes the codes for:

A standalone IoT plant watch server that connects to your local router and allows you to monitor
    and water 1 or 2 plants as well as collect environmental data

an implementation of the module with MQTT for data storage. 

an implementation of the module that creates a mesh network allowing multiple nodes to connect with
    each other over a greater range acting as acess points and stations.
  --> please note that you will need to use martin's lwip nat library 
      setup for the mesh network to work properly (https://github.com/martin-ger/lwip_nat_arduino)
In the hardware folder the files include a full schematic for the PCB as well as a picture of the working module.
Please note that you will NOT be able to program the module if you solder it onto the board 
    (the reset pin is connected to GPIO16 and will NOT allow new uploads while connected to the board)
 
 HARDWARE:
          
          NodeMCU ESP8266 (PCB was designed with the CP2102 module)
          
          DHT22 (or DHT11, depending on your needs)
           
          ADS1115 16 bit Analog to digital Converter
          
          Light photo resistor 
          
          Soil Hygrometer (can support 2)
          
          Rain sensor (optional)
          
          5V water pump (or valve in the case of more than one node)
          
          LM2956 DC-DC Buck converter
          
          Battery pack/Batteries
          
          Some Resistors, Diodes, and capacitors (values depend on your power source)
    
 Pins used:
 
      NodeMCU
      
          D0: RST
          
          D1 : SCL (ADS1115)
          
          D2 : SDA (ADS1115)
          
          D3 : DHT22
          
          D5 : Waterpump/Valve
          
      ADS1115:
          
          0 : Light sensor
          1 : Hygrometer
          2 : extra hygrometer
          3 : Battery monitor
          
        
          
          
  
