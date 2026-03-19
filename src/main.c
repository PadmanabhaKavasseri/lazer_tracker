#include "metadata_parser.h"
#include "motor_commands.h"
#include <gst/app/gstappsink.h>
#include <stdio.h>
#include <sys/time.h>



typedef struct {
    GstClockTime last_buffer_pts;
    const char* stage_name;
} TimingData;

// Function declarations
static GstFlowReturn on_new_metadata_sample(GstElement *sink, gpointer user_data);
void process_metadata(char *metadata_text, size_t size);

static GstPadProbeReturn timing_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    TimingData *timing = (TimingData*)user_data;
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    
    // Use buffer PTS to track the same buffer through pipeline
    GstClockTime pts = GST_BUFFER_PTS(buffer);
    
    if (GST_CLOCK_TIME_IS_VALID(pts) && timing->last_buffer_pts != GST_CLOCK_TIME_NONE) {
        // Calculate time since this buffer was at the previous stage
        struct timeval now;
        gettimeofday(&now, NULL);
        static struct timeval stage_times[4] = {0}; // decode, preproc, inference, postproc
        static int stage_index = 0;
        
        if (strcmp(timing->stage_name, "DECODE") == 0) stage_index = 0;
        else if (strcmp(timing->stage_name, "PREPROC") == 0) stage_index = 1;
        else if (strcmp(timing->stage_name, "INFERENCE") == 0) stage_index = 2;
        else if (strcmp(timing->stage_name, "POSTPROC") == 0) stage_index = 3;
        
        if (stage_index > 0 && stage_times[stage_index-1].tv_sec != 0) {
            long diff_ms = (now.tv_sec - stage_times[stage_index-1].tv_sec) * 1000 + 
                          (now.tv_usec - stage_times[stage_index-1].tv_usec) / 1000;
            printf("[TIMING] %s processing: %ld ms\n", timing->stage_name, diff_ms);
        }
        
        stage_times[stage_index] = now;
    }
    
    timing->last_buffer_pts = pts;
    return GST_PAD_PROBE_OK;
}

// Add to your main() after pipeline creation:
void add_timing_probes(GstElement *pipeline) {
    GstElement *preproc = gst_bin_get_by_name(GST_BIN(pipeline), "preproc");
    GstElement *inference = gst_bin_get_by_name(GST_BIN(pipeline), "inference");
    GstElement *postproc = gst_bin_get_by_name(GST_BIN(pipeline), "postproc");
    
    // Create timing data structures
    static TimingData decode_timing = {.stage_name = "DECODE"};
    static TimingData preproc_timing = {.stage_name = "PREPROC"};
    static TimingData inference_timing = {.stage_name = "INFERENCE"};
    static TimingData postproc_timing = {.stage_name = "POSTPROC"};
    
    // Add probes
    GstPad *preproc_sink = gst_element_get_static_pad(preproc, "sink");
    GstPad *preproc_src = gst_element_get_static_pad(preproc, "src");
    GstPad *inference_src = gst_element_get_static_pad(inference, "src");
    GstPad *postproc_src = gst_element_get_static_pad(postproc, "src");
    
    gst_pad_add_probe(preproc_sink, GST_PAD_PROBE_TYPE_BUFFER, timing_probe_callback, &decode_timing, NULL);
    gst_pad_add_probe(preproc_src, GST_PAD_PROBE_TYPE_BUFFER, timing_probe_callback, &preproc_timing, NULL);
    gst_pad_add_probe(inference_src, GST_PAD_PROBE_TYPE_BUFFER, timing_probe_callback, &inference_timing, NULL);
    gst_pad_add_probe(postproc_src, GST_PAD_PROBE_TYPE_BUFFER, timing_probe_callback, &postproc_timing, NULL);
}


