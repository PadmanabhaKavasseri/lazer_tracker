#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>

// Function declarations (prototypes)
static GstFlowReturn on_new_metadata_sample(GstElement *sink, gpointer user_data);
void process_metadata(char *metadata_text, size_t size);

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstElement *metadata_sink;
    GstBus *bus;
    
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    // Create pipeline from your command line string
    pipeline = gst_parse_launch(
        "qtimlvconverter name=preproc "
        "qtimltflite name=inference delegate=external external-delegate-path=libQnnTFLiteDelegate.so external-delegate-options=\"QNNExternalDelegate,backend_type=htp;\" model=/home/ubuntu/TFLite/yolov5m-320x320-int8.tflite "
        "qtimlpostprocess name=postproc results=5 module=yolov5 labels=/home/ubuntu/TFLite/yolov8.json settings=\"{\\\"confidence\\\": 70.0}\" "
        "filesrc location=/home/ubuntu/testVideos/Draw_720p_180s_30FPS.mp4 ! qtdemux ! queue ! h264parse ! v4l2h264dec capture-io-mode=4 output-io-mode=4 ! video/x-raw,format=NV12 ! queue ! tee name=split "
        "split. ! qtimetamux name=metamux ! tee name=meta_tee "
        "meta_tee. ! queue ! qtivoverlay ! autovideosink "
        "meta_tee. ! queue ! qtimlmetaextractor ! appsink name=metadata_sink emit-signals=true sync=false "
        "split. ! queue ! preproc. preproc. ! queue ! inference. inference. ! queue ! postproc. postproc. ! text/x-raw ! queue ! metamux.",
        NULL);
    
    if (!pipeline) {
        g_printerr("Failed to create pipeline\n");
        return -1;
    }
    
    // Find the appsink by name
    metadata_sink = gst_bin_get_by_name(GST_BIN(pipeline), "metadata_sink");
    if (!metadata_sink) {
        g_printerr("Failed to find metadata_sink element\n");
        return -1;
    }
    
    // Connect callback to receive metadata
    g_signal_connect(metadata_sink, "new-sample", G_CALLBACK(on_new_metadata_sample), NULL);
    
    // Start pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    // Run main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    
    // Cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
    
    return 0;
}

// Callback function to handle metadata
static GstFlowReturn on_new_metadata_sample(GstElement *sink, gpointer user_data) {
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    
    // Pull the sample from appsink
    sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (sample) {
        buffer = gst_sample_get_buffer(sample);
        
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            // Process the metadata text
            printf("Received metadata: %.*s\n", (int)map.size, (char*)map.data);
            
            // Your metadata processing code here
            process_metadata((char*)map.data, map.size);
            
            gst_buffer_unmap(buffer, &map);
        }
        
        gst_sample_unref(sample);
    }
    
    return GST_FLOW_OK;
}

void process_metadata(char *metadata_text, size_t size) {
    // Parse and process your metadata here
    // Could be JSON parsing, bounding box extraction, etc.
    printf("Processing metadata of size %zu\n", size);
    
    // Example: Simple text processing
    if (size > 0) {
        // Add null terminator for safe string operations
        char *null_terminated = g_strndup(metadata_text, size);
        printf("Metadata content: %s\n", null_terminated);
        g_free(null_terminated);
    }
}
