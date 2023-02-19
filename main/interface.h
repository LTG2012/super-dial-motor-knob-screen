#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "RGB.h"
#include "BleKeyboard.h"
#include "OneButton.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "USB.h"
#include "USBHID.h"
#include "driver/rtc_io.h"
#include "display_task.h"
#include "Watchdog.h"
USBHID HID;

WATCHDOG Watchdog;//看门狗对象.


static const uint8_t report_descriptor[] = {  // 8 axis
  // DIAL
  0x05, 0x01,
  0x09, 0x0e,
  0xa1, 0x01,
  0x85, 10,
  0x05, 0x0d,
  0x09, 0x21,
  0xa1, 0x00,
  0x05, 0x09,
  0x09, 0x01,
  0x95, 0x01,
  0x75, 0x01,
  0x15, 0x00,
  0x25, 0x01,
  0x81, 0x02,
  0x05, 0x01,
  0x09, 0x37,
  0x95, 0x01,
  0x75, 0x0f,
  0x55, 0x0f,
  0x65, 0x14,
  0x36, 0xf0, 0xf1,
  0x46, 0x10, 0x0e,
  0x16, 0xf0, 0xf1,
  0x26, 0x10, 0x0e,
  0x81, 0x06,
  0xc0,
  0xc0
};

class CustomHIDDevice : public USBHIDDevice {
public:
  CustomHIDDevice(void) {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
      HID.addDevice(this, sizeof(report_descriptor));
    }
  }

  void begin(void) {
    HID.begin();
  }
  void end(void) {
    HID.end();
  }
  uint16_t _onGetDescriptor(uint8_t *buffer) {
    memcpy(buffer, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
  }

  bool send(uint8_t keys) {

    uint8_t dial_report[2];
    dial_report[0] = keys;
    dial_report[1] = 0;
    if (keys == DIAL_L || keys == DIAL_L_F)
      dial_report[1] = 0xff;
    return HID.SendReport(10, dial_report, 2);
  }
};

CustomHIDDevice Device;


const char *ServerName = "ESP32-Reuleaux-RGB";
char mac_tmp[6];
const char *ssid = mac_tmp;
const char *password = "";
WebServer server(80);


BleKeyboard bleKeyboard;
void AutoWifiConfig();


static KnobConfig configs[] = {
  // int32_t num_positions;
  // int32_t position;
  // float position_width_radians;
  // float detent_strength_unit;
  // float endstop_strength_unit;
  // float snap_point;

  {
    0,
    0,
    10 * PI / 180,
    0,
    1,
    1.1,
  },
  {
    //1设置界面
    0,
    0,
    90 * PI / 180,
    1,
    1,
    0.55,
  },
  {
    //2设置界面 offset调整力度
    0,
    0,
    22.5 * PI / 180,
    1,
    1,
    0.55,  // Note the snap point is slightly past the midpoint (0.5); compare to normal detents which use a snap point *past* the next value (i.e. > 1)
  },
  {
    //3 主界面
    0,
    0,
    90 * PI / 180,
    1,
    1,
    0.55,  // Note the snap point is slightly past the midpoint (0.5); compare to normal detents which use a snap point *past* the next value (i.e. > 1)
  },
  {
    1,  //4
    0,
    60 * PI / 180,
    0.01,
    0.6,
    1.1,
  },
  {
    256,  //5
    127,
    1 * PI / 180,
    0,
    1,
    1.1,
  },
  {
    //6surface dial
    0,
    0,
    3 * PI / 180,
    1,
    1,
    1.1,
  },
  {
    //7
    100,
    50,
    1 * PI / 180,
    1,
    1,
    1.1,
  },
  {
    //8
    32,
    0,
    8.225806452 * PI / 180,
    0.2,
    1,
    1.1,
  },
};


#define OFF_PIN GPIO_NUM_7
#define OFF_UP_PIN GPIO_NUM_6
#define PUSH_BUTTON GPIO_NUM_5
#define IO_ON_OFF GPIO_NUM_18
OneButton button(PUSH_BUTTON, true, true);
bool offset_flag;
uint32_t interface_time;
uint8_t push_flag, push_states;
uint32_t push_time, push_in_time, push_two_time;
uint8_t dial_flag;
uint8_t power_scale = 50;  // 力度
uint8_t angle_scale = 75;  //角度
uint8_t push_scale = 3;