int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstElement *CSI_pipeline;
    GstElement *target_pipe;
    GstElement *metadata_sink;
    GMainLoop *loop;

    if (init_arduino_serial() != 0) {
        return -1;
    }
            
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    
    // Create pipeline

    target_pipe = gst_parse_launch(
        "qtimlvconverter name=preproc "
        "qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options=\"QNNExternalDelegate,backend_type=htp;\" model=/home/ubuntu/TFLite/laser2.lite "
        "qtimlpostprocess name=postproc results=1 module=yolov5 labels=/home/ubuntu/TFLite/laser.json settings=\"{\\\"confidence\\\": 90.0}\" "
        "qtiqmmfsrc camera=0 ! video/x-raw,format=NV12 ! qtivtransform flip-vertical=true flip-horizontal=true ! videoconvert ! videobalance brightness=-1.0 saturation=1.9 contrast=2.0 ! videoconvert ! queue ! tee name=split "
        "split. ! qtimetamux name=metamux ! tee name=meta_tee "
        "meta_tee. ! queue ! qtivoverlay ! autovideosink "
        "meta_tee. ! queue ! qtimlmetaextractor ! appsink name=metadata_sink emit-signals=true sync=false "
        "split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.",
        NULL);


    CSI_pipeline = gst_parse_launch(
        "qtimlvconverter name=preproc "
        "qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options=\"QNNExternalDelegate,backend_type=htp;\" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite "
        "qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings=\"{\\\"confidence\\\": 70.0}\" "
        "qtiqmmfsrc camera=0 ! video/x-raw,format=NV12  ! qtivtransform flip-vertical=true ! queue ! tee name=split "
        "split. ! qtimetamux name=metamux ! tee name=meta_tee "
        "meta_tee. ! queue ! qtivoverlay ! autovideosink "
        "meta_tee. ! queue ! qtimlmetaextractor ! appsink name=metadata_sink emit-signals=true sync=false "
        "split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.",
        NULL);

    pipeline = gst_parse_launch(
        "qtimlvconverter name=preproc "
        "qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options=\"QNNExternalDelegate,backend_type=htp;\" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite "
        "qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings=\"{\\\"confidence\\\": 70.0}\" "
        "v4l2src device=/dev/video0 ! video/x-raw,format=NV12,width=1280,height=720,framerate=30/1 ! queue ! tee name=split "
        "split. ! qtimetamux name=metamux ! tee name=meta_tee "
        "meta_tee. ! queue ! qtivoverlay ! autovideosink "
        "meta_tee. ! queue ! qtimlmetaextractor ! appsink name=metadata_sink emit-signals=true sync=false "
        "split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.",
        NULL);

    
    if (!target_pipe) {
        g_printerr("Failed to create pipeline\n");
        return -1;
    }
    
    // Find and configure appsink
    metadata_sink = gst_bin_get_by_name(GST_BIN(target_pipe), "metadata_sink");
    if (!metadata_sink) {
        g_printerr("Failed to find metadata_sink element\n");
        return -1;
    }
    
    g_signal_connect(metadata_sink, "new-sample", G_CALLBACK(on_new_metadata_sample), NULL);

    add_timing_probes(target_pipe);
    
    // Start pipeline
    gst_element_set_state(target_pipe, GST_STATE_PLAYING);
    
    // Run main loop
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    
    // Cleanup
    gst_element_set_state(target_pipe, GST_STATE_NULL);
    gst_object_unref(target_pipe);
    g_main_loop_unref(loop);
    cleanup_arduino_serial();
    
    return 0;
}

static GstFlowReturn on_new_metadata_sample(GstElement *sink, G_GNUC_UNUSED gpointer user_data) {
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    
    sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (sample) {
        buffer = gst_sample_get_buffer(sample);
        
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            process_metadata((char*)map.data, map.size);
            gst_buffer_unmap(buffer, &map);
        }
        
        gst_sample_unref(sample);
    }
    
    return GST_FLOW_OK;
}

void process_metadata(char *metadata_text, size_t size) {
    if (size > 0) {
        char *null_terminated = g_strndup(metadata_text, size);
        
        DetectionResult *result = parse_detection_metadata(null_terminated);
        if (result) {
            print_detection_result(result);
            
            // Your custom processing here
            for (guint i = 0; i < result->num_detections; i++) {
                ObjectDetection *det = &result->detections[i];
                
                // Filter for stop signs with high confidence
                if (is_high_confidence_detection(det, 90.0) && 
                    strcmp(det->class_name, "target") == 0) {
                    
                    // Calculate center coordinates
                    float center_x = det->x + (det->width / 2.0);
                    float center_y = det->y + (det->height / 2.0);
                    
                    // COMPENSATE FOR BOTH FLIPS
                    // center_x = 1.0 - center_x;  // Since coordinates are normalized 0.0-1.0
                    center_y = 1.0 - center_y;

                    // Map to servo angles using custom functions
                    int pan_angle = map_to_pan_angle(center_x) + LASER_PAN_OFFSET;
                    int tilt_angle = map_to_tilt_angle(center_y) + LASER_TILT_OFFSET;

                    // Clamp final values to absolute bounds
                    // if (pan_angle > LASER_PAN_LEFT) pan_angle = LASER_PAN_LEFT;
                    // if (pan_angle < LASER_PAN_RIGHT) pan_angle = LASER_PAN_RIGHT;
                    // if (tilt_angle > LASER_TILT_TOP) tilt_angle = LASER_TILT_TOP;
                    // if (tilt_angle < LASER_TILT_BOTTOM) tilt_angle = LASER_TILT_BOTTOM;

                    apply_calibration_correction(center_x, center_y, &pan_angle, &tilt_angle);


                    
                    // Print what would be sent to Arduino
                    printf("SERVO COMMAND: %d,%d\n", pan_angle, tilt_angle);
                    send_arduino_command_binary(pan_angle, tilt_angle);
                }
            }
            free_detection_result(result);
        }
        g_free(null_terminated);
    }
}



