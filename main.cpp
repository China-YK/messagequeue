#include <sys/msg.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

typedef struct
{
    long type;//System V规定，消息队列的每个消息的开头必须是一个类型为long的字段，字段名随意（队列先进先出）
    struct
    {
        int id;
        char name[128];
        int age;
        char message[512];
    }data;
}MyStruct;
void test_messagequeue(){
    pid_t pid = fork();
    if (pid > 0){
        int msg_id = msgget(ftok(".",1),IPC_CREAT|0666);
        //来吧孩子们...
        //msgget方法名和前面一样的逻辑xxxget
        //第一个参数ftok不多讲了
        //第二个参数控制行为设置，也不多讲了
        MyStruct my_struct;
        memset(&my_struct,0,sizeof(my_struct));
        //结构体≠数组，所以我们这里要取地址才行
        ssize_t ret = msgrcv(msg_id,&my_struct,sizeof(my_struct)-sizeof(long),1,0);
        //讲一下msgrcv
        //第一个参数，消息队列id表示操作那个消息队列(在底层其实是一个链表，然后有意思的是每个节点元素可以不一样，struct student->struct people->nullptr)
        //第二个参数，指向这个节点(消息)的指针，在msgrcv里面表示用来存放符合条件的那个消息节点(拷贝),然后队列删除那个节点
        //这里讲一下为什么要使用拷贝，明明拷贝效率底下。因为消息队列是内核对象属于内核态，而这里的msg属于用户态，linux最基本的就是保障用户态(内存)和内核态(地址空间)的隔离嘛
        //第三个参数，表示这个消息节点的字节数，但是要删去类型字段的大小:-sizeof(long)
        //第四个参数，表示消息类型筛选条件
        //0表示选择的是第一个队列节点(队头)
        //>0(K)表示选择的是第一个类型字段的值为K的节点
        //<0(X)先将X取绝对值abs(X),然后选择整个队列里面 类型字段的值 <= abs(X) 的消息中最小的那一个
        //第五个参数，表示控制行为方式，通常可以使用|组合
        //0表示阻塞等待，如果目前队列中没有符合条件的节点，那就在msgrcv阻塞，直到等待第一个符合条件的节点
        //IPC_NOWAIT表示非阻塞等待，如果目前消息队列中没有符合条件的节点，那就直接返回-1，并且设置errno=ENOMSG
        //MSG_NOERROR表示可以截断消息，表示如果消息体太长，会截断到 sizeof(my_struct)-sizeof(long) 长度并返回成功
        if (ret == -1){//成功收到ret==0，失败则ret==-1
            fprintf(stderr,"message receive failed!\n");
            exit(-1);
        }
        fprintf(stdout,"id:%d name:%s age:%d data:%s",my_struct.data.id,my_struct.data.name,my_struct.data.age,my_struct.data.message);
        msgctl(msg_id,IPC_RMID,nullptr);
    }else if (pid == 0){
        int msg_id = msgget(ftok(".",1),0666);
        MyStruct my_struct;
        my_struct.data.id = 2025103620;
        strcpy(my_struct.data.name,"YK yyds!");
        my_struct.data.age = 23;
        strcpy(my_struct.data.message,"hello fprocess!");
        my_struct.type = 1;
        ssize_t ret = msgsnd(msg_id,&my_struct,sizeof(my_struct)-sizeof(long),0);
        //发送消息到消息队列里面
        //前三个参数不多讲了，就是要注意，这里哪怕是发送也需要使用地址传递(第二个参数)
        //只有4个参数，第四个参数表示控制行为方式
        //0表示阻塞发送，如果队列满了(一般linux系统里面默认的队列大小为16k)，就阻塞发送，直到队列有空间发送这条消息
        //IPC_NOWAIT表示非阻塞发送，若队列满了(剩余的空间不足以这条消息发送)，就直接返回-1，并且设置errno=EAGAIN
        fprintf(stdout,"已发送");
        if (ret == -1){
            fprintf(stderr,"message send failed!\n");
            exit(-1);
        }

        // msgctl(msg_id,IPC_RMID,nullptr);
    }else{
        fprintf(stderr,"fork failed!\n");
        exit(-1);
    }
}
int main(){
    test_messagequeue();
    return 0;
}