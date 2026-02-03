#ifndef METADATA_PARSER_H
#define METADATA_PARSER_H

#include <glib.h>
#include <gst/gst.h>

#ifdef __cplusplus
extern "C" {
#endif

// Keypoint/Landmark structure
typedef struct {
    gchar *name;
    gdouble x;
    gdouble y;
} Keypoint;

// Bounding box detection
typedef struct {
    guint id;
    gchar *class_name;
    gdouble confidence;
    guint color;
    
    // Bounding box coordinates (normalized 0.0-1.0)
    gfloat x, y, width, height;
    
    // Optional landmarks
    Keypoint *landmarks;
    guint num_landmarks;
    
    // Optional extra parameters
    GHashTable *xtraparams;
} ObjectDetection;

// Complete detection result for one frame
typedef struct {
    ObjectDetection *detections;
    guint num_detections;
    
    // Frame metadata
    guint64 timestamp;
    guint sequence_index;
    guint sequence_num_entries;
    gint parent_id;
    
    // Optional stream info
    gchar *stream_id;
    guint64 stream_timestamp;
} DetectionResult;

// Function declarations
DetectionResult* parse_detection_metadata(const char *metadata_text);
DetectionResult* parse_gst_structure(GstStructure *structure);
void parse_detection_structure(GstStructure *det_structure, ObjectDetection *detection);
void print_detection_result(const DetectionResult *result);
void free_detection_result(DetectionResult *result);
int map_to_servo_angle(float normalized_coord, int image_dimension);

// Utility functions
void convert_to_pixel_coordinates(const ObjectDetection *detection, 
                                 guint image_width, guint image_height,
                                 guint *x1, guint *y1, guint *x2, guint *y2);
gboolean is_high_confidence_detection(const ObjectDetection *detection, gdouble threshold);

#ifdef __cplusplus
}
#endif

#endif /* METADATA_PARSER_H */
