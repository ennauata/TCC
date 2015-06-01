import numpy as np
import cv2
import os
from random import shuffle

# This script take the average among all frames in the videos
rootdir = '/home/nelson/TCC/caffe_model/seq_files/train'

train_data = []
label_data = []
frame_count = 0
first = True
prev_video = "none"
average_video = np.zeros((144, 180))
label = 0
for subdir, dirs, files in os.walk(rootdir):

    # Iterate over frames
    for frame in files:
        class_name = subdir.split('/')[-2]
        curr_video = subdir.split('/')[-1]

        if prev_video != curr_video:
            if first == True:
                first = False
            else:
                train_data.append(np.divide(np.array(average_video), frame_count))

            average_video = np.zeros((144, 180))
            frame_count = 0
            label_data.append(class_name)

        else:

            # Take average
            im = cv2.imread(os.path.join(subdir, frame), 0)
            average_video = np.add(average_video, im)
            frame_count = frame_count + 1

        prev_video = curr_video

# Append last training sample
train_data.append(np.divide(np.array(average_video), frame_count))
#print(np.array(train_data))

# Define parameters
rows = 144
cols = 180
magic = 2051
num_items = 73

# Shuffle elements
idx = range(num_items)
shuffle(idx)
shuffled_train_data = np.array(train_data)[idx]
shuffled_train_labels = np.array(label_data)[idx]

# Convert dataset to uint8 by truncating the elements
params = np.array([magic, num_items, rows, cols])
ubyte_train_data = np.array(shuffled_train_data).flatten()
print(ubyte_train_data.astype(np.uint8))
with open("./train/train_data_ubyte", 'wb') as f:
    f.write(params.astype(np.int32))
    f.write(ubyte_train_data.astype(np.uint8))

# Convert labels to uint8
magic = 2049
num_labels = 73
params = np.array([magic, num_labels])

classes = {"bend": 0, "jack": 1, "jump": 2, "pjump": 3, "run": 4, "side": 5, "skip": 6, "walk": 7, "wave1": 8, "wave2": 9}
ubyte_train_label = np.array([classes[l] for l in shuffled_train_labels])
print(ubyte_train_label)
with open("./train/train_label_ubyte", 'wb') as f:
    f.write(params.astype(np.int32))
    f.write(ubyte_train_label.astype(np.uint8))