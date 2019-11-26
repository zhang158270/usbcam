/*v4l2_example.c*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>



#define TRUE            (1)
#define FALSE           (0)

#define FILE_VIDEO      "/dev/video0"
#define IMAGE           "./img/"

#define IMAGEWIDTH      640
#define IMAGEHEIGHT     480

#define FRAME_NUM       4

int fd;
struct v4l2_buffer buf;

struct buffer
{
	void * start;
	unsigned int length;
	long long int timestamp;
} *buffers;


int v4l2_init()
{
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_format fmt;
	struct v4l2_streamparm stream_para;

	int ret;

	struct v4l2_control ctrl;

	//打开摄像头设备
	if ((fd = open(FILE_VIDEO, O_RDWR)) == -1)
	{
		printf("Error opening V4L interface\n");
		return FALSE;
	}

	//查询设备属性
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		printf("Error opening device %s: unable to query device.\n",FILE_VIDEO);
		return FALSE;
	}
	else
	{
		printf("driver:\t\t%s\n",cap.driver);
		printf("card:\t\t%s\n",cap.card);
		printf("bus_info:\t%s\n",cap.bus_info);
		printf("version:\t%d\n",cap.version);
		printf("capabilities:\t%x\n",cap.capabilities);
		//4L2_CAP_VIDEO_CAPTURE 0x00000001
		// 这个设备支持 video capture 的接口，即这个设备具备 video capture 的功能
		if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
		{
			printf("Device %s: supports capture.\n",FILE_VIDEO);
		}
		//V4L2_CAP_STREAMING 0x04000000
		// 这个设备是否支持 streaming I/O 操作函数
		if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
		{
			printf("Device %s: supports streaming.\n",FILE_VIDEO);
		}
	}

	//Support format:
	//  1.MJPEG
	//  2.YUV 4:2:2 (YUYV)
	//显示所有支持帧格式
	fmtdesc.index=0;
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
	{
		printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
		fmtdesc.index++;
	}

	//检查是否支持某帧格式
	//V4L2_PIX_FMT_YUYV
	//V4L2_PIX_FMT_MJPEG
	struct v4l2_format fmt_test;
	fmt_test.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;

	fmt_test.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;
	if(ioctl(fd,VIDIOC_TRY_FMT,&fmt_test)==-1)
		printf("not support format YUYV!\n");
	else
		printf("support format YUYV\n");

	fmt_test.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG;
	if(ioctl(fd,VIDIOC_TRY_FMT,&fmt_test)==-1)
		printf("not support format MJPEG!\n");
	else
		printf("support format MJPEG\n");

	fmt_test.fmt.pix.pixelformat=V4L2_PIX_FMT_GREY;
	if(ioctl(fd,VIDIOC_TRY_FMT,&fmt_test)==-1)
		printf("not support format grey!\n");
	else
		printf("support format grey\n");



	//查看及设置当前格式
	printf("set fmt...\n");
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.height = IMAGEHEIGHT;
	fmt.fmt.pix.width = IMAGEWIDTH;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;


	if(ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
		printf("Unable to set format--VIDIOC_S_FMT\n");
	else
		printf("be able to set format--VIDIOC_S_FMT\n");
	//查询
	if(ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
		printf("Unable to set format--VIDIOC_G_FMT\n");
	else
		printf("be able to set format--VIDIOC_G_FMT\n");

	printf("fmt.type:\t\t%d\n",fmt.type);
	printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
	printf("pix.height:\t\t%d\n",fmt.fmt.pix.height);
	printf("pix.width:\t\t%d\n",fmt.fmt.pix.width);
	printf("pix.field:\t\t%d\n",fmt.fmt.pix.field);

	return TRUE;
}



int v4l2_mem_ops()
{
	unsigned int n_buffers;
	struct v4l2_requestbuffers req;

	//申请帧缓冲
	req.count=FRAME_NUM;
	req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory=V4L2_MEMORY_MMAP;
	if(ioctl(fd,VIDIOC_REQBUFS,&req)==-1)
	{
		printf("request for buffers error\n");
		return FALSE;
	}

	// 申请用户空间的地址列
	buffers = malloc(req.count*sizeof (*buffers));
	if (!buffers)
	{
		printf ("out of memory!\n");
		return FALSE;
	}

	// 进行内存映射
	for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
	{
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		//查询
		if (ioctl (fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			printf("query buffer error\n");
			return FALSE;
		}

		//映射
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if (buffers[n_buffers].start == MAP_FAILED)
		{
			printf("buffer map error\n");
			return FALSE;
		}
	}
	return TRUE;
}



int v4l2_frame_process()
{
	unsigned int n_buffers;
	enum v4l2_buf_type type;
	char file_name[100];
	char index_str[50];
	long long int extra_time = 0;
	long long int cur_time = 0;
	long long int last_time = 0;
	struct timeval    tv;
	struct timezone tz;
	struct tm         *p;

	//入队和开启采集
	for (n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
	{
		buf.index = n_buffers;
		ioctl(fd, VIDIOC_QBUF, &buf);
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMON, &type);



	//出队，处理，写入yuv文件，入队，循环进行
	int loop = 0;
	while(loop < 100)//一次循环4张
	{
		for(n_buffers = 0; n_buffers < FRAME_NUM; n_buffers++)
		{
			//出队
			buf.index = n_buffers;
			ioctl(fd, VIDIOC_DQBUF, &buf);

			//查看采集数据的时间戳之差，单位为
			//   buffers[n_buffers].timestamp = buf.timestamp.tv_sec*1000000+buf.timestamp.tv_usec;
			//  cur_time = buffers[n_buffers].timestamp;
			//  extra_time = cur_time - last_time;
			//   last_time = cur_time;
			//  printf("time_deta:%lldms\n\n",extra_time/1000);
			//printf("buf_len:%d\n",buffers[n_buffers].length);

			//处理数据只是简单写入文件，名字以loop的次数和帧缓冲数目有关
			printf("grab image data OK\n");
			memset(file_name,0,sizeof(file_name));
			memset(index_str,0,sizeof(index_str));
			gettimeofday(&tv, &tz);
			p = localtime(&tv.tv_sec);
			sprintf(index_str,"%d-%d-%d-%ld",p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec/1000);
			strcpy(file_name,IMAGE);//字符串复制
			strcat(file_name,index_str);//字符串追加到
			strcat(file_name,".jpg");
			FILE *fp2 = fopen(file_name, "wb");
			if(!fp2)
			{
				printf("open %s error\n",file_name);
				return(FALSE);
			}
			fwrite(buffers[n_buffers].start, IMAGEHEIGHT*IMAGEWIDTH*2,1,fp2);
			fclose(fp2);
			printf("save %s OK\n",file_name);

			//入队循环
			ioctl(fd, VIDIOC_QBUF, &buf);
		}

		loop++;
	}
	return TRUE;
}




int v4l2_release()
{
	unsigned int n_buffers;
	enum v4l2_buf_type type;

	//关闭流
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMON, &type);

	//关闭内存映射
	for(n_buffers=0; n_buffers<FRAME_NUM; n_buffers++)
	{
		munmap(buffers[n_buffers].start,buffers[n_buffers].length);
	}

	//释放自己申请的内存
	free(buffers);

	//关闭设备
	close(fd);
	return TRUE;
}



int main(int argc, char const *argv[])
{
	v4l2_init();
	printf("init....\n");

	v4l2_mem_ops();
	printf("malloc....\n");

	v4l2_frame_process();
	printf("process....\n");

	v4l2_release();
	printf("release\n");

	return TRUE;
}
