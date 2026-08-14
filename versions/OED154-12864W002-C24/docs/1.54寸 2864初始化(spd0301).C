/*  QG-2864KSWNG01  */



void spd0301()
{
   RES=0;
   delay(1000);
   RES=1;
   delay(1000);
   
   write_i(0xae); /* set  display off */
   
   write_i(0x00); /* set  lower column start address */
   write_i(0x10); /* set  higher column start address */
   
   write_i(0x40); /* set  display start line */
   
   write_i(0x2E);
   
   write_i(0x81); /* set  contrast control */
   write_i(0x32); 
   
   write_i(0x82);    
       write_i(0x80);
   
   write_i(0xa1); /* set  segment remap  */
   
   write_i(0xa6); /* set  normal display */
   
   write_i(0xa8); /* set  multiplex ratio */
   write_i(0x3f); /* 1/64 */
   
   write_i(0xad); /* master configuration */
   write_i(0x8e); /* external vcc supply */
   
   write_i(0xc8); /* set  com scan direction */
   
   write_i(0xd3); /* set  display offset  */
   write_i(0x40);
   
   write_i(0xd5); /* set  display clock divide/oscillator frequency */
   write_i(0xf0); 
   
   write_i(0xD8);    /*set area color mode off */
       write_i(0x05);
       
       write_i(0xD9);
       write_i(0xF1);
   
   write_i(0xda); /* set  com pin configuartion */
   write_i(0x12); 
   
   write_i(0x91);
   write_i(0x3F);
    write_i(0x3F);
      write_i(0x3F);
       write_i(0x3F); 
   
   write_i(0xaf); /* set  display on */
}

void write_i(unsigned char ins)
{
   RS=0;
   CS=0;
   WR=0;
   P1=ins;
   WR=1;
   CS=1;
}
 
void write_d(unsigned char dat)  
{
   RS=1;
   CS=0;
   WR=0;
   P1=dat;
   WR=1;
   CS=1;
}      	

void delay(unsigned int i)
{
   while(i>0)
      {
      	i--;
      }
}

