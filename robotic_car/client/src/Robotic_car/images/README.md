This is the folder where the images saved in the client ends up.

The images saved must be moved to the right folder in the data folder, which lie in root/networks/data.


The instruction-bytes file contains the instruction that correspond to each respective image.
These instructions are used as labels. The network will try to learn to map an image to an instruction-byte.

Example of what is saved in the instruction-bytes file:

	../robotic_car/images/1.png 00100010 01111111 00000000 