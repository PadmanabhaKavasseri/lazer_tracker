#include "metadata_parser.h"
#include <stdio.h>
#include <string.h>

// Main entry point - simplified for known format
DetectionResult* parse_detection_metadata(const char *metadata_text) {
    GValue value = G_VALUE_INIT;
    DetectionResult *result = NULL;
    
    // We know it's always GST_TYPE_LIST from our pipeline
    g_value_init(&value, GST_TYPE_LIST);
    if (gst_value_deserialize(&value, metadata_text)) {
        // Get the first (and only) element which contains the ObjectDetection structure
        const GValue *first_element = gst_value_list_get_value(&value, 0);
        if (GST_VALUE_HOLDS_STRUCTURE(first_element)) {
            GstStructure *structure = (GstStructure*)g_value_get_boxed(first_element);
            result = parse_gst_structure(structure);
        }
    }
    g_value_unset(&value);
    
    return result;
}

// Parse the main ObjectDetection structure
DetectionResult* parse_gst_structure(GstStructure *structure) {
    if (!structure) {
        return NULL;
    }
    
    DetectionResult *result = g_malloc0(sizeof(DetectionResult));
    
    // Extract frame metadata
    gst_structure_get_uint64(structure, "timestamp", &result->timestamp);
    gst_structure_get_uint(structure, "sequence-index", &result->sequence_index);
    gst_structure_get_uint(structure, "sequence-num-entries", &result->sequence_num_entries);
    gst_structure_get_int(structure, "parent-id", &result->parent_id);
    
    // Extract stream info if present
    const gchar *stream_id = gst_structure_get_string(structure, "stream-id");
    if (stream_id) {
        result->stream_id = g_strdup(stream_id);
    }
    gst_structure_get_uint64(structure, "stream-timestamp", &result->stream_timestamp);
    
    // Extract bounding boxes - we know it's always GstValueArray
    const GValue *bboxes_value = gst_structure_get_value(structure, "bounding-boxes");
    if (bboxes_value && GST_VALUE_HOLDS_ARRAY(bboxes_value)) {
        guint array_size = gst_value_array_get_size(bboxes_value);
        result->num_detections = array_size;
        
        if (result->num_detections > 0) {
            result->detections = g_malloc0(sizeof(ObjectDetection) * result->num_detections);
            
            for (guint i = 0; i < result->num_detections; i++) {
                const GValue *detection_value = gst_value_array_get_value(bboxes_value, i);
                
                // We know detections are always GstStructure (not strings)
                if (GST_VALUE_HOLDS_STRUCTURE(detection_value)) {
                    GstStructure *det_struct = (GstStructure*)g_value_get_boxed(detection_value);
                    parse_detection_structure(det_struct, &result->detections[i]);
                }
            }
        }
    }
    // If bounding-boxes is empty or missing, num_detections remains 0
    
    return result;
}

// Parse detection from GstStructure
void parse_detection_structure(GstStructure *det_structure, ObjectDetection *detection) {
    if (!det_structure) {
        return;
    }
    
    // Initialize detection structure
    memset(detection, 0, sizeof(ObjectDetection));
    
    // Extract basic fields
    detection->class_name = g_strdup(gst_structure_get_name(det_structure));
    gst_structure_get_uint(det_structure, "id", &detection->id);
    gst_structure_get_double(det_structure, "confidence", &detection->confidence);
    gst_structure_get_uint(det_structure, "color", &detection->color);
    
    // Extract rectangle coordinates - we know it's always GstValueArray of floats
    const GValue *rectangle_value = gst_structure_get_value(det_structure, "rectangle");
    if (rectangle_value && GST_VALUE_HOLDS_ARRAY(rectangle_value)) {
        guint rect_size = gst_value_array_get_size(rectangle_value);
        if (rect_size >= 4) {
            // We know these are always floats
            detection->x = g_value_get_float(gst_value_array_get_value(rectangle_value, 0));
            detection->y = g_value_get_float(gst_value_array_get_value(rectangle_value, 1));
            detection->width = g_value_get_float(gst_value_array_get_value(rectangle_value, 2));
            detection->height = g_value_get_float(gst_value_array_get_value(rectangle_value, 3));
        }
    }
    
    // Extract landmarks if present (optional feature)
    const GValue *landmarks_value = gst_structure_get_value(det_structure, "landmarks");
    if (landmarks_value && GST_VALUE_HOLDS_ARRAY(landmarks_value)) {
        guint landmarks_size = gst_value_array_get_size(landmarks_value);
        if (landmarks_size > 0) {
            detection->num_landmarks = landmarks_size;
            detection->landmarks = g_malloc0(sizeof(Keypoint) * detection->num_landmarks);
            
            for (guint i = 0; i < detection->num_landmarks; i++) {
                const GValue *landmark_value = gst_value_array_get_value(landmarks_value, i);
                if (GST_VALUE_HOLDS_STRUCTURE(landmark_value)) {
                    GstStructure *landmark_struct = (GstStructure*)g_value_get_boxed(landmark_value);
                    if (landmark_struct) {
                        detection->landmarks[i].name = g_strdup(gst_structure_get_name(landmark_struct));
                        gst_structure_get_double(landmark_struct, "x", &detection->landmarks[i].x);
                        gst_structure_get_double(landmark_struct, "y", &detection->landmarks[i].y);
                    }
                }
            }
        }
    }
}

// Print detection results
void print_detection_result(const DetectionResult *result) {
    if (!result) return;
    
    printf("\n=== Detection Result ===\n");
    printf("Timestamp: %lu\n", result->timestamp);
    printf("Sequence: %u/%u\n", result->sequence_index, result->sequence_num_entries);
    printf("Detections: %u\n", result->num_detections);
    
    if (result->num_detections == 0) {
        printf("  No objects detected in this frame\n");
    } else {
        for (guint i = 0; i < result->num_detections; i++) {
            const ObjectDetection *det = &result->detections[i];
            printf("  [%u] %s: %.1f%% at (%.3f,%.3f,%.3f,%.3f)\n",
                   det->id, det->class_name ? det->class_name : "unknown", 
                   det->confidence, det->x, det->y, det->width, det->height);
            
            for (guint j = 0; j < det->num_landmarks; j++) {
                printf("    Landmark %s: (%.3f, %.3f)\n",
                       det->landmarks[j].name, det->landmarks[j].x, det->landmarks[j].y);
            }
        }
    }
    printf("========================\n\n");
}

// Free allocated memory
void free_detection_result(DetectionResult *result) {
    if (!result) return;
    
    for (guint i = 0; i < result->num_detections; i++) {
        g_free(result->detections[i].class_name);
        for (guint j = 0; j < result->detections[i].num_landmarks; j++) {
            g_free(result->detections[i].landmarks[j].name);
        }
        g_free(result->detections[i].landmarks);
        if (result->detections[i].xtraparams) {
            g_hash_table_destroy(result->detections[i].xtraparams);
        }
    }
    g_free(result->detections);
    g_free(result->stream_id);
    g_free(result);
}

// Utility: Convert normalized coordinates to pixel coordinates
void convert_to_pixel_coordinates(const ObjectDetection *detection, 
                                 guint image_width, guint image_height,
                                 guint *x1, guint *y1, guint *x2, guint *y2) {
    *x1 = (guint)(detection->x * image_width);
    *y1 = (guint)(detection->y * image_height);
    *x2 = (guint)((detection->x + detection->width) * image_width);
    *y2 = (guint)((detection->y + detection->height) * image_height);
}

// Utility: Check if detection meets confidence threshold
gboolean is_high_confidence_detection(const ObjectDetection *detection, gdouble threshold) {
    return detection->confidence >= threshold;
}
