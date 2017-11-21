# Program to flip images vertically.
# If an obstacle is located on the left, the image can be flipped to be used for an obstacle to the right
# The program assumes that the images are located in the folder data.
# They are saved to the folder temp located in data

from PIL import Image
import os

files = os.listdir('data/')
for f in files:
    if f.endswith('.png'):
        i = Image.open(os.path.join('data/',f))
        fn, fext = os.path.splitext(f)
        i.transpose(Image.FLIP_LEFT_RIGHT).save('data/temp/0{}.png'.format(fn))
