# Battleboats-with-Arduinos

# Summary #
This C program allowed two Arduino microcontrollers to play the popular game Battleboats on the Arduinos' LCD display. The game could be played between two human players (using one Arduino each), or between one human player vs. one computer player (using one Arduino each). The Arduinos would communicate using an encoding and decoding protocol using checksums and the data was transferred back and forth with the use of wires connected to certain ports on the board. Each board would receive the data and then interpret the data for a command in the game. Players would use the buttons on the board to set up the position of the chips and choose coordinates to attack. 

Here is a video that went through the gameplay: https://classes.soe.ucsc.edu/cmpe013/Spring17/Labs/Lab9/Lab9_demo.mp4

# Technologies Used #
Most of the code was written in C with some in C++. The intention of this lab was to use C/C++ to load and execute a program on the microcontroller. As aforementioned, the microcontrollers were connected with wires that linked to the proper ports on both the Arduinos. 

# Files #
Most of the default libraries were provided to us by our instructor. These libraries/imports included converting the data from the port to code that could be interpreted by C/C++. 

The files that are of most importance are ArtificialAgent.c, Field.c, Protocol.c. 

ArtificialAgent.c was the computer client. This file utilized a bunch of switch cases to analyze the state of the game (on each Arduino), resembling a Finite State Machine. For instance, if it was determined that the "state" of the board is that it's waiting for a command from the opponent, the board wait to analyze the incoming command and then switch to another case based on what conditions were met. This file also features the A.I client that was developed, which was an extension of the Finite State Machine. The client would randomly guess at first (never the same location twice), waiting to register a hit. If a hit was registered, you could assume that the next logical guess would be either N/S by 1 square or E/W by 1 square of the location of the hit. The client would store the location of the previous hit and execute the aforementioned logic to eliminate the opponent's boats.

Field.c was the file that was developed to build the environment. This environment needed to be able to have 4 different sized boats, arranged in any orientation. Furthermore, there needed to be a grid layout where everying was mapped on (boats, hits). This file also imported some files to convert the grid and the boats to a visible line on the Arduino's LCD. 

Protocol.c was the file that was developed to communicate between the Arduinos. Different types of messages (differentiated by a certain part of the string being different - the string that was transferred between Arduinos) would convey different messages - hence they all needed to be interpreted differently. A checksum was utilized to ensure that the data that was transferred was accurate and valid. Furthermore, once this data was encoded/decoded (encoded when passing data to the other Arduino, decoded when receiving data from the other Arduino), the Agent file would "run" the game with the updated information.

Please refer to the video linked above for the gameplay!
