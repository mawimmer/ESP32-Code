import shutil
import os

# Pfade von deinem Repo aus gesehen
src_dir = "src" 
dst_dir = os.path.join("..", "WLED", "usermods", "multiple_rotary_encoder")

def sync():
    abs_src = os.path.abspath(src_dir)
    abs_dst = os.path.abspath(dst_dir)
    
    if os.path.exists(abs_dst):
        shutil.rmtree(abs_dst)
    
    # Hier einfach weitere Ordner/Dateien in die Liste werfen
    ignore = shutil.ignore_patterns('wled_mock', 'wled_mock.h', 'main.cpp')
    
    shutil.copytree(abs_src, abs_dst, ignore=ignore)
    print(f"Done! {abs_src} -> {abs_dst}")

if __name__ == "__main__":
    sync()