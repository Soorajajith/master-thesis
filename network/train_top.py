# This program is run before the program fine_tune.py
# The program will train the top only with the extracted features from the convolution layers of VGG16
# It saves the model of the top, which must be used in fine_tune.py

import numpy as np
import math
import matplotlib.pyplot as plt
from keras.preprocessing.image import ImageDataGenerator
from keras.models import Sequential
from keras.layers import Dropout, Flatten, Dense
from keras import applications
from keras.utils.np_utils import to_categorical

# dimensions of the images.
img_width, img_height = 160, 120

top_model_weights_path = 'top_model_weights.h5'
train_data_dir = 'data/train'
validation_data_dir = 'data/val'
epochs = 50
batch_size = 64


def save_bottleneck_features():
    # data augmentation
    # rescale is done since images in RGB coefficients (0-255),
    # this is to high to process so rescale between 0 and 1 instead 
    datagen = ImageDataGenerator(rescale=1. / 255)

    # VGG16 network, without the top
    model = applications.VGG16(include_top=False, weights='imagenet')

    # get the training data
    generator = datagen.flow_from_directory(
        train_data_dir,
        target_size=(img_height,img_width),
        batch_size=batch_size,
        class_mode=None,
        shuffle=False)
    
    nb_train_samples = len(generator.filenames)
    
    #This is done due to bug in predict_generator not being able to determine
    #the correct number of iterations when working on batches when the number
    #of sambles is not divisible by the batch size
    size_train = int(math.ceil(nb_train_samples // batch_size))
    
    bottleneck_features_train = model.predict_generator(
        generator, size_train, verbose=1)
    
    np.save(open('bottleneck_features_train.npy', 'wb'),
            bottleneck_features_train)

    # get the validation data
    generator = datagen.flow_from_directory(
        validation_data_dir,
        target_size=(img_height,img_width),
        batch_size=batch_size,
        class_mode=None,
        shuffle=False)
    
    nb_validation_samples = len(generator.filenames)
    
    size_validation = int(math.ceil(nb_validation_samples // batch_size))
    
    bottleneck_features_validation = model.predict_generator(
        generator, size_validation, verbose=1)
    
    np.save(open('bottleneck_features_validation.npy', 'wb'),
            bottleneck_features_validation)


def train_top_model():
    
    datagen = ImageDataGenerator(rescale=1./255)
    generator = datagen.flow_from_directory(  
         train_data_dir,  
         target_size=(img_height,img_width),  
         batch_size=batch_size,  
         class_mode='categorical',  
         shuffle=False)  

    num_classes = len(generator.class_indices)
    
    train_data = np.load(open('bottleneck_features_train.npy', 'rb'))
    train_labels = generator.classes
    train_labels = to_categorical(train_labels, num_classes=num_classes)

    generator = datagen.flow_from_directory(  
         validation_data_dir,  
         target_size=(img_height,img_width),  
         batch_size=batch_size,  
         class_mode='categorical',  
         shuffle=False)
    
    validation_data = np.load(open('bottleneck_features_validation.npy', 'rb'))
    
    validation_labels = generator.classes
    validation_labels = to_categorical(validation_labels, num_classes=num_classes)

    model = Sequential()
    model.add(Flatten(input_shape=train_data.shape[1:]))
    model.add(Dense(256, activation='relu'))
    model.add(Dropout(0.5))
    model.add(Dense(256, activation='relu'))
    model.add(Dropout(0.5))
    model.add(Dense(num_classes, activation='softmax'))

    model.compile(loss='categorical_crossentropy',
                  optimizer='sgd',
                  metrics=['accuracy'])

    history = model.fit(train_data, train_labels,
              epochs=epochs,
              batch_size=batch_size,
              validation_data=(validation_data, validation_labels), 
              verbose=1)
    
    model.save_weights(top_model_weights_path)
    
    (eval_loss, eval_accuracy) = model.evaluate(
            validation_data, validation_labels, batch_size=batch_size, verbose=1)
    
    print("[INFO] accuracy: {:.2f}%".format(eval_accuracy * 100))  
    print("[INFO] Loss: {}".format(eval_loss))

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

save_bottleneck_features()
train_top_model()
