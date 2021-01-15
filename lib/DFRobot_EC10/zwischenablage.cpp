
// void DFRobot_EC10::ecCalibration(byte mode)
// {
//     char *receivedBufferPtr;
//     static boolean ecCalibrationFinish = 0;
//     static boolean enterCalibrationFlag = 0;
//     static float rawECsolution;
//     float KValueTemp;
//     switch(mode)
//     {
//       case 0:
//       if(enterCalibrationFlag)
//          Serial.println(F(">>>Command Error<<<"));
//       break;
      
//       case 1:
//       enterCalibrationFlag = 1;
//       ecCalibrationFinish = 0;
//       Serial.println();
//       Serial.println(F(">>>Enter Calibration Mode<<<"));
//       Serial.println(F(">>>Please put the probe into the 12.88ms/cm buffer solution<<<"));
//       Serial.println();
//       break;
     
//       case 2:
//       if(enterCalibrationFlag)
//       {
//           if((this->_ecvalueRaw>6)&&(this->_ecvalueRaw<18))  //recognize 12.88ms/cm buffer solution
//           {
//             rawECsolution = 12.9*(1.0+0.0185*(this->_temperature-25.0));  //temperature compensation
//           }
//           else{
//             Serial.print(F(">>>Buffer Solution Error<<<   "));
//             ecCalibrationFinish = 0;
//           }
            
//           //Serial.print("Kvaluetemp");
//           //Serial.println(KValueTemp);
//           if((KValueTemp>0.5) && (KValueTemp<1.5))
//           {
//               Serial.println();
//               Serial.print(F(">>>Successful,K:"));
//               Serial.print(KValueTemp);
//               Serial.println(F(", Send EXIT to Save and Exit<<<"));
       
                
      
//               ecCalibrationFinish = 1;
//           }
//           else{
//             Serial.println();
//             Serial.println(F(">>>Failed,Try Again<<<"));
//             Serial.println();
//             ecCalibrationFinish = 0;
//           }        
//       }
//       break;

//         case 3:
//         if(enterCalibrationFlag)
//         {
//             Serial.println();
//             if(ecCalibrationFinish)
//             {   
//               if((this->_ecvalueRaw>6)&&(this->_ecvalueRaw<18)) 
//               {
//                  EEPROM_write(KVALUEADDR, this->_kvalue);
//                  Serial.print(F(">>>Calibration Successful"));
//               }
      
              
//             }
//             else Serial.print(F(">>>Calibration Failed"));       
//             Serial.println(F(",Exit Calibration Mode<<<"));
//             Serial.println();
//             ecCalibrationFinish = 0;
//             enterCalibrationFlag = 0;
//         }
//         break;
//     }
// }
