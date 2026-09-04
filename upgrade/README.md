<!--
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-07-26 15:10:04
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-12 09:12:09
 * @FilePath: /Doorbell/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# 更新说明


## 20241224
1.修复运行初死机，原因：在定时器结构体未初始化前，红外检测线程调用了该接口，后定时器结构体初始化时将数据重新清除，导致数据丢失，段错误；解决方法，为了避免定时器使用受初始化流程影响，将定时器结构体数组静态赋值，在程序编译时完成初始化,并删除mian函数中Timer_init函数调用及删除该函数

## 20250106
1.在CircularListRequest申请节点时，避免节点在申请后未使用，导致下次申请失败，后续所有节点的申请都必须重新写入到队列中；此问题分别修改了语音播放、音频输出等相关队列
2.添加用户默认配置文件，添加默认配置文件保存接口
3.优化管理员、开锁等密码设置功能，新增密码范围控制，支持4-6位开锁密码
4.优化数字键盘修改语言命令，添加默认出厂语言设置功能
    (1)修改默认出厂语言指令 9 + DEFAULT_FACTORY_SET_FLAG + language + #    (DEFAULT_FACTORY_SET_FLAG = 9)
    (2)修改语言指令 9 + language + #
5.优化数字按键公共密码开锁指令，添加默认出厂密码设置功能
    (1)修改默认出厂Lock锁公共密码指令 011 + DEFAULT_FACTORY_SET_FLAG + 新密码 + # + 二次确认密码 + #
    (2)修改默认出厂Lock锁公共密码指令 022 + DEFAULT_FACTORY_SET_FLAG + 新密码 + # + 二次确认密码 + #
6.修复管理员模式下*返回后未关闭30秒超时退出管理员定时器
7.优化PushRouteStack接口逻辑，删除该接口ExitHandle回调函数调用操作
8.优化红外检测功能，红外中断触发后开启或刷新红外电平稳定检测定时器，定时器开启期间中断触发重新刷新计时，定时结束作一次有效检测，定时时间一秒

## 20250108
1.修复管理员密码修改失败问题
2.添加复位时不会复位用户设置的语言
3.优化数字按键设置开锁时间，添加默认出厂开锁时间设置
    (1)修改默认出厂Gate开锁时间 4 + DEFAULT_FACTORY_SET_FLAG + ***(Time)
    (2)修改默认出厂Lock开锁时间 2 + DEFAULT_FACTORY_SET_FLAG + ***(Time)

## 20250109
1.更换视频采集夜视效果配置文件
2.添加系统事件注册及循环接口
3.修复红外切换阻塞问题，其原因是定时器抢占了安凯视频采集配置线程，执行夜视效果切换，导致其安凯接口内部阻塞，解决方案如4
4.优化视频采集夜视切换及I帧请求接口，将该功能更改为原子标志位，具体切换请求放到视频采集线程处理
5.修复数字按键连续删除卡片时序号0卡无法删除问题，优化删除逻辑

## 20250111
1.添加获取本地设备ID接口
2.修复另一台门口机通话时，本机通话灯错误亮起，其原因是接收到通话消息未作校验确认是否对象是本机，从而开启通话灯，将该校验逻辑补上修复该问题

## 20250308
1.修复警报触发后呼叫灯在未监控下常亮问题,在警报结束后判断是否处于监控来控制呼叫灯
2.添加网卡接收异常后重启网卡设备处理-NetMsgcomm.c、NetworkRaw.c

## 20250311
1.修复上电协商慢导致网卡重启操作，引起套接字无法正常工作问题，修改逻辑：检测到正常工作后异常才开启网卡重启处理

## 20250315
1.修复在修改卡片密码时，为销毁定时器而设置一毫秒触发导致在其他操作销毁定时器时冲突，死机
2.定时器添加原子操作，防止二次删除竞争操作

## 一下是hare所注及修改 ##
## 20250507
1.修改概率性进监控无图或协商不上的问题 修改问题思路:加载模块还未完成时 就先初始化了 导致无图出错  处理办法:VideoInputOpen函数中添加usleep(500*1000)延时

## 2025.11.13
1.音频、视频采集改为自在监控内触发
2.在 main.c 主函数内添加内核崩溃等日志文本对比检测 检测到相关打印就重启门口机