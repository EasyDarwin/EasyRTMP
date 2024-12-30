## EasyRTMP

EasyRTMP是[EasyDarwin开源社区](https://www.easydarwin.org "EasyDarwin")开发的一套RTMP直播推送功能组件，内部集成了包括：基本RTMP协议、断线重连、异步推送、环形缓冲区、推送网络拥塞自动丢帧、缓冲区关键帧检索、事件回调(断线、音视频数据回调)，通过EasyRTMP我们就可以避免接触到稍显复杂的RTMP推送或者客户端流程，只需要调用EasyRTMP的几个API接口，就能轻松、稳定地进行流媒体音视频数据的推送，支持市面上绝大部分的RTMP流媒体服务器，全平台支持：Windows、Linux、ARM(各种交叉编译工具链)、Android、iOS;


### 编译方法 ###

	Windows平台采用Visual Studio编译sln

	Linux下执行Builtit文件编译,具体如下：
	chmod +x Builtit
		

## 调用过程 ##

![EasyRTMP](https://pvale.com/share/image/EasyRTMP.png)


## 调用示例 ##

- EasyRTMP Windows：读取文件或者网络流，推送到RTMP服务器
		
		//-m: 拉流模式，TCP或者UDP
		//-d: 输入源地址，流地址或者文件地址
		//-s: 输出方式，rtmp代表RTMP推流出去
		//-f: 输出地址，RTMP推流地址
		./easyrtmp_demo.exe -m tcp -d rtsp://192.168.1.100/ch1 -s rtmp -f rtmp://127.0.0.1:10035/hls/ch1 -t 30

- EasyRTMP Android：支持前/后摄像头直播、安卓屏幕直播

	[https://fir.im/easyrtmp](https://fir.im/easyrtmp "https://fir.im/easyrtmp")


- EasyRTMP iOS：支持前/后摄像头直播

	[https://itunes.apple.com/us/app/easyrtmp/id1222410811?mt=8](https://itunes.apple.com/us/app/easyrtmp/id1222410811?mt=8 "EasyRTMP_iOS")


## 更多流媒体音视频资源

EasyDarwin开源流媒体服务器：<a href="http://www.easydarwin.org" target="_blank" title="EasyDarwin开源流媒体服务器">www.EasyDarwin.org</a>
