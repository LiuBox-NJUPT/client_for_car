#include<stdio.h>    
#include<iostream>
#include"std_msgs/String.h"
#include"std_msgs/Int8.h"
#include<vector>
    #include<stdlib.h>    
    #include<netinet/in.h>    
    #include<sys/socket.h>    
    #include<arpa/inet.h>    
    #include<string.h>    
    #include<unistd.h>    
#include<ros/ros.h>
#define BUFFER_SIZE 1024    
using namespace std;
void name_change(unsigned char name[]);
void cmd_go(ros::Publisher go_pub,string CMD);
void cmd_open(ros::Publisher open_pub,int flag);
void cmd_stop(ros::Publisher stop_pub,int flag);
int FourCharToInt(unsigned char *buffer,int offset);

void IntToFourChar(unsigned char *buffer,int car_id,int offset);
void SplitString(const string& s, vector<string>& v, const string& c);
        
    int main(int argc, char ** argv)    
    {    
        struct sockaddr_in server_addr;    
        server_addr.sin_family = AF_INET;    
        server_addr.sin_port = htons(4399);    
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");   //ip 
        bzero(&(server_addr.sin_zero), 8);    
        //设置超时机制
	struct timeval tv;
       tv.tv_sec=1;
	tv.tv_usec=0;       

	ros::init(argc,argv,"socket_node");
	ros::NodeHandle nh;

	/*用话题实现命令发布*/
	ros::Publisher go_pub=nh.advertise<std_msgs::String>("cmd_go",1000);
	ros::Publisher open_pub=nh.advertise<std_msgs::Int8>("cmd_open",1000);
	ros::Publisher stop_pub=nh.advertise<std_msgs::Int8>("cmd_stop",1000);

	vector<string> CMD; //存储命令的不同分割
	string str_cmd;  //存储命令的string形式

	/*检查socket连接*/
        int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);    
        if(server_sock_fd == -1)    
        {    
        perror("socket error");    
        return 1;    
        }    
	else
	{
	cout<<"socket success"<<endl;}

        unsigned char recv_msg[BUFFER_SIZE];    
        unsigned char input_msg[BUFFER_SIZE];	
	unsigned char recv_msg_alter[BUFFER_SIZE];//从指令中截取的具体命令
	bzero(recv_msg,BUFFER_SIZE);
	bzero(input_msg,BUFFER_SIZE);
	bzero(recv_msg_alter,sizeof(recv_msg_alter));


	int name_ud,p=0,byte_num=0;	//name_ud为从服务器解析出的id，byte_num为解析出的命令长度
					//p为缓冲区读取位置的标记	
	unsigned char initialize_char[BUFFER_SIZE];
	string name="bat&95,lati&42505589N,long&147185084E";
	IntToFourChar(initialize_char,name.size()+4,0);
	IntToFourChar(initialize_char,1,4);
	memcpy(initialize_char+8,&name[0],name.size());

       while(1) 
       {
	connect(server_sock_fd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in));
	send(server_sock_fd,initialize_char,sizeof(initialize_char),0);    
 
	recv(server_sock_fd,recv_msg,BUFFER_SIZE,0);
	byte_num=FourCharToInt(recv_msg,p);
	while(1){		
       	if(byte_num > 0)    
	{    
		name_ud=FourCharToInt(recv_msg,p+4);

		printf("服务器指令长度:%d,小车代号:%d\n", byte_num,name_ud);
		if(byte_num>BUFFER_SIZE)
		{cout<<"解析出指令长度异常，自动转化为"<<BUFFER_SIZE<<endl;byte_num=BUFFER_SIZE;}
		
		memcpy(recv_msg_alter,recv_msg+p+8,byte_num-4);	
	    	str_cmd=(const char*)recv_msg_alter;
		cout<<str_cmd<<endl;
		SplitString(str_cmd,CMD,"&");
		
		if(CMD[0]=="go")
		{
		cmd_go(go_pub,CMD[1]);	
		}

		else if(CMD[0]=="stop"){
		cmd_stop(stop_pub,atoi(CMD[1].c_str()));
		}

		else if(CMD[0]=="open"){
		cmd_open(open_pub,atoi(	CMD[1].c_str()));
		}
	}

                else if(byte_num < 0)    
                {    
                printf("接受消息出错!\n");
	    	break;	
                }    



                  //进入常发状态  
                    
	//	cmd_stop(stop_pub,-1);
	//	cmd_open(open_pub,0);
	//	cmd_go(go_pub,0);		    

	p+=byte_num;                    
	if(recv_msg[p]!='\0')
	byte_num=FourCharToInt(recv_msg,p);
	else
		break;
	
	}

        /*下面清空缓存区*/
                bzero(recv_msg, BUFFER_SIZE);   
		bzero(input_msg,BUFFER_SIZE);
		bzero(recv_msg_alter,sizeof(recv_msg_alter));
		byte_num=0;p=0;
		CMD.resize(0);
			
	    
      }    
       close(server_sock_fd);
        return 0;    
}   


void cmd_go(ros::Publisher go_pub,string CMD)
{
	if(ros::ok()){
		std_msgs::String msg;
		msg.data=CMD;
		go_pub.publish(msg);
	}
}
void cmd_open(ros::Publisher open_pub,int flag)
{
	if(ros::ok())
	{
		std_msgs::Int8 msg;
		msg.data=flag;
		open_pub.publish(msg);
	}
}
void cmd_stop(ros::Publisher stop_pub,int flag)
{
	//cout<<"stop ready"<<endl;
	if(ros::ok()){
		std_msgs::Int8 msg;
		msg.data=flag;
		stop_pub.publish(msg);
	}
}
void SplitString(const string& s, vector<string>& v, const string& c)
{
  string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while(string::npos != pos2)
  {
    v.push_back(s.substr(pos1, pos2-pos1));

    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if(pos1 != s.length())
    v.push_back(s.substr(pos1));
}
int FourCharToInt(unsigned char* buffer,int offset){
	return (buffer[offset    ]<<24)
	      |(buffer[offset + 1]<<16)
	      |(buffer[offset + 2]<< 8)
	      |(buffer[offset + 3]    );
}
void IntToFourChar(unsigned char *buffer,int car_id,int offset)
{
	buffer[offset  ]=(car_id>>24)&0xFF;
	buffer[offset+1]=(car_id>>16)&0xFF;
	buffer[offset+2]=(car_id>> 8)&0xFF;
	buffer[offset+3]=(car_id)    &0xFF;
}



