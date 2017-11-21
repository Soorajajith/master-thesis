# Program for moving the images to the right folder.
# The file needs to be modified if it is going to be used.

import shutil

# One folder for each movement instruction
dest1 = 'data/001000100000000001111111/'
dest2 = 'data/001000100000000010000000/'
dest3 = 'data/001000100111111100000000/'
dest4 = 'data/001000100111111101100000/'
dest5 = 'data/001000100111111110100000/'

# The folder where the saved images taken with the robotic car are placed
folder = '' 

# Open the instruction bytes file. Ex: data/instruction_bytes.txt
with open('') as f:
    for line in f:
        imgPath,inst = line.split(' ',1)
        x,img = imgPath.rsplit('/',1)
        src = folder + img
        
        if '00100010 00000000 01111111' in inst:
            shutil.move(src, dest1)
        elif '00100010 00000000 10000000' in inst:
            shutil.move(src, dest2)
        elif '00100010 01111111 00000000' in inst:
            shutil.move(src, dest3)
        elif '00100010 01111111 01100000' in inst:
            shutil.move(src, dest4)
        elif '00100010 01111111 10100000' in inst:
            shutil.move(src, dest5)
