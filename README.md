# GStreamer Metadata Parser

Real-time object detection metadata parser for GStreamer pipelines with `qtimlmetaextractor`.

## Build

```bash
make
```

## Check Devices USB
```
v4l2-ctl --list-devices
```

## USB Cam to HDMI 
```
gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! autovideosink

```

## Run

```bash
./bin/metadata_processor
```


## USB Cam to object detection IMDSK

```
gst-launch-1.0 -e \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings="{\"confidence\": 70.0}" \
v4l2src device=/dev/video2 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.

```

## CSI Cam to object detection IMDSK

```
gst-launch-1.0 -e  \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite \
qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings="{\"confidence\": 70.0}" \
qtiqmmfsrc camera=0 ! video/x-raw,format=NV12 ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.

```

## Framerate Testing using fpsdisplaysink

### USB Cam
```
GST_DEBUG="identity:5" gst-launch-1.0 v4l2src device=/dev/video2 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! identity name=timing-start ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```
### CSI Cam
```
GST_DEBUG="identity:5" gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! identity name=timing-start ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```






















## Classification Pipeline
```
gst-launch-1.0 -e --gst-debug=2 filesrc location=/home/ubuntu/testVideos/Animals_000_1080p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split split. ! queue ! qtivcomposer name=mixer sink_1::dimensions="<1920,1080>" ! queue ! autovideosink \
split. ! queue ! qtimlvconverter ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite ! queue ! qtimlpostprocess results=1 module=mobilenet labels=/home/ubuntu/TFLite/mobilenet.labels settings="{\"confidence\": 51.0}" ! video/x-raw,format=BGRA,width=640,height=360 ! queue ! mixer.

```


## USB Cam Mobilnet pipeline IMSDK
```
gst-launch-1.0 -e \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite \
qtimlpostprocess name=postproc results=5 module=mobilenet labels=/home/ubuntu/TFLite/mobilenet.labels settings="{\"confidence\": 51.0}" \
v4l2src device=/dev/video2 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.


```


## Mobilnet with metamux
```

gst-launch-1.0 -e --gst-debug=2 \
qtiqmmfsrc camera=0 name=camsrc ! video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1 ! queue ! tee name=split \
split. ! queue ! qtimetamux name=metamux ! queue ! qtivoverlay ! queue ! autovideosink \
split. ! queue ! qtimlvconverter ! queue ! qtimltflite delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite ! queue ! qtimlvclassification threshold=60.0 results=3 module=mobilenet labels=/home/ubuntu/mobilenet.labels  ! text/x-raw ! queue ! metamux.



```

model=/home/ubuntu/TFLite/mobilenet_v2_1.0_224_quant.tflite



qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite