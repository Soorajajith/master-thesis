# The program will fine-tune the convolutional layers in the VGG16 network, right now the first 11 layers are frozen.
# The top is also fine-tuned. The model from train_top.py will be needed.

from keras import applications
from keras.preprocessing.image import ImageDataGenerator
from keras import optimizers
from keras.models import Sequential, Model
from keras.layers import Dropout, Flatten, Dense
from keras.callbacks import LearningRateScheduler, Callback
from keras.preprocessing import image
import numpy as np
import math
import matplotlib.pyplot as plt
import keras.backend as k

# path to the model weights files.
top_model_weights_path = 'top_model_weights.h5'
# dimensions of the images.
img_width, img_height = 160, 120

train_data_dir = 'data/train'
validation_data_dir = 'data/val'
epochs = 50
batch_size = 64


# VGG16 network, without the top
base_model = applications.VGG16(weights='imagenet', include_top=False, input_shape=(img_height,img_width,3))

#new model to put on top of the convolutional model
top_model = Sequential()
top_model.add(Flatten(input_shape=base_model.output_shape[1:]))
top_model.add(Dense(256, activation='relu'))
top_model.add(Dropout(0.5))
top_model.add(Dense(256, activation='relu'))
top_model.add(Dropout(0.5))
top_model.add(Dense(3, activation='softmax'))

#load the weights learned from previous training
top_model.load_weights(top_model_weights_path)

# add the model on top of the convolutional model
model = Model(inputs=base_model.input, outputs=top_model(base_model.output))

# set the first 11 layers (up to the last conv block)
# to non-trainable
for layer in model.layers[:11]:
    layer.trainable = False

# compile the model with a SGD optimizer.
# use lr=0.0001 if the same learning rate should be used through out the
# training   
model.compile(loss='categorical_crossentropy',
              optimizer=optimizers.SGD(lr=0.0001, momentum=0.9),
              metrics=['accuracy'])

# data augmentation
# rescale is done since images in RGB coefficients (0-255),
# this is to high to process so rescale between 0 and 1 instead 
train_datagen = ImageDataGenerator(rescale=1. / 255)#, shear_range=0.2)

test_datagen = ImageDataGenerator(rescale=1. / 255)#, shear_range=0.2)

train_generator = train_datagen.flow_from_directory(
    train_data_dir,
    target_size=(img_height, img_width),
    batch_size=batch_size,
    class_mode='categorical')

nb_train_samples = len(train_generator.filenames)
size_train = int(math.ceil(nb_train_samples // batch_size))


validation_generator = test_datagen.flow_from_directory(
    validation_data_dir,
    target_size=(img_height, img_width),
    batch_size=batch_size,
    class_mode='categorical')

nb_validation_samples = len(validation_generator.filenames)
size_validation = int(math.ceil(nb_validation_samples // batch_size))

model.summary()

#%%
# Fine-tune
history = model.fit_generator(
        train_generator,
        steps_per_epoch=size_train,
        epochs=epochs,
        validation_data=validation_generator,
        validation_steps=size_validation,
        verbose=1)

#%%
# Test how well it did
scores = model.evaluate_generator(validation_generator, size_validation)
print('Scores (generator):', scores)
print('Val loss (generator):', scores[0])
print('Val Accuracy (generator):', scores[1])



#%%

# saves  the whole model with weights and optimizer
model.save('fine_tuned_model.h5', )

#saves only the model's architecture
model_json = model.to_json()
with open('fine_tuned_arch.json', 'w') as json_file:
    json_file.write(model_json)

#saves only weights
model.save_weights('fine_tuned_weights.h5')


#%%
plt.figure(1)
     
# summarize history for accuracy  
plt.subplot(211)  
plt.plot(history.history['acc'])  
plt.plot(history.history['val_acc'])  
plt.title('model accuracy')  
plt.ylabel('accuracy')  
plt.xlabel('epoch')  
plt.legend(['train', 'val'], loc='upper left') 
    
# summarize history for loss  
plt.subplot(212)  
plt.plot(history.history['loss'])  
plt.plot(history.history['val_loss'])  
plt.title('model loss')  
plt.ylabel('loss')  
plt.xlabel('epoch')  
plt.legend(['train', 'val'], loc='upper left')  
plt.show()  
