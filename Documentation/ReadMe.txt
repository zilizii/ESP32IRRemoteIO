This project is a demo to try the ESP32 RMT HW functions to create a Pulse With Modulation IR Demo to imitate a Chinese LED 24 key IR remote for a RGB controller.

Demo : Capture the 24 IR remote Control signals -> Done 
Check : 24_b_IR_remote.xlsm for the exlored IR codes --> done by this demo application


1st stage goal : Develop the basic levels -> 80% done, under test
2nd stage goal : explore more remote protocoll
3rd stage goal : extend with MQTT
4th stage goal : Store the differntial remote data in a structured way in the controller / SD card.
5th stage goal : Schematic board design + enclosure design
6th stage goal : Prepare proper documentation for the code.

1st test: 
Step 1: Reset the controller
Step 2: ESP 32 shall generate the TURN ON command via IR for the RGB Controller to turn on
Step 3: ESP 32 shall generate the IR codes for a cycle demo where RGB colours cycle.
Issue was the Stop bit position. --> Solved :-) 
