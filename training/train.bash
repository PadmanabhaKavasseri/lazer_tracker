#!/bin/bash
mkdir -p training_images
count=0

echo "=== QTI Camera Image Capture ==="
echo "Press ENTER to capture image"
echo "Type 'q' + ENTER to quit"
echo ""

while true; do
    printf "Ready to capture image #%d (ENTER/q): " $count
    read input
    
    if [ "$input" = "q" ]; then
        break
    fi
    
    echo "Capturing..."
    
    # Define target filename
    target_file="training_images/img_$(printf "%04d" $count).png"
    
    # Capture using filesink with exact filename
    timeout 3s gst-launch-1.0 qtiqmmfsrc camera=0 ! \
        "video/x-raw,format=NV12,width=1280,height=720,framerate=30/1" ! \
        qtivtransform flip-vertical=true flip-horizontal=true ! \
        videoconvert ! \
        videobalance brightness=0.0 saturation=2.0 contrast=2.0 ! \
        pngenc ! \
        filesink location="$target_file" 2>/dev/null
    
    # Check if file was created and has content
    if [ -f "$target_file" ] && [ -s "$target_file" ]; then
        size=$(stat -c%s "$target_file" 2>/dev/null || echo "0")
        echo "✓ Saved: $target_file ($size bytes)"
        ((count++))
    else
        echo "✗ Capture failed - check camera"
        # Clean up any empty file
        rm -f "$target_file" 2>/dev/null
    fi
    echo ""
done

echo "=== Capture Complete ==="
echo "Total images captured: $count"
echo ""
echo "Files created:"
ls -la training_images/
