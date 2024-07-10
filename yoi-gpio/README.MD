
# 명령어

```bash
sudo gcc gpio.c -o gpio
gpio
gpio
sudo ./gpio read
```

写 gpio
9555 有16个pin，分成两个部分， 高8位， 第一个参数 write, 第二个参数1, 第三个参数时8个PIN组成的byte值
sudo ./gpio write 1 255
低8位
sudo ./gpio write 0 00

gpio默认为高电平

sudo ./gpio write 0 0 (1-8bit全低）

sudo ./gpio write 1 0 (9-12bit全低）

sudo ./gpio write 0 255 (1-8全高）

sudo ./gpio write 1 255 (9-12全高）

测试输入：(gpio脚分别对地短接）
sudo ./gpio read


