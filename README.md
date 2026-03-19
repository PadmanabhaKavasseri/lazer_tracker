# GStreamer Metadata Parser

Real-time object detection metadata parser for GStreamer pipelines with `qtimlmetaextractor`.

## Build GST Appsink

```bash
make
```

## Compile Arduino Code
```
cd /home/ubuntu/lazer_tracker/servo_controller
arduino-cli compile --fqbn arduino:avr:uno servo_controller.ino
```

## Flash Arduino Code
```
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno servo_controller.ino

```

## Arduino Test Commands
```

```


## Check Devices USB
```
v4l2-ctl --list-devices
```

## USB Cam to HDMI 
```
gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! autovideosink

```

## CSI Cam to HDMI 
```
gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! autovideosink

```

## CSI Cam ROTATE  to HDMI 
```
gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! autovideosink

```

--no magenta--
gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! videoconvert ! videobalance saturation=3.0 contrast=1.5 ! autovideosink

--can see--
gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! videoconvert ! videobalance saturation=1.2 contrast=1.1 ! autovideosink

--cant see---
gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! videoconvert ! videobalance saturation=2.0 contrast=1.3 hue=0.8 ! autovideosink

gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! videoconvert ! videobalance saturation=-2.0 contrast=0 hue=0.8 ! autovideosink






--works well--
gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! videoconvert ! videobalance brightness=-1 saturation=2.0 contrast=1.4 ! autovideosink



gst-launch-1.0 qtiqmmfsrc camera=0 ! "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! qtivtransform flip-vertical=true ! videoconvert ! videobalance brightness=-1.0 saturation=1.9 contrast=2.0 ! waylandsink fullscreen=true



export XDG_RUNTIME_DIR=/dev/socket/weston && mkdir -p $XDG_RUNTIME_DIR && weston --continue-without-input --idle-time=0 --backend=drm-backend.so --xwayland &





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

## CSI Cam to object detection IMDSK Rotate upside down

```
gst-launch-1.0 -e  \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/yolov5s-320x320-int8.tflite \
qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings="{\"confidence\": 70.0}" \
qtiqmmfsrc camera=0 ! video/x-raw,format=NV12  ! qtivtransform flip-vertical=true ! queue ! tee name=split \
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

## CSI Cam to EI object detection IMDSK Rotate upside down

```
gst-launch-1.0 -e  \
qtimlvconverter name=preproc \
qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options="QNNExternalDelegate,backend_type=htp;" model=/home/ubuntu/TFLite/laser1.lite \
qtimlpostprocess name=postproc results=1 module=yolov5 labels=/home/ubuntu/TFLite/laser.json settings="{\"confidence\": 80.0}" \
qtiqmmfsrc camera=0 ! video/x-raw,format=NV12  ! qtivtransform flip-vertical=true ! queue ! tee name=split \
split. ! qtimetamux name=metamux ! qtivoverlay ! autovideosink \
split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.

```


### Training Pipeline
```
rm -rf training_images && mkdir training_images


gst-launch-1.0 qtiqmmfsrc camera=0 ! \
    "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! \
    qtivtransform flip-vertical=true flip-horizontal=true ! \
    videoconvert ! \
    videobalance brightness=-1.0 saturation=1.0 contrast=1.5 ! \
    videorate ! \
    "video/x-raw,framerate=1/1" ! \
    pngenc ! \
    multifilesink location="training_images/img_%04d.png"



```



## 
When the target is at the top left:
(0.135937,0.133333,0.126563,0.168750) was SERVO COMMAND: 115,119
the servo command it should send out is (Pan: 121, Tilt: 123)

When the target is top right:
(0.685938,0.133333,0.126563,0.168750) was SERVO COMMAND: 80,119
it should send out (Pan: 70, Tilt: 123)

When the target is bottom left:
(0.154687,0.475000,0.126563,0.168750) was SERVO COMMAND: 114,105
it should send out (Pan: 120, Tilt: 100)

When the target is bottom right:
(0.721875,0.475000,0.126563,0.168750) was SERVO COMMAND: 77,105
it should send out (Pan: 67, Tilt: 98)