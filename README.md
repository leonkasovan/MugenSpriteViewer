![image](https://github.com/user-attachments/assets/15391a55-fb70-43fe-adc3-87f088a0ff04)

# Mugen Sprite Viewer

### Support for:
1. SFFv1 with PCX Compression
2. SFFv2 with RLE8,RLE5,LZ5,PNG Compression

### Feature:
1. Auto animation
2. Browse and zoom with mouse scroll
3. Load additional palette (*.act)
4. Auto resize image preview

### Install
Download from here:  
Windows 7/10/11 : https://github.com/leonkasovan/MugenSpriteViewer/releases/download/v1.0/MugenSpriteViewer-win64-v1.0.zip  
Linux x86_64 : https://github.com/leonkasovan/MugenSpriteViewer/releases/download/v1.0/MugenSpriteViewer-linux-x86_64-v1.0.zip  
Extract and run it from command line.  
Or register it in Windows to handle SFF file.  

### Usage:
```
# MugenSpriteViewer.exe kfmZ.sff
```

### Best usage:
![open_with](https://github.com/user-attachments/assets/8592d06d-8931-478a-8afb-167b82e8c7f3)

By registering in windows explorer (Open with ...) to handle SFF file.  
After registering, just double click any SFF file to view it.  

### Build from source:
```
git clone https://github.com/leonkasovan/MugenSpriteViewer.git
make
```

### Todo:
1. Extra tools support via dynamic library loading.
2. New tool_sff_info  : generate SFF file information.
3. New tool_export_pngs : export SFF as PNG images.
4. New tool_export_atlas : export SFF as sprite atlas (sprite sheet).
6. New tool_load_animation : load animation definition (AIR format).
6. New tool_optimize_sff : optimitize SFF.

https://github.com/user-attachments/assets/f2283a08-4585-4c3e-a514-def683f36dcf


