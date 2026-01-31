"""
Script to convert logo.png to C++ header file with raw bytes
Usage: python change_logo.py
"""

import os
from pathlib import Path
from PIL import Image

def get_image_dimensions(image_path):
    """Get width and height of the image"""
    try:
        with Image.open(image_path) as img:
            return img.width, img.height
    except Exception as e:
        print(f"Warning: Could not read image dimensions: {e}")
        return 196, 196  # Default dimensions

def read_png_as_bytes(png_path):
    """Read PNG file as raw bytes"""
    with open(png_path, 'rb') as f:
        return f.read()

def bytes_to_cpp_array(data, bytes_per_line=12):
    """Convert bytes to C++ array format"""
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        hex_values = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'    {hex_values}')
    
    return ',\n'.join(lines)

def create_logo_header(logo_bytes, width, height, output_path):
    """Create Logo.h header file"""
    
    cpp_array = bytes_to_cpp_array(logo_bytes)
    array_size = len(logo_bytes)
    
    header_content = f"""#pragma once

namespace resource
{{

inline int s_LogoWidth = {width};
inline int s_LogoHeight = {height};

inline unsigned char s_Logo[{array_size}] = {{
{cpp_array}
}};

}} // namespace resource
"""
    
    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    # Write header file
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    print(f"✓ Created {output_path}")
    print(f"  - Image size: {width}x{height}")
    print(f"  - Array size: {array_size} bytes")

def main():
    # Define paths
    script_dir = Path(__file__).parent
    logo_path = script_dir / 'logo.png'
    output_path = script_dir.parent / 'include' / '69' / 'resource' / 'Logo.h'
    
    print(f"Logo path: {logo_path}")
    print(f"Output path: {output_path}")
    
    # Check if logo.png exists
    if not logo_path.exists():
        print(f"❌ Error: {logo_path} not found!")
        print(f"   Please place logo.png in the same directory as this script.")
        return 1
    
    # Get image dimensions
    width, height = get_image_dimensions(logo_path)
    
    # Read PNG as bytes
    print(f"\nReading {logo_path}...")
    logo_bytes = read_png_as_bytes(logo_path)
    
    # Create header file
    print(f"Creating C++ header file...")
    create_logo_header(logo_bytes, width, height, output_path)
    
    print("\n✓ Done!")
    return 0

if __name__ == '__main__':
    exit(main())