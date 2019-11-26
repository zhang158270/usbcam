# usbcam
take pictures for  usb camera  

1.查询摄像头是否存在
ls /dev/video0 
/dev/video0

2.安装配置工具
$ apt-get install v4l-utils

3.查询所有参数
 v4l2-ctl --all
 
 4.与曝光相关的参数
曝光模式
exposure_auto 0x009a0901 (menu)   : min=0 max=3 default=3 value=1
相机曝光时间
exposure_absolute 0x009a0902 (int)    : min=1 max=5000 step=1 default=157 value=200
曝光自动优先级
exposure_auto_priority 0x009a0903 (bool)   : default=0 value=1

曝光模式-详细解释
0--V4L2_EXPOSURE_AUTO 
Automatic exposure time, automatic iris aperture.自动曝光时间，自动光圈光圈。

1--V4L2_EXPOSURE_MANUAL 
Manual exposure time, manual iris.手动曝光时间，手动光圈。

2--V4L2_EXPOSURE_SHUTTER_PRIORITY 
Manual exposure time, auto iris.手动曝光时间，自动光圈。

3--V4L2_EXPOSURE_APERTURE_PRIORITY 
Auto exposure time, manual iris.自动曝光时间，手动光圈。

相机曝光时间 说明
当exposure_auto=1( V4L2_EXPOSURE_MANUAL )时可设置曝光绝对值。

曝光自动优先级
Frame rate is set to be camera_fps. Without settingexposure_auto_priority, AE will never change this frame rate - you will get aframe every 1/fps seconds. AE can request an exposure time that is < 1/fpsseconds, but never exceed it. The exposure mode (night, sport, etc) variesother parameters such as whether to increase gain or exposure time first, andwhat maximum value to adopt for each.
帧频设置为camera_fps。 如果没有设置exposure_auto_priority，AE将永远不会更改此帧速率-您将每1 / fps秒获得一次帧。 AE可以请求小于1 / fps秒的曝光时间，但决不能超过它。 曝光模式（夜晚，运动等）会改变其他参数，例如是先增加增益还是先增加曝光时间，以及每个参数采用的最大值。
 也就是说exposure_auto_priority=0是锁定camera的fps，exposure_auto_priority=1是fps不固定，曝光设置优先。

实际设置
1.v4l2-ctl -c exposure_auto=1
2. v4l2-ctl -c exposure_absolute=100
