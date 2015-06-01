#!/usr/bin/env sh
# This script converts the mnist data into lmdb/leveldb format,
# depending on the value assigned to $BACKEND.

EXAMPLE=/home/nelson/TCC/caffe_model/
DATA=/home/nelson/TCC/caffe_model/
BUILD=/home/nelson/TCC/caffe_model/bin

BACKEND="lmdb"

echo "Creating ${BACKEND}..."

rm -rf $EXAMPLE/cnn_train_${BACKEND}
rm -rf $EXAMPLE/cnn_test_${BACKEND}

$BUILD/convert_data $DATA/train/train_data_ubyte \
  $DATA/train/train_label_ubyte $EXAMPLE/cnn_train_${BACKEND} --backend=${BACKEND}

$BUILD/convert_data $DATA/test/test_data_ubyte \
  $DATA/test/test_label_ubyte $EXAMPLE/cnn_test_${BACKEND} --backend=${BACKEND}

echo "Done."
