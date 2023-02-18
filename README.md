### 1、demo使用

```bash
$ make clean && make DEBUG=1
$
$ ./ts_mux_h264_aac
Usage:
   ./ts_mux_h264_aac avfile/test1_856x480_24fps.h264 24 avfile/test1_44100_stereo.aac out1.ts
   ./ts_mux_h264_aac avfile/test2_720x480_30fps.h264 60 avfile/test2_48000_stereo.aac out2.ts
   ./ts_mux_h264_aac avfile/test3_1280x720_20fps.h264 20 avfile/test1_44100_stereo.aac out3.ts
```

（注：目前合成的out1.ts在使用potPlayer播放时进度条有点问题，待修复。）

**目前合成视频的现象：**

（out2.ts含有除SPS、PPS、IDR、PSLICE以外的其他NALU，所以主要看out1.ts和out3.ts）

- **out1.ts：**
  - 电影和电视、ACG播放器
    - 播放不流畅（含有B帧，而程序只填pts没填dts导致，对比out3.ts可知，无关紧要）；
    - 正常播放进度条时长正常（29s），拖动后有声音但画面卡住一会才动、音视频同步；
  - VLC
    - 播放不流畅（含有B帧，而程序只填pts没填dts导致，对比out3.ts可知，无关紧要）；
    - 正常播放进度条时长正常（29s），拖动后有声音但画面会灰屏有画面在动过一会才恢复、音视频同步；
  - potplayer
    - 播放流畅；
    - 正常播放时进度条时长不正常（21s），拖动画面不会卡住、但音视频有时不同步；
- **out3.ts**
  - 电影和电视、ACG播放器
    - 播放流畅；
    - 正常播放进度条时长正常（30s），拖动后有声音但画面卡住一会才动、音视频“同步”；
  - VLC
    - 播放流畅；
    - 正常播放进度条时长正常（30s），拖动后有声音但画面会灰屏有画面在动过一会才恢复、音视频“同步”；
  - potplayer
    - 播放流畅；
    - 正常播放时进度条时长不正常（8s），拖动画面和声音都会卡住一会才动；






### 2、参考文章

 - [【科普】TS文件格式：什么是TS？如何打开和转换它？\_都叫兽软件](https://www.reneelab.com.cn/m/what-is-ts-file-and-how-to-play-ts-video-1.html)

 - [TS封装格式 - CrazyDiode - 博客园](https://www.cnblogs.com/CoderTian/p/7198765.html)**（推荐！！看这篇基本就够了！！！其余文章作为参考即可。）**

 - [TS文件格式\_LaugustusJ的博客-CSDN博客\_ts文件](https://blog.csdn.net/qq_16334327/article/details/95936374)**（没仔细看）**

 - [TS 文件格式解析\_影音视频技术-CSDN博客\_ts格式解析](https://blog.csdn.net/coloriy/article/details/79852682)**（没仔细看）**

 - [测试验证：h264实时流封装ts文件存储，完整实现 - jamin-snails的个人空间 - OSCHINA - 中文开源技术交流社区](https://my.oschina.net/u/2430809/blog/727904)**（没仔细看）**

**工具下载：**

 - [SpecialFTS.exe(demo中的tools/SpecialFTS\_1.1.7z)](https://sourceforge.net/projects/tsformatanalysis/files/binary/SpecialFTS_1.1.7z/download)【[源码下载地址](https://sourceforge.net/projects/tsformatanalysis/)】

**参考源码：**

 - [https://download.csdn.net/download/zhuweigangzwg/5605869?spm=1003.2166.3001.6637.7](https://download.csdn.net/download/zhuweigangzwg/5605869?spm=1003.2166.3001.6637.7)**（推荐！！）**
 - [https://github.com/Jsnails/MUX\_TS](https://github.com/Jsnails/MUX_TS)**（根据前者进行改动的，可以不用看）**



### 附录（demo目录架构）

```bash
$ tree
.
├── aac_adts.c
├── aac_adts.h
├── avfile
│   ├── out1.ts
│   ├── out2.ts
│   ├── out3.ts
│   ├── test1_44100_stereo.aac
│   ├── test1_856x480_24fps.h264
│   ├── test2_48000_stereo.aac
│   ├── test2_720x480_30fps.h264
│   └── test3_1280x720_20fps.h264
├── crcLib.c
├── crcLib.h
├── docs
│   ├── TS封装格式 - CrazyDiode - 博客园.mhtml
│   ├── TS文件格式_LaugustusJ的博客-CSDN博客_ts文件.mhtml
│   ├── TS 文件格式解析_影音视频技术-CSDN博客_ts格式解析.mhtml
│   ├── 测试验证：h264实时流封装ts文件存储，完整实现 - jamin-snails的个人空间 - OSCHINA - 中文开源技术交流社区.mhtml
│   └── 【科普】TS文件格式：什么是TS？如何打开和转换它？_都叫兽软件.mhtml
├── h264_nalu.c
├── h264_nalu.h
├── main.c
├── Makefile
├── README.md
├── reference_src
│   ├── H264_AAC_TS_MUX.tar.bz2
│   └── MUX_TS-master.zip
├── tools
│   └── SpecialFTS_1.1.7z
├── ts.c
└── ts.h
```