uint8_t lv_page = 0, lv_adjust_flag = 0;
void doubleclick() {
  push_states = 2;
  Serial.println("doubleclick");
}
void click() {
  push_states = 1;
  Serial.println("click");
}
void longPressStart() {
  push_states = 3;
  Serial.println("longPressStart");
}
void duringLongPress() {
  if (button.isLongPressed()) {
    push_states = 3;
    Serial.print("duringLongPress:");
  }
}
void longPressStop() {
  push_states = 4;
  Serial.println("longPressStop");
}
void power_off() {
  digitalWrite(OFF_PIN, LOW);
  delay(100);
  digitalWrite(OFF_PIN, LOW);
  delay(200);
  digitalWrite(OFF_PIN, HIGH);
  delay(100);
  digitalWrite(OFF_PIN, LOW);
  delay(200);
  digitalWrite(OFF_PIN, HIGH);

  rgb_off();
  digitalWrite(TFT_BLK, LOW);
  sleep_flag = 1;
  motor.disable();

  rtc_gpio_init(TFT_BLK);
  rtc_gpio_init(PUSH_BUTTON);

  rtc_gpio_pullup_dis(TFT_BLK);
  rtc_gpio_pulldown_en(TFT_BLK);

  rtc_gpio_pullup_en(PUSH_BUTTON);
  rtc_gpio_pulldown_dis(PUSH_BUTTON);

  gpio_deep_sleep_hold_en();

  esp_sleep_enable_ext0_wakeup(PUSH_BUTTON, 0);  //1 = High, 0 = Low
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}
void eeprom_read() {
  if (isnan(EEPROM.readUChar(4))) {
    Serial.println("write power");
    EEPROM.writeUChar(4, 50);
    delay(10);
    EEPROM.commit();

  } else {
    if (EEPROM.readUChar(4) > 100) {
      Serial.println("write power1");
      EEPROM.writeUChar(4, 50);
      delay(10);
      EEPROM.commit();
    } else {
      power_scale = EEPROM.readUChar(4);
      Serial.println(power_scale);
    }
  }
  if (isnan(EEPROM.readUChar(8))) {
    Serial.println("write angle");
    EEPROM.writeUChar(8, 75);
    delay(10);
    EEPROM.commit();

  } else {
    if (EEPROM.readUChar(8) > 100) {
      Serial.println("write angle1");
      EEPROM.writeUChar(8, 75);
      delay(10);
      EEPROM.commit();
    } else {
      angle_scale = EEPROM.readUChar(8);
      Serial.println(angle_scale);
    }
  }
  if (isnan(EEPROM.readUChar(12))) {
    Serial.println("write push");
    EEPROM.writeUChar(12, 3);
    delay(10);
    EEPROM.commit();

  } else {
    if (EEPROM.readUChar(12) > 5) {
      Serial.println("write push1");
      EEPROM.writeUChar(12, 3);
      delay(10);
      EEPROM.commit();
    } else {
      push_scale = EEPROM.readUChar(12);
      Serial.println(push_scale);
    }
  }
}
// 长时间休眠
void sleep_time(uint8_t move) {
  static uint16_t time = 0;
  static uint8_t dis_flag = 0;
  if (move) {
    time = 0;
    digitalWrite(TFT_BLK, HIGH);
  }

  time++;
  Serial.println(time);
  // 50为一秒   3000为一分钟
  if (time > 1500) {
    digitalWrite(TFT_BLK, LOW);
  }
  if (time > 15000) {
    power_off();
  }
}
void send_config(uint8_t num) {
  KnobConfig set_config;
  set_config = configs[num];
  set_config.detent_strength_unit = configs[num].detent_strength_unit * (100 - power_scale) * 0.02;
  set_config.position_width_radians = configs[num].position_width_radians * (100 - angle_scale) * 0.04;
  setConfig(set_config);
}
void interface_run(void *parameter) {
  uint16_t img_angle = 0, position_flag = 0, last_img_angle = 0;
  float last_adjusted_angle = 0;
  int16_t last_position = 0;
  uint8_t p;  //当前选择的位置
  KnobConfig set_config;
  KnobState state;

  button.reset();  //清除一下按钮状态机的状态
  button.attachClick(click);
  button.attachDoubleClick(doubleclick);
  button.attachLongPressStart(longPressStart);
  //  button.attachDuringLongPress(duringLongPress);
  button.attachLongPressStop(longPressStop);

  USB.begin();


  pinMode(OFF_UP_PIN, OUTPUT);
  pinMode(OFF_PIN, OUTPUT);
  pinMode(IO_ON_OFF, OUTPUT);
  digitalWrite(IO_ON_OFF, HIGH);
  delay(10);
  digitalWrite(OFF_UP_PIN, LOW);
  delay(10);
  digitalWrite(OFF_PIN, LOW);

  Serial.println("Starting RGB");
  strip.begin();                    // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();                     // Turn OFF all pixels ASAP
  strip.setBrightness(brightness);  // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.println("RGB on");

  eeprom_read();


  send_config(3);
  Serial.println("setConfig");
  Watchdog.begin();//默认定时器0,10秒超时.
  while (1) {
    Watchdog.feed();//喂狗
    sleep_time(0);
    if (xQueueReceive(knob_state_queue_, &state, 0) == pdTRUE) {
      pthread_mutex_lock(&lvgl_mutex);
      float adjusted_sub_position, raw_angle, adjusted_angle;
      
      adjusted_sub_position = state.sub_position_unit * state.config.position_width_radians;
          raw_angle = state.current_position * state.config.position_width_radians;
          adjusted_angle = -(raw_angle + adjusted_sub_position);
          if (adjusted_angle > 0)
            img_angle = (uint16_t)(adjusted_angle * 573) % 3600;
          else
            img_angle = 3600 - (uint16_t)(abs(adjusted_angle) * 573) % 3600;
      // Serial.print(last_adjusted_angle);
      // Serial.print("/");
      // Serial.println(abs(adjusted_angle-last_adjusted_angle));
      if (abs(adjusted_angle-last_adjusted_angle) > 0.05) {
        
            sleep_time(1);
            last_adjusted_angle = adjusted_angle;
          }

      // Serial.print(raw_angle);
      // Serial.print("/");
      // Serial.print(adjusted_angle);
      // Serial.print("/");
      // Serial.print(img_angle);
      // Serial.print("/");
      // Serial.print(last_img_angle);
      // Serial.print("/");
      // Serial.println((uint16_t)(adjusted_angle * 573));
      if (last_position != state.current_position) {
        position_flag = 1;
        sleep_time(1);
        if (state.current_position - last_position > 0)
          dial_flag = 1;
        else
          dial_flag = 2;
      }
      last_position = state.current_position;
      switch (lv_page) {
        case 0:
          adjusted_sub_position = state.sub_position_unit * 90 * PI / 180;
          raw_angle = state.current_position * 90 * PI / 180;
          adjusted_angle = -(raw_angle + adjusted_sub_position);
          if (adjusted_angle > 0)
            img_angle = (uint16_t)(adjusted_angle * 573) % 3600;
          else
            img_angle = 3600 - (uint16_t)(abs(adjusted_angle) * 573) % 3600;
        //   if (abs(img_angle - last_img_angle) > 50) {
        //     lv_img_set_angle(ui_Image0_2, img_angle);
        //     last_img_angle = img_angle;
        //   }
          lv_img_set_angle(ui_Image0_2, img_angle);
            if((img_angle>=0 && img_angle<=450)  ||  (img_angle>3150 && img_angle<=3600))
              p = 0;
            else if((img_angle>450 && img_angle<=1350))
              p = 3;
            else if((img_angle>1350 && img_angle<=2250))
              p = 2;
            else if((img_angle>2250 && img_angle<=3150))
              p = 1;
            switch (p) {
              case 0:
                lv_obj_set_style_outline_width(ui_Image0_0, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
              case 1:
                lv_obj_set_style_outline_width(ui_Image0_0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
              case 2:
                lv_obj_set_style_outline_width(ui_Image0_0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_4, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
              case 3:
                lv_obj_set_style_outline_width(ui_Image0_0, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_3, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_outline_width(ui_Image0_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                break;
            }
          break;
        case 1:

          // adjusted_sub_position = state.sub_position_unit * state.config.position_width_radians;
          // raw_angle = state.current_position * state.config.position_width_radians;
          // adjusted_angle = -(raw_angle + adjusted_sub_position);
          // if (adjusted_angle > 0)
          //   img_angle = (uint16_t)(adjusted_angle * 573) % 3600;
          // else
          //   img_angle = 3600 - (uint16_t)(abs(adjusted_angle) * 573) % 3600;
          // if (abs(img_angle - last_img_angle) > 5) {
          //   lv_img_set_angle(ui_Image2, img_angle);
          //   last_img_angle = img_angle;
          // }
          lv_img_set_angle(ui_Image2, img_angle);
          break;
        case 2:

          switch (lv_adjust_flag)  //--------------------------------------------------------------------------
          {
            case 0:  //力度
              power_scale = state.current_position;
              set_config.position = power_scale;
              break;
            case 1:  //角度
              angle_scale = state.current_position;
              set_config.position = angle_scale;
              break;
            case 2:  //按钮强度
              break;
          }
          lv_meter_set_indicator_value(meter, line_indic, 100 - state.current_position);
          break;
      }
      pthread_mutex_unlock(&lvgl_mutex);
    }

    unsigned long currentMillis = millis();
    //rgb显示
    if (currentMillis - pixelPrevious >= pixelInterval) {  //  Check for expired time
      pixelPrevious = currentMillis;                       //  Run current frame
      switch (rgb_flag) {
        case 0:
          break;
        case 1:
          role_fill();
          break;
        case 2:
          rainbow2();
          break;
      }
    }
    //----------------------旋转部分
    //------------------------按键处理----------------------//
    if (digitalRead(PUSH_BUTTON) == 0) {  //按下
      if (push_flag == 0) {
        push_time = currentMillis;
        push_flag = 1;
      }
      if (currentMillis - push_time > 10) {  //消抖10ms
        if (push_flag == 1) {
          push_flag = 2;
          push_in_time = currentMillis;
          playHaptic(push_scale);
          //           rgb_flag = 1;
          // pixelRoll = 0;
        }
      }
    }
    if (push_flag && digitalRead(PUSH_BUTTON)) {  //松开
      //     rgb_flag = 0;
      // rgb_off();
      push_two_time = currentMillis;
      push_flag = 0;
    }
    // ---- 页面处理
    switch (lv_page) {
      case 0:
        {
          //左旋右旋
          if (dial_flag == 1) {
            strip2();
          } else if (dial_flag == 2) {
            strip3();
          }
          switch (push_states) {
            case 1:  //单击
              if (sleep_flag) {
                digitalWrite(TFT_BLK, HIGH);
                sleep_flag = 0;
                motor.enable();
              }
              break;
            case 2:  //双击切换
              switch (p) {
                case 0:  //surface dial
                  lv_event_send(ui_Button0, LV_EVENT_CLICKED, 0);
                  configs[3].position = state.current_position;
                  send_config(6);
                  lv_page = 1;
                  Serial.println(lv_page);
                  Serial.println("Starting BLE work!");
                  bleKeyboard.begin();
                  Device.begin();
                  break;
                case 1:  //力度设置界面
                  lv_event_send(ui_Button0, LV_EVENT_RELEASED, 0);
                  switch (lv_adjust_flag) {
                    case 0:
                      lv_label_set_text(ui2_Label1, "力度");
                      configs[3].position = state.current_position;
                      set_config = configs[7];
                      set_config.position = power_scale;
                      set_config.detent_strength_unit = configs[7].detent_strength_unit * (100 - power_scale) * 0.02;
                      set_config.position_width_radians = configs[7].position_width_radians * (100 - angle_scale) * 0.04;
                      setConfig(set_config);
                      break;
                    case 1:
                      lv_label_set_text(ui2_Label1, "角度");
                      configs[3].position = state.current_position;
                      set_config = configs[7];
                      set_config.position = angle_scale;
                      set_config.detent_strength_unit = configs[7].detent_strength_unit * (100 - power_scale) * 0.02;
                      set_config.position_width_radians = configs[7].position_width_radians * (100 - angle_scale) * 0.04;
                      setConfig(set_config);
                      break;
                    case 2:
                      lv_label_set_text(ui2_Label1, "按钮强度");
                      break;
                  }


                  lv_page = 2;
                  Serial.println(lv_page);
                  break;
                case 2:         //关机
                  power_off();  //ip5306关机

                  break;
                case 3:  //跳转到设置
                  // motor.disable();
                  EEPROM.writeFloat(0, motor.zero_electric_angle);
                  delay(1);
                  EEPROM.commit();
                  lv_event_send(ui_Button0, LV_EVENT_SHORT_CLICKED, 0);
                  configs[3].position = state.current_position;
                  send_config(1);
                  lv_page = 3;
                  Serial.println(lv_page);
                  char str[10];
                  sprintf(str, "%.2f", motor.zero_electric_angle);
                  lv_label_set_text(ui_Label3, str);
                  AutoWifiConfig();

                  break;
              }
              break;
            case 3:  //长按
              rgb_flag = 2;
              break;
            case 4:  //松开
              rgb_flag = 0;
              rgb_off();
              break;
            default:
              break;
          }
        }

        break;
      case 1:  //页面1
        {
          //左旋右旋
          if (dial_flag == 1) {
            strip2();
          } else if (dial_flag == 2) {
            strip3();
          }
          //如果蓝牙连接成功
          if (bleKeyboard.isConnected()) {
            if (dial_flag == 1) {
              bleKeyboard.sendDialReport(DIAL_L);
            } else if (dial_flag == 2) {
              bleKeyboard.sendDialReport(DIAL_R);
            }
          } else {
            if (dial_flag == 1) {
              Device.send(DIAL_L);
            } else if (dial_flag == 2) {
              Device.send(DIAL_R);
            }
          }
          switch (push_states) {
            case 1:  //单击

              if (bleKeyboard.isConnected()) {
                bleKeyboard.sendDialReport(DIAL_PRESS);
                delay(50);
                bleKeyboard.sendDialReport(DIAL_RELEASE);
              } else {
                Device.send(DIAL_PRESS);
                delay(50);
                Device.send(DIAL_RELEASE);
              }

              break;
            case 2:  //双击切换
              lv_event_send(ui_Button1, LV_EVENT_CLICKED, 0);
              send_config(3);
              lv_page = 0;
              Serial.println(lv_page);

              if (bleKeyboard.isConnected()) {
                bleKeyboard.sendDialReport(DIAL_RELEASE);

              } else
                Device.send(DIAL_RELEASE);

              bleKeyboard.end();
              Device.end();
              break;
            case 3:  //长按
              rgb_flag = 2;
              if (bleKeyboard.isConnected()) {
                bleKeyboard.sendDialReport(DIAL_PRESS);
                delay(500);
                bleKeyboard.sendDialReport(DIAL_RELEASE);
              } else {
                Device.send(DIAL_PRESS);
                delay(500);
                Device.send(DIAL_RELEASE);
              }
              break;
            case 4:  //松开
              rgb_flag = 0;
              rgb_off();
              Serial.println("DIAL_RELEASE");

              if (bleKeyboard.isConnected())
                bleKeyboard.sendDialReport(DIAL_RELEASE);
              else
                Device.send(DIAL_RELEASE);
              break;
            default:
              break;
          }
        }

        break;
      case 2:
        {
          if (dial_flag == 1) {
            set_config.detent_strength_unit = configs[7].detent_strength_unit * (100 - power_scale) * 0.02;
            set_config.position_width_radians = configs[7].position_width_radians * (100 - angle_scale) * 0.04;
            if (millis() - interface_time > 10) {
              interface_time = millis();
              setConfig(set_config);
            }
            strip2();
          } else if (dial_flag == 2) {
            set_config.detent_strength_unit = configs[7].detent_strength_unit * (100 - power_scale) * 0.02;
            set_config.position_width_radians = configs[7].position_width_radians * (100 - angle_scale) * 0.04;
            if (millis() - interface_time > 10) {
              interface_time = millis();
              setConfig(set_config);
            }
            strip3();
          }
          switch (push_states) {
            case 1:  //单击
              lv_adjust_flag++;
              if (lv_adjust_flag > 1)
                lv_adjust_flag = 0;
              switch (lv_adjust_flag) {
                case 0:
                  lv_label_set_text(ui2_Label1, "力度");
                  configs[3].position = state.current_position;
                  set_config = configs[7];
                  set_config.position = power_scale;
                  set_config.detent_strength_unit = configs[7].detent_strength_unit * (100 - power_scale) * 0.02;
                  set_config.position_width_radians = configs[7].position_width_radians * (100 - angle_scale) * 0.04;
                  setConfig(set_config);
                  break;
                case 1:
                  lv_label_set_text(ui2_Label1, "角度");
                  configs[3].position = state.current_position;
                  set_config = configs[7];
                  set_config.position = angle_scale;
                  set_config.detent_strength_unit = configs[7].detent_strength_unit * (100 - power_scale) * 0.02;
                  set_config.position_width_radians = configs[7].position_width_radians * (100 - angle_scale) * 0.04;
                  setConfig(set_config);
                  break;
                case 2:
                  lv_label_set_text(ui2_Label1, "按钮强度");
                  break;
              }
              break;
            case 2:  //双击切换
              EEPROM.writeUChar(4, power_scale);
              EEPROM.commit();
              Serial.println(power_scale);

              EEPROM.writeUChar(8, angle_scale);
              EEPROM.commit();
              Serial.println(angle_scale);

              lv_event_send(ui_Button2, LV_EVENT_CLICKED, 0);
              configs[7].position = state.current_position;
              send_config(3);
              lv_page = 0;
              break;
            case 3:  //长按
              rgb_flag = 2;
              break;
            case 4:  //松开
              rgb_flag = 0;
              rgb_off();
              Serial.println("DIAL_RELEASE");
              break;
            default:
              break;
          }
        }
        break;
      case 3:
        {
          server.handleClient();
          if (dial_flag == 1) {
            if (offset_flag) {
              motor.zero_electric_angle = motor.zero_electric_angle - 0.01;
              char str[10];
              sprintf(str, "%.2f", motor.zero_electric_angle);
              lv_label_set_text(ui_Label3, str);
            }
            strip2();
          } else if (dial_flag == 2) {
            if (offset_flag) {
              motor.zero_electric_angle = motor.zero_electric_angle + 0.01;
              char str[10];
              sprintf(str, "%.2f", motor.zero_electric_angle);
              lv_label_set_text(ui_Label3, str);
            }
            strip3();
          }
          switch (push_states) {
            case 1:  //单击
              if (offset_flag) {
                offset_flag = 0;
                lv_obj_set_style_text_color(ui_Label3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                EEPROM.writeFloat(0, motor.zero_electric_angle);
                EEPROM.commit();
                send_config(1);
              } else {
                offset_flag = 1;
                lv_obj_set_style_text_color(ui_Label3, lv_color_hex(0xFF4B4B), LV_PART_MAIN | LV_STATE_DEFAULT);
                send_config(2);
              }
              break;
            case 2:  //双击切换
              offset_flag = 0;
              lv_obj_set_style_text_color(ui_Label3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
              EEPROM.writeFloat(0, motor.zero_electric_angle);
              EEPROM.commit();
              lv_event_send(ui_Button2, LV_EVENT_CLICKED, 0);
              send_config(3);
              lv_page = 0;
              WiFi.disconnect();
              WiFi.mode(WIFI_OFF);
              // motor.enable();
              break;
            case 3:  //长按
              EEPROM.writeFloat(0, 100);
              EEPROM.commit();
              delay(10);
              ESP.restart();
              rgb_flag = 2;
              break;
            case 4:  //松开
              rgb_flag = 0;
              rgb_off();
              Serial.println("DIAL_RELEASE");
              break;
            default:
              break;
          }
        }
        break;
    }
    push_states = 0;
    dial_flag = 0;
    button.tick();

    vTaskDelay(20);
  }
}
const char *serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='/update' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";
void AutoWifiConfig() {
  //wifi初始化
  sprintf(mac_tmp, "%02X\r\n", (uint32_t)(ESP.getEfuseMac() >> (24)));
  sprintf(mac_tmp, "ESP32-%c%c%c%c%c%c", mac_tmp[4], mac_tmp[5], mac_tmp[2], mac_tmp[3], mac_tmp[0], mac_tmp[1]);

  WiFi.mode(WIFI_AP);
  while (!WiFi.softAP(ssid, password)) {};  //启动AP
  Serial.println("AP启动成功");
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  byte mac[6];
  WiFi.macAddress(mac);
  WiFi.setHostname(ServerName);
  Serial.printf("macAddress 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  /*use mdns for host name resolution*/
  if (!MDNS.begin(ssid)) {  //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on(
    "/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    },
    []() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {  //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });
  server.begin();
}
#endif