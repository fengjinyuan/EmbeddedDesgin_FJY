#include "gpio_iic.h"
#include "uart.h"
#include "mpu_9250.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "mpu_DMP.h"

#define PI    3.14159265358979323846

#define Kp 100.0f//100.0f
#define Ki 0.002f
#define halfT 0.001f
#define sampleFreq   1
float q0 = 1.0f,q1 = 0.0f,q2 = 0.0f,q3 = 0.0f;

#define q30		1073741824.0f
#define Pitch_error	1.0
#define Roll_error	-2.0
#define Yaw_error	0.0
//float q0 = 1, q1 = 0, q2 = 0, q3 = 0;
float exInt = 0, eyInt = 0, ezInt = 0;

typedef struct{
	short X;
	short Y;
	short Z;
}AXIS;
AXIS mpu_9250_accl,mpu_9250_gyro,mpu_9250_mag;
AXIS q_angle;

void Read_Accl_Gyro(){
	  u8 ACCL_GYRO_BUFFER[14];
	  Read_ByteBuffer(GYR_ACC_DEVICE,ACCL_REGISTER,ACCL_GYRO_BUFFER,14);
		mpu_9250_accl.X = (short)(ACCL_GYRO_BUFFER[0]<<8|ACCL_GYRO_BUFFER[1]);
		mpu_9250_accl.Y = (short)(ACCL_GYRO_BUFFER[2]<<8|ACCL_GYRO_BUFFER[3]);
		mpu_9250_accl.Z = (short)(ACCL_GYRO_BUFFER[4]<<8|ACCL_GYRO_BUFFER[5]);
		mpu_9250_gyro.X = (short)(ACCL_GYRO_BUFFER[8]<<8|ACCL_GYRO_BUFFER[9]);
		mpu_9250_gyro.Y = (short)(ACCL_GYRO_BUFFER[10]<<8|ACCL_GYRO_BUFFER[11]);
		mpu_9250_gyro.Z = (short)(ACCL_GYRO_BUFFER[12]<<8|ACCL_GYRO_BUFFER[13]);
}
void Read_Mag(){
	
	u8 x_axis,y_axis,z_axis;
	u8 BUF[6];
	x_axis = Read_Byte(MAG_DEVICE,MAG_ASAX);
	y_axis = Read_Byte(MAG_DEVICE,MAG_ASAY);
	z_axis = Read_Byte(MAG_DEVICE,MAG_ASAZ);
	
	if((Read_Byte(MAG_DEVICE,MAG_STATUS_1)&0x02)==0)//data ready
	{
		
		 BUF[0]=Read_Byte(MAG_DEVICE,MAG_XOUT_L); //Low data	
		 if((Read_Byte(MAG_DEVICE,MAG_STATUS_2)&0x08)==1)// data reading end register & check Magnetic sensor overflow occurred 
		 {
			 BUF[0]=Read_Byte(MAG_DEVICE,MAG_XOUT_L);//reload data
		 } 
		 BUF[1]=Read_Byte(MAG_DEVICE,MAG_XOUT_H); //High data	
		 if((Read_Byte(MAG_DEVICE,MAG_STATUS_2)&0x08)==1)// data reading end register
		 {
			 BUF[1]=Read_Byte(MAG_DEVICE,MAG_XOUT_H);
		 }
		 mpu_9250_mag.X=((BUF[1]<<8)|BUF[0]);//*(((x_axis-128)>>8)+1);		
		 
		
			BUF[2]=Read_Byte(MAG_DEVICE,MAG_YOUT_L); //Low data	
		 if((Read_Byte(MAG_DEVICE,MAG_STATUS_2)&0x08)==1)// data reading end register
		 {
			 BUF[2]=Read_Byte(MAG_DEVICE,MAG_YOUT_L);
		 }		 
		 BUF[3]=Read_Byte(MAG_DEVICE,MAG_YOUT_H);//High data	
		 if((Read_Byte(MAG_DEVICE,MAG_STATUS_2)&0x08)==1)// data reading end register
		 {
			 BUF[3]=Read_Byte(MAG_DEVICE,MAG_YOUT_H);
	   }
		 mpu_9250_mag.Y=((BUF[3]<<8)|BUF[2]);//*(((y_axis-128)>>8)+1);	
		 
	
		 BUF[4]=Read_Byte(MAG_DEVICE,MAG_ZOUT_L); //Low data	
		 if((Read_Byte(MAG_DEVICE,MAG_STATUS_2)&0x08)==1)// data reading end register
		 {
			 BUF[4]=Read_Byte(MAG_DEVICE,MAG_ZOUT_L);
		 }	 
		 BUF[5]=Read_Byte(MAG_DEVICE,MAG_ZOUT_H);; //High data	
		 if((Read_Byte(MAG_DEVICE,MAG_STATUS_2)&0x08)==1)// data reading end register
		 {
			 BUF[5]=Read_Byte(MAG_DEVICE,MAG_ZOUT_H);
		 }
		  mpu_9250_mag.Z=((BUF[5]<<8)|BUF[4]);//*(((z_axis-128)>>8)+1);	
	}					      
	
}
void IMUupdate(float gx, float gy, float gz, float ax, float ay, float az){
	float norm;
  float vx, vy, vz;// wx, wy, wz;
  float ex, ey, ez;
	
	float q0q0 = q0*q0;
  float q0q1 = q0*q1;
  float q0q2 = q0*q2;
  float q1q1 = q1*q1;
  float q1q3 = q1*q3;
  float q2q2 = q2*q2;
  float q2q3 = q2*q3;
  float q3q3 = q3*q3;
	
	if(ax*ay*az==0)return;
	
	norm = sqrt(ax*ax + ay*ay + az*az);
	ax = ax / norm;
	ay = ay / norm;
	az = az / norm;
	
	vx = 2*(q1q3 - q0q2);
	vy = 2*(q0q1 + q2q3);
	vz = q0q0 - q1q1 - q2q2 + q3q3;
	
	ex = ay*vz - az*vy ;
	ey = az*vx - ax*vz ;
	ez = ax*vy - ay*vx ;
	
	exInt = exInt + ex * Ki;
	eyInt = eyInt + ey * Ki;
	ezInt = ezInt + ez * Ki;
	
	gx = gx + Kp*ex + exInt;
	gy = gy + Kp*ey + eyInt;
	gz = gz + Kp*ez + ezInt;
	
	q0 = q0 + (-q1*gx - q2*gy - q3*gz)*halfT;
	q1 = q1 + (q0*gx + q2*gz - q3*gy)*halfT;
	q2 = q2 + (q0*gy - q1*gz + q3*gx)*halfT;
	q3 = q3 + (q0*gz + q1*gy - q2*gx)*halfT;
	
	norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
	q0 = q0 / norm;
	q1 = q1 / norm;
	q2 = q2 / norm;
	q3 = q3 / norm;
	
	q_angle.Z = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2*q2 - 2 * q3* q3 + 1)* 57.3;
	q_angle.Y = asin(-2 * q1 * q3 + 2 * q0* q2)* 57.3;
	q_angle.X = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1)* 57.3;
	
	//if(q_angle.Y>90||q_angle.Y<-90)
	//{
	//	if(q_angle.Y>0)q_angle.Y=180-q_angle.Y;
	//	if(q_angle.Y<0)q_angle.Y=-(180+q_angle.Y);
	//}
}
 void AHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz) {
           float norm;
           float hx, hy, hz, bx, bz;
           float vx, vy, vz, wx, wy, wz; 
           float ex, ey, ez;
 
           // auxiliary variables to reduce number of repeated operations
           float q0q0 = q0*q0;
           float q0q1 = q0*q1;
           float q0q2 = q0*q2;
           float q0q3 = q0*q3;
           float q1q1 = q1*q1;
           float q1q2 = q1*q2;
           float q1q3 = q1*q3;
           float q2q2 = q2*q2;
           float q2q3 = q2*q3;
           float q3q3 = q3*q3;
          
           // normalise the measurements
           norm = sqrt(ax*ax + ay*ay + az*az);
           ax = ax / norm;
           ay = ay / norm;
           az = az / norm;
           norm = sqrt(mx*mx + my*my + mz*mz);
           mx = mx / norm;
           my = my / norm;
           mz = mz / norm;
          
           // compute reference direction of magnetic field
           hx = 2*mx*(0.5 - q2q2 - q3q3) + 2*my*(q1q2 - q0q3) + 2*mz*(q1q3 + q0q2);
           hy = 2*mx*(q1q2 + q0q3) + 2*my*(0.5 - q1q1 - q3q3) + 2*mz*(q2q3 - q0q1);
           hz = 2*mx*(q1q3 - q0q2) + 2*my*(q2q3 + q0q1) + 2*mz*(0.5 - q1q1 -q2q2);        
           bx = sqrt((hx*hx) + (hy*hy));
           bz = hz;
          
					// estimated direction of gravity and magnetic field (v and w)
           vx = 2*(q1q3 - q0q2);
           vy = 2*(q0q1 + q2q3);
           vz = q0q0 - q1q1 - q2q2 + q3q3;
           wx = 2*bx*(0.5 - q2q2 - q3q3) + 2*bz*(q1q3 - q0q2);
           wy = 2*bx*(q1q2 - q0q3) + 2*bz*(q0q1 + q2q3);
           wz = 2*bx*(q0q2 + q1q3) + 2*bz*(0.5 - q1q1 - q2q2); 
          
					// error is sum ofcross product between reference direction of fields and directionmeasured by sensors 
           ex = (ay*vz - az*vy) + (my*wz - mz*wy);
           ey = (az*vx - ax*vz) + (mz*wx - mx*wz);
           ez = (ax*vy - ay*vx) + (mx*wy - my*wx);
          
           // integral error scaled integral gain
           exInt = exInt + ex*Ki* (1.0f / sampleFreq);
           eyInt = eyInt + ey*Ki* (1.0f / sampleFreq);
           ezInt = ezInt + ez*Ki* (1.0f / sampleFreq);
           // adjusted gyroscope measurements
           gx = gx + Kp*ex + exInt;
           gy = gy + Kp*ey + eyInt;
           gz = gz + Kp*ez + ezInt;
          
           // integrate quaternion rate and normalize
           q0 = q0 + (-q1*gx - q2*gy - q3*gz)*halfT;
           q1 = q1 + (q0*gx + q2*gz - q3*gy)*halfT;
           q2 = q2 + (q0*gy - q1*gz + q3*gx)*halfT;
           q3 = q3 + (q0*gz + q1*gy - q2*gx)*halfT; 
          
           // normalise quaternion
           norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
           q0 = q0 / norm;
           q1 = q1 / norm;
           q2 = q2 / norm;
           q3 = q3 / norm;
					 
					q_angle.Z = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2*q2 - 2 * q3* q3 + 1)*57.3;
					q_angle.Y = asin(-2 * q1 * q3 + 2 * q0* q2)* 57.3;
					q_angle.X = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1)* 57.3;
}
int main(){
	//u8 ACCL_GYRO_BUFFER[14];
	u8 MAG_BUFFER[6];
	GPIO_config();
	UART_config();
	//Config MPU_9250
	if(Read_Byte(GYR_ACC_DEVICE,WHO_AM_I)==0x71){
		printf("\r\n Start Config MPU_9250 \r\n");
		Write_Byte(GYR_ACC_DEVICE,PWR_MAGMT_1,0x00);
		printf("\r\n Config PWR is %x \r\n",0x00);
		Write_Byte(GYR_ACC_DEVICE,INT_PIN_CONFIG,0x02);
		printf("\r\n Config by pass_mode \r\n");
		if(Read_Byte(MAG_DEVICE,MAG_ID_ADDR)==0x48){
			printf("\r\n Magnetometer Check Device is OK! \r\n");
			//Config Magnetometer
			Write_Byte(MAG_DEVICE,MAG_CTR2_ADDR,0x01);
			Write_Byte(MAG_DEVICE,MAG_CTR1_ADDR,0x12);   //设置磁力计 连续测量 16bit输出 
			printf("\r\n Config Magnetometer is %x \r\n",0x12);
			
		}else{
			printf("\r\n Magnetometer Check Device is Error!  Read Magnetometer Value will be failed! \r\n");
		}
		//Config Configuration Register
		Write_Byte(GYR_ACC_DEVICE,CONFIG,0x07);   //设置低通滤波率 3600HZ  
		printf("\r\n Config Configuration Register is %x \r\n",0x07);		
		//Config Sample Rate Divider
		Write_Byte(GYR_ACC_DEVICE,SIMPLE_DIV,0x07);     //Simple Rate = Fs/(1+div)
		printf("\r\n Config Sample Rate Divider is %x \r\n",0x07);
		//Config Gyroscope Configuration
		Write_Byte(GYR_ACC_DEVICE,CONFIG_GYRO,0x18);  //设置陀螺仪满量程+2000dps    fchoice=0x00
		printf("\r\n Config Gyroscope Configuration is %x \r\n",0x18);
		//Config Accelerometer Configuration
		Write_Byte(GYR_ACC_DEVICE,CONFIG_ACCL_1,0x18); //设置加速度计满量程+16g
		printf("\r\n Config Accelerometer Configuration is %x \r\n",0x18);
		//Write_Byte(GYR_ACC_DEVICE,CONFIG_ACCL_1,0x00); //设置加速度计满量程+2g
		//printf("\r\n Config Accelerometer Configuration is %x \r\n",0x00);
		//Config Accelerometer Configuration_1
		Write_Byte(GYR_ACC_DEVICE,CONFIG_ACCL_2,0x08); //加速度计高通滤波频率 0x08  (1.13kHz)
		printf("\r\n Config Accelerometer Configuration_1 is %x \r\n",0x08);
  }else{
		printf("\r\n Device ID is Error! \r\n");
	}
	while(1){
		float gx,gy,gz,ax,ay,az,mx,my,mz;
		//Read ACCL_GYRO
		Read_Accl_Gyro();
		//Read_ByteBuffer(GYR_ACC_DEVICE,ACCL_REGISTER,ACCL_GYRO_BUFFER,14);
		//mpu_9250_accl.X = (short)(ACCL_GYRO_BUFFER[0]<<8|ACCL_GYRO_BUFFER[1]);
		//mpu_9250_accl.Y = (short)(ACCL_GYRO_BUFFER[2]<<8|ACCL_GYRO_BUFFER[3]);
		//mpu_9250_accl.Z = (short)(ACCL_GYRO_BUFFER[4]<<8|ACCL_GYRO_BUFFER[5]);
		//mpu_9250_gyro.X = (short)(ACCL_GYRO_BUFFER[8]<<8|ACCL_GYRO_BUFFER[9]);
		//mpu_9250_gyro.Y = (short)(ACCL_GYRO_BUFFER[10]<<8|ACCL_GYRO_BUFFER[11]);
		//mpu_9250_gyro.Z = (short)(ACCL_GYRO_BUFFER[12]<<8|ACCL_GYRO_BUFFER[13]);
		//Read MAG
		Write_Byte(MAG_DEVICE,MAG_CTR2_ADDR,0x01);
		Write_Byte(MAG_DEVICE,MAG_CTR1_ADDR,0x12);
		//while((Read_Byte(MAG_DEVICE,MAG_STATUS_1)&0x01)!=0x00);
		//Read_ByteBuffer(MAG_DEVICE,MAG_REGISTER,MAG_BUFFER,6);
		//mpu_9250_mag.X = (short)(MAG_BUFFER[1]<<8|MAG_BUFFER[0]);
		//mpu_9250_mag.Y = (short)(MAG_BUFFER[3]<<8|MAG_BUFFER[2]);
		//mpu_9250_mag.Z = (short)(MAG_BUFFER[5]<<8|MAG_BUFFER[4]);
		Read_Mag();
		gx = ((float)mpu_9250_gyro.X/16.4)*(PI/180);
		gy = ((float)mpu_9250_gyro.Y/16.4)*(PI/180);
		gz = ((float)mpu_9250_gyro.Z/16.4)*(PI/180);
		//ax = (float)mpu_9250_accl.X/163.8;
		//ay = (float)mpu_9250_accl.Y/163.8;
		//az = (float)mpu_9250_accl.Z/163.8;
		
		ax = (float)mpu_9250_accl.X/2048;
		ay = (float)mpu_9250_accl.Y/2048;
		az = (float)mpu_9250_accl.Z/2048;
		//ax = (float)mpu_9250_mag.X*0.15;
		//ay = (float)mpu_9250_mag.Y*0.15;
		//az = (float)mpu_9250_mag.Z*0.15;
		mx = (float)mpu_9250_mag.X*0.15;
		my = (float)mpu_9250_mag.Y*0.15;
		mz = (float)mpu_9250_mag.Z*0.15;
		AHRSupdate(gx,gy,gz,ax,ay,az,mx,my,mz);
		//IMUupdate(gx,gy,gz,ax,ay,az);
		printf("\r\n ACCL X=%.3f   ,Y=%.3f    ,Z=%.3f \r\n",(float)mpu_9250_accl.X/163.8,(float)mpu_9250_accl.Y/163.8,(float)mpu_9250_accl.Z/163.8);  //16384->1g  分辨率缩小100=>  163.84
		printf("\r\n GYRO X=%.3f   ,Y=%.3f    ,Z=%.3f \r\n",(float)mpu_9250_gyro.X/16.4,(float)mpu_9250_gyro.Y/16.4,(float)mpu_9250_gyro.Z/16.4);     //16.4->1d
		printf("\r\n MAG  X=%.3f   ,Y=%.3f    ,Z=%.3f \r\n",(float)mpu_9250_mag.X*0.15,(float)mpu_9250_mag.Y*0.15,(float)mpu_9250_mag.Z*0.15);        //0.15
		printf("\r\n Yaw=%.3f   ,Pitch=%.3f    ,Roll=%.3f \r\n",(float)q_angle.Z,(float)q_angle.Y,(float)q_angle.X);
		printf("\r\n <----------------------------------------> \r\n");
		//delay_5us(10000);
	}
	return 0;
}
/*int main(){
	  short a[3];              // [x, y, z]            accel vector
    short g[3];              // [x, y, z]            gyro vector
    long _q[4];
	  int r,rev;
	  u32 result;
	  uint8_t devStatus; 
	  short sensors;
	  unsigned char fifoCount;
	
		GPIO_config();
		UART_config();
	  result = mpu_init();
	  if(!result){
			while(mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL));
			while(mpu_set_gyro_fsr(2000));
			while(mpu_set_accel_fsr(2));
			mpu_get_power_state(&devStatus);
	    printf(devStatus ? " \r\n MPU6050 connection successful \r\n" : "\r\n MPU6050 connection failed %u \r\n",devStatus);
			while(mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL));
			while(dmp_load_motion_driver_firmware());
		  while(mpu_set_dmp_state(1));
      while(dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT|DMP_FEATURE_SEND_RAW_ACCEL|DMP_FEATURE_SEND_CAL_GYRO|DMP_FEATURE_GYRO_CAL));
      while(dmp_set_fifo_rate(100));
			while(mpu_reset_fifo());
			do {
					delay_ms(1000/100);
					r=dmp_read_fifo(g,a,_q,NULL,&sensors,&fifoCount);
			} while (r!=0 || fifoCount<5);
      
		}
		while(1){
					short gyro[3], accel_short[3], sensors;
					unsigned char more;
					long accel[3], quat[4], temperature;
			    //delay_ms(1000/100);
					rev = dmp_read_fifo(gyro,accel_short,quat,NULL,&sensors,&more);
					if(rev != 0)
					{
							printf("\r\n dmp_read_fifo error.rev = %d\r\n",rev);
					}
					if(sensors & INV_WXYZ_QUAT)
					{
							q0 = quat[0] / q30;
							q1 = quat[1] / q30;
							q2 = quat[2] / q30;
							q3 = quat[3] / q30;
			
#if 1
							printf("\r\n quat[0] = %ld,quat[1] = %ld\n,quat[2] = %ld,quat[3] = %ld \r\n",quat[0],quat[1],quat[2],quat[3]);
							printf("\r\n q0 = %.1f,q1 = %.1f,q2 = %.1f,q3 = %.1f \r\n\n",q0,q1,q2,q3);
#endif
							q_angle.Y = asin(-2*q1*q3 + 2*q0*q2)*57.3;
							q_angle.X = atan2(2*q2*q3 + 2*q0*q1,-2*q1*q1 - 2*q2*q2 +1) * 57.3;
							q_angle.Z = atan2(2*(q1*q2 + q0*q3),q0*q0 +  q1*q1 - q2*q2 - q3*q3)*57.3;
							printf("\r\n Pitch :%.1f ",(float)q_angle.Y);
							printf("Roll :%.1f  ",(float)q_angle.X);
							printf("Yaw :%.1f  ",(float)q_angle.Z);
							printf("\r\n");
				} 
		}
		return 0;
}*/
