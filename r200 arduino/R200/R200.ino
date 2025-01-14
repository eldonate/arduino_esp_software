/*
由四川睿频科技有限公司编写
复制及使用请保留版权所属
*/


/*多次读取指令*/
unsigned char ReadMulti[10] = {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;


void setup() {
  //设置串口，并设置LED
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200); //设置串口波特率115200
  Serial.println("Hello world.");// "Hello world."
  Serial.write(ReadMulti,10);
}

void loop() {

  //间隔一段时间后发生循环读取命令
  timeSec ++ ;
  if(timeSec >= 50000){
    timemin ++;
    timeSec = 0;
    if(timemin >= 20){
      timemin = 0;
      //发送循环读取指令
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.write(ReadMulti,10);
      digitalWrite(LED_BUILTIN, LOW); 
    }
  }
 
  if(Serial.available() > 0)//串口接收到数据
  {
    //收到回复,以下是读到卡片示例
    //AA 02 22 00 11 C7 30 00 E2 80 68 90 00 00 50 0E 88 C6 A4 A7 11 9B 29 DD 
    /*
    AA:帧头
    02:指令代码
    22:指令参数
    00 11:指令数据长度，17个字节
    C7：RSSI信号强度
    30 00:标签PC码：标签厂相关信息登记
    E2 80 68 90 00 00 50 0E 88 C6 A4 A7：EPC代码
    11 9B:CRC校验
    29:校验
    DD:帧尾 
    */
    incomedate = Serial.read();//获取串口接收到的数据
    //判断是否为对应指令代码
    if((incomedate == 0x02)&(parState == 0))
    {
      parState = 1;
    }
    //判断是否为对应指令参数
    else if((parState == 1)&(incomedate == 0x22)&(codeState == 0)){  
        codeState = 1;
        dataAdd = 3;
    }
    else if(codeState == 1){
      dataAdd ++;
      //获取RSSI
      if(dataAdd == 6)
      {
        Serial.print("RSSI:"); 
        Serial.println(incomedate, HEX); 
        }
       //获取PC码
       else if((dataAdd == 7)|(dataAdd == 8)){
        if(dataAdd == 7){
          Serial.print("PC:"); 
          Serial.print(incomedate, HEX);
        }
        else {
           Serial.println(incomedate, HEX);
        }
       }
       //获取EPC，如需对EPC处理，可以在该处进行
       else if((dataAdd >= 9)&(dataAdd <= 20)){
        if(dataAdd == 9){
          Serial.print("EPC:"); 
        }        
        Serial.print(incomedate, HEX);
       }
       //位置溢出，进行重新接收
       else if(dataAdd >= 21){
        Serial.println(" "); 
        dataAdd= 0;
        parState = 0;
        codeState = 0;
        }
    }
     else{
      dataAdd= 0;
      parState = 0;
      codeState = 0;
    }
  }
}
