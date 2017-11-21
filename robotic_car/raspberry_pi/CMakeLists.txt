cmake_minimum_required(VERSION 2.8)
project( robot )
find_package( OpenCV REQUIRED)
include_directories(.)
include_directories(/opt/vc/include)
include_directories(/home/pi/git/raspberrypi/userland)
include_directories(/home/pi/git/raspberrypi/userland/host_applications/linux/libs/bcm_host/include)
include_directories(/home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam)
include_directories(/home/pi/git/raspberrypi/userland/interface/vcos)
include_directories(/home/pi/git/raspberrypi/userland/interface/vcos/pthreads)
include_directories(/home/pi/git/raspberrypi/userland/interface/vmcs_host/linux)
include_directories(/home/pi/git/raspberrypi/userland/interface/khronos/include)
include_directories(/home/pi/git/raspberrypi/userland/interface/khronos/common)
include_directories(/home/pi/git/robidouille/raspicam_cv)
include_directories(/home/pi/robotbil/libs/raspicam_cv)
include_directories(/home/pi/robotbil/libs)

add_library(RaspiCamCV 
  ./libs/raspicam_cv/RaspiCamCV.c 
  ./libs/robotnetwork.c 
  ./libs/robotserial.c)

add_executable(robot main.c 
  ./libs/raspicam_cv/RaspiCamCV.c
  ./libs/robotnetwork.c
  ./libs/robotserial.c
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/RaspiCamControl.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/RaspiPreview.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/RaspiCLI.c  
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/RaspiTex.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/RaspiTexUtil.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/tga.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/gl_scenes/mirror.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/gl_scenes/sobel.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/gl_scenes/square.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/gl_scenes/teapot.c 
  /home/pi/git/raspberrypi/userland/host_applications/linux/apps/raspicam/gl_scenes/yuv.c)

target_link_libraries( robot 
  ${OpenCV_LIBS} 
  /opt/vc/lib/libmmal_core.so 
  /opt/vc/lib/libmmal_util.so 
  /opt/vc/lib/libmmal_vc_client.so 
  /opt/vc/lib/libvcos.so 
  /opt/vc/lib/libbcm_host.so 
  /opt/vc/lib/libGLESv2.so 
  /opt/vc/lib/libEGL.so)