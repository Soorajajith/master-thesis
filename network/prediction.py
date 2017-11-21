# Program for running the prediction with the robotic car

from keras.models import load_model
from keras.preprocessing import image
import numpy as np
import socket
from PIL import Image
import os
import sys

model = load_model(os.path.dirname(sys.executable) + '\model.h5')

def Main():
    host = '127.0.0.1'
    port = 8889
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((host, port))
    while True:
        data, addr = s.recvfrom(57601)
        result = predictImage(data)
        s.sendto(result, addr)
        
    s.close()

def predictImage(var):

    img = Image.frombytes('RGB', (160, 120), var, "raw", 'BGR', 0, 1)
    
    x = image.img_to_array(img)
    x = np.expand_dims(x, axis=0)

    images = np.vstack([x])
    pred = model.predict(images, batch_size=64)
    return np.argmax(pred[0]).tobytes()
    	
if __name__ == "__main__":
	Main()
    
