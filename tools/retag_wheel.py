import os
import sys
import glob
import zipfile
import hashlib
import base64
def get_sha256_and_size(data):
    sha = hashlib.sha256(data).digest()
    sha_str = base64.urlsafe_b64encode(sha).decode('utf-8').rstrip('=')
    return f"sha256={sha_str}", len(data)
def retag_wheel(wheel_path, old_tag, new_tag):
    new_path = wheel_path.replace(old_tag, new_tag)
    temp_path = wheel_path + ".tmp"
    
    with zipfile.ZipFile(wheel_path, 'r') as zin:
        with zipfile.ZipFile(temp_path, 'w', compression=zipfile.ZIP_DEFLATED) as zout:
            # 1. Read all files first
            files = {}
            for name in zin.namelist():
                files[name] = zin.read(name)
            
            # 2. Find WHEEL and RECORD paths
            wheel_file_name = [n for n in files if n.endswith(".dist-info/WHEEL")][0]
            record_file_name = [n for n in files if n.endswith(".dist-info/RECORD")][0]
            
            # 3. Update WHEEL content
            wheel_data = files[wheel_file_name]
            lines = wheel_data.decode('utf-8').splitlines()
            new_lines = []
            for line in lines:
                if line.startswith("Tag:"):
                    new_lines.append(line.replace(old_tag, new_tag))
                else:
                    new_lines.append(line)
            new_wheel_data = "\n".join(new_lines).encode('utf-8')
            files[wheel_file_name] = new_wheel_data
            
            # 4. Calculate new hash/size for WHEEL
            wheel_hash, wheel_size = get_sha256_and_size(new_wheel_data)
            
            # 5. Update RECORD content
            record_data = files[record_file_name]
            record_lines = record_data.decode('utf-8').splitlines()
            new_record_lines = []
            for line in record_lines:
                parts = line.split(',')
                if len(parts) >= 3 and parts[0] == wheel_file_name:
                    new_record_lines.append(f"{parts[0]},{wheel_hash},{wheel_size}")
                else:
                    new_record_lines.append(line)
            files[record_file_name] = "\n".join(new_record_lines).encode('utf-8')
            
            # 6. Write all files to new zip
            for name, data in files.items():
                zout.writestr(name, data)
                
    os.replace(temp_path, new_path)
    if os.path.exists(wheel_path):
        os.remove(wheel_path)
    print(f"Successfully retagged wheel: {new_path}")
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python retag_wheel.py <wheel_dir>")
        sys.exit(1)
    
    wheel_dir = sys.argv[1]
    # Find the linux_x86_64 wheel
    wheels = glob.glob(os.path.join(wheel_dir, "*linux_x86_64.whl"))
    if not wheels:
        print("No linux_x86_64 wheel found.")
        sys.exit(0)
        
    for w in wheels:
        retag_wheel(w, "linux_x86_64", "manylinux_2_35_x86_64")