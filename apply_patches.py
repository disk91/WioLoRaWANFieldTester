from os.path import join, isfile, isdir

Import("env")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduino-samd-seeed")
LIBS_DIRS = env.GetLibSourceDirs()

def find_library(name):
    for dir in LIBS_DIRS:
        lib_dir = join(dir, name)
        if isdir(lib_dir):
            return lib_dir
    return None

def patch_file(file, patch):
    if isfile(file) and isfile(patch):
        if not isfile(file + ".patched"):
            env.Execute("patch -p0 -N -s \"%s\" \"%s\"" % (file, patch))
            env.Execute("touch \"%s.patched\"" % file)

# Patch Adafruit_GPS library
folder = find_library("Adafruit GPS Library")
if folder:
    file = join(folder, "src", "Adafruit_GPS.h")
    patch = join("patches", "Adafruit_GPS.h.patch")
    patch_file(file, patch)

# Patch MCCI LoRaWAN LMIC library
folder = find_library("MCCI LoRaWAN LMIC library")
if folder:
    file = join(folder, "project_config", "lmic_project_config.h")
    patch = join("patches", "lmic_project_config.h.patch")
    patch_file(file, patch)

# Patch SoftwareSerial library
file = join(FRAMEWORK_DIR, "libraries", "SoftwareSerial", "SoftwareSerial.h")
patch = join("patches", "SoftwareSerial.h.patch")
patch_file(file, patch)

