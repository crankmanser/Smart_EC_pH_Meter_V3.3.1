#include "Display.h"
#include "globals.h"

// --- Private variables for the Display module ---
static char currentScrollingItemText[64];
static int scrollTextOffset = 0;
static unsigned long lastScrollUpdateTime = 0;
static int scrollingTextPixelWidth = 0;
static unsigned long lastAnimTime = 0;
static int idleAnimPhase = 0;
static bool errorBlinkState = false;
static int calibAnimPhase = 0;
static int measuringAnimPhase = 0;

// --- Constants ---
const int SCROLL_DELAY_MS = 300;
const int SCROLL_CHAR_STEP = 1;
const int STATUS_ICON_AREA_WIDTH = 20;
const int defaultLabelHeight = 10;
const int CONTENT_START_X_OLED3 = STATUS_ICON_AREA_WIDTH + 3;
const int SYSTEM_STATUS_BAR_HEIGHT_OLED3 = 10;


// Forward declarations for functions used only inside this .cpp file
void tcaSelect(uint8_t ch);
void drawWiFiIcon(Adafruit_SSD1306& d, int x, int y, int s);
void drawBluetoothIcon(Adafruit_SSD1306& d, int x, int y, bool c);
void displayDeviceModeIcon(Adafruit_SSD1306& d, int x, int y);
void drawOLED3StatusIcons(Adafruit_SSD1306& d);
void drawSystemStatusBarOLED3_ui(Adafruit_SSD1306& d);

// =======================================================
// IMPLEMENTATIONS
// =======================================================


// --- From Sect 5: Icon Drawing & Battery Calculation Functions ---
int calculateBatteryLevel(float v) { if(v>=8.0f)return 4;if(v>=7.4f)return 3;if(v>=6.8f)return 2;if(v>=6.2f)return 1;return 0;}
float estimateBatteryPercentage(float v) { const float minV=6.2f,maxV=8.2f;if(v>=maxV)return 100.0f;if(v<=minV)return 0.0f;return((v-minV)/(maxV-minV))*100.0f;}
void drawWiFiIcon(Adafruit_SSD1306& d,int x,int y,int s){if(s<0)s=0;if(s>4)s=4;if(s==0){d.setTextSize(1);d.setCursor(x,y+7);d.print("W0");return;}d.drawPixel(x+4,y+7,1);if(s>=1){d.drawPixel(x+3,y+5,1);d.drawPixel(x+4,y+5,1);d.drawPixel(x+5,y+5,1);}if(s>=2){d.drawPixel(x+2,y+3,1);d.drawPixel(x+3,y+3,1);d.drawPixel(x+4,y+3,1);d.drawPixel(x+5,y+3,1);d.drawPixel(x+6,y+3,1);}if(s>=3){d.drawPixel(x+1,y+1,1);d.drawPixel(x+2,y+1,1);d.drawPixel(x+3,y+1,1);d.drawPixel(x+4,y+1,1);d.drawPixel(x+5,y+1,1);d.drawPixel(x+6,y+1,1);d.drawPixel(x+7,y+1,1);}if(s>=4){if(y>0){d.drawPixel(x+0,y,1);d.drawPixel(x+8,y,1);}else{d.drawPixel(x+0,0,1);d.drawPixel(x+8,0,1);}}}
void drawBluetoothIcon(Adafruit_SSD1306& d,int x,int y,bool c){d.setTextSize(1);d.setCursor(x,y);if(c)d.print("BT+");else d.print("bt-");}
void drawBatteryIcon(Adafruit_SSD1306& d,int x,int y,int l){int bW=12,bH=7,tW=1,tH=3,brdW=1;if(l<0)l=0;if(l>4)l=4;d.drawRect(x,y,bW,bH,1);d.fillRect(x+bW,y+(bH-tH)/2,tW,tH,1);int sHW=bW-(2*brdW),sW=2,sp=1;int sXS=x+brdW+sp,sY=y+brdW,sH=bH-(2*brdW);for(int i=0;i<4;++i){int cSX=sXS+(i*(sW+sp));if(cSX+sW>x+bW-brdW)break;if(i<l)d.fillRect(cSX,sY,sW,sH,1);}}
void drawIdleModeAnimation(Adafruit_SSD1306&d,int x,int y){d.setTextSize(1);d.fillRect(x,y,18,8,0);d.setCursor(x,y);switch(idleAnimPhase){case 0:d.print("   ");break;case 1:d.print(".  ");break;case 2:d.print(".. ");break;case 3:d.print("...");break;}}
void drawErrorModeAnimation(Adafruit_SSD1306&d,int x,int y){d.setTextSize(1);d.fillRect(x,y,18,8,0);d.setCursor(x,y);if(errorBlinkState)d.print(" ? ");else d.print("   ");}
void drawCalibrationModeAnimation(Adafruit_SSD1306&d,int x,int y){d.setTextSize(1);d.fillRect(x,y,18,8,0);d.setCursor(x,y);switch(calibAnimPhase){case 0:d.print("<  ");break;case 1:d.print(" <-");break;case 2:d.print("<->");break;case 3:d.print(" ->");break;}}
void drawMeasuringModeAnimation(Adafruit_SSD1306&d,int x,int y){d.setTextSize(1);d.fillRect(x,y,18,8,0);d.setCursor(x,y);if(measuringAnimPhase==0)d.print("+/-");else d.print("-/+");}
void displayDeviceModeIcon(Adafruit_SSD1306& d, int x, int y){d.setTextSize(1);int w=18;if(currentDeviceStatus==STATUS_BOOTING||currentDeviceStatus==STATUS_SAVING||currentDeviceStatus==STATUS_SD_ERROR)w=24;d.fillRect(x,y,w,8,0);d.setCursor(x,y);switch(currentDeviceStatus){case STATUS_IDLE:drawIdleModeAnimation(d,x,y);break;case STATUS_ERROR:drawErrorModeAnimation(d,x,y);break;case STATUS_CALIBRATING:drawCalibrationModeAnimation(d,x,y);break;case STATUS_MEASURING:drawMeasuringModeAnimation(d,x,y);break;case STATUS_BOOTING:d.print("Boot");break;case STATUS_SAVING:d.print("Save");break;case STATUS_SD_ERROR:d.print("SD!");break;default:d.print("???");break;}}


// --- From Sect 6: Core Display Helper Functions ---
void drawTruncatedText(Adafruit_SSD1306& disp,int x,int y_top,int mW,const char* ft,bool inv){if(!ft)return;disp.setTextSize(1);int16_t xb,yb;uint16_t wf,ht;disp.getTextBounds("Tg",0,0,&xb,&yb,&wf,&ht);char b[128];strncpy(b,ft,127);b[127]=0;disp.getTextBounds(b,0,0,&xb,&yb,&wf,&ht);if(wf>mW){int ac=0,cw=0;const char*el="...";uint16_t ew,eh;disp.getTextBounds(el,0,0,&xb,&yb,&ew,&eh);for(int i=0;b[i]!=0;++i){char tc[2]={b[i],0};uint16_t cw_d,ch_d;disp.getTextBounds(tc,0,0,&xb,&yb,&cw_d,&ch_d);if(cw+cw_d+ew<=mW){cw+=cw_d;ac++;}else break;}if(ac>0&&ac<strlen(ft)){b[ac]=0;strncat(b,el,127-strlen(b));}else if(ac==0&&ew<=mW)strncpy(b,el,127);else if(ac==0)b[0]=0;}int tdyc=y_top+(8-ht)/2;if(inv){disp.fillRect(x,y_top,mW,8,1);disp.setTextColor(0);}else{disp.fillRect(x,y_top,mW,8,0);disp.setTextColor(1);}disp.setCursor(x,tdyc);disp.print(b);if(inv)disp.setTextColor(1);}
void drawScrollingText(Adafruit_SSD1306& disp,int x,int y_top,int mW,const char* ft,bool inv){if(!ft)return;disp.setTextSize(1);int16_t xb,yb;uint16_t wf,ht;disp.getTextBounds("Tg",0,0,&xb,&yb,&wf,&ht);disp.getTextBounds(ft,0,0,&xb,&yb,&wf,&ht);if(wf<=mW){drawTruncatedText(disp,x,y_top,mW,ft,inv);isTextScrollingActive=false;return;}isTextScrollingActive=true;if(strncmp(currentScrollingItemText,ft,63)!=0){strncpy(currentScrollingItemText,ft,63);currentScrollingItemText[63]=0;scrollTextOffset=0;scrollingTextPixelWidth=wf;}char sb[128];snprintf(sb,128,"%s   %s",currentScrollingItemText,currentScrollingItemText);int lws=strlen(currentScrollingItemText)+3;if(scrollTextOffset>=lws)scrollTextOffset=0;char ds[64];strncpy(ds,sb+scrollTextOffset,63);ds[63]=0;char fdb[64];int ac=0,cw=0;for(int i=0;ds[i]!=0;++i){char tc[2]={ds[i],0};uint16_t cw_d,ch_d;disp.getTextBounds(tc,0,0,&xb,&yb,&cw_d,&ch_d);if(cw+cw_d<=mW){cw+=cw_d;ac++;}else break;}strncpy(fdb,ds,ac);fdb[ac]=0;int tdyc=y_top+((8-ht)/2);if(inv){disp.fillRect(x,y_top,mW,8,1);disp.setTextColor(0);}else{disp.fillRect(x,y_top,mW,8,0);disp.setTextColor(1);}disp.setCursor(x,tdyc);disp.print(fdb);if(inv)disp.setTextColor(1);}
void drawInverseBoxLabel(Adafruit_SSD1306& d,const char* lt){int by=SCREEN_HEIGHT-defaultLabelHeight;if(lt==nullptr||strlen(lt)==0){d.fillRect(0,by,SCREEN_WIDTH,defaultLabelHeight,0);return;}d.setTextSize(1);d.setFont(nullptr);int16_t txb,tyb;uint16_t tw,th;d.getTextBounds(lt,0,0,&txb,&tyb,&tw,&th);const int bph=4,bpv=1;int bw=tw+bph*2,bh=th+bpv*2;if(bh<defaultLabelHeight)bh=defaultLabelHeight;int bx=(SCREEN_WIDTH-bw)/2;if(bx<0)bx=0;d.fillRect(bx>1?bx-2:0,by,bw+4,bh,0);d.fillRect(bx,by,bw,bh,1);d.setTextColor(0);d.setCursor(bx+bph,by+(bh-th)/2);d.print(lt);d.setTextColor(1);}
void drawOLED3StatusIcons(Adafruit_SSD1306&d){d.drawFastVLine(STATUS_ICON_AREA_WIDTH,0,SCREEN_HEIGHT,1);d.setTextSize(1);d.setTextColor(1);d.setCursor(2,2);if(digitalRead(TOP_LED_GREEN_PIN)==HIGH&&digitalRead(TOP_LED_RED_PIN)==LOW)d.print("TG");else if(digitalRead(TOP_LED_RED_PIN)==HIGH&&digitalRead(TOP_LED_GREEN_PIN)==LOW)d.print("TR");else if(digitalRead(TOP_LED_RED_PIN)==HIGH&&digitalRead(TOP_LED_GREEN_PIN)==HIGH)d.print("TO");else d.print("T-");d.setCursor(2,SCREEN_HEIGHT-8-2);if(digitalRead(BOTTOM_LED_GREEN_PIN)==HIGH)d.print("BG");else d.print("B-");}
void drawStaticHighlightMenuList(Adafruit_SSD1306&d,const MenuItem it[],int ni,int si,bool af,bool isp,int xo,int ys){d.setTextSize(1);const int ilh=10;int ypt=ys,yct=ys+ilh,ynt=ys+2*ilh;int imw=SCREEN_WIDTH-xo;d.fillRect(xo,ypt,imw,ilh*3,0);if(si>0&&ni>0&&it&&it[si-1].label)drawTruncatedText(d,xo,ypt+1,imw,it[si-1].label,false);if(si>=0&&si<ni&&it&&it[si].label){bool aih=af||isp;if(af)drawScrollingText(d,xo,yct+1,imw,it[si].label,aih);else drawTruncatedText(d,xo,yct+1,imw,it[si].label,aih);}if(si<ni-1&&ni>0&&it&&it[si+1].label)drawTruncatedText(d,xo,ynt+1,imw,it[si+1].label,false);}

// --- From Sect 7: Screen Specific Display Functions ---
void drawSystemStatusBarOLED3_ui(Adafruit_SSD1306& d){int x=CONTENT_START_X_OLED3,yi=1;d.fillRect(x,0,SCREEN_WIDTH-x,SYSTEM_STATUS_BAR_HEIGHT_OLED3,0);drawWiFiIcon(d,x,yi,3);x+=12+8;drawBluetoothIcon(d,x,yi,false);x+=20+8;drawBatteryIcon(d,x,yi,currentBatteryIconLevel);x+=15+8;displayDeviceModeIcon(d,x,yi+1);}

void displayHomeScreen_ui() {
    // This function will need to be updated with real data later
    // For now, it uses placeholders or last known values.
    char buf[32];
    if (needsRedrawOLED3) {
        tcaSelect(OLED3_TCA_CHANNEL);
        oled3.clearDisplay();
        drawOLED3StatusIcons(oled3);
        drawSystemStatusBarOLED3_ui(oled3);
        // ... rest of OLED3 home screen drawing
        oled3.display();
        needsRedrawOLED3 = false;
    }
    if (needsRedrawOLED2) {
        tcaSelect(OLED2_TCA_CHANNEL);
        oled2.clearDisplay();
        // ... OLED2 home screen drawing
        drawInverseBoxLabel(oled2, "");
        oled2.display();
        needsRedrawOLED2 = false;
    }
    if (needsRedrawOLED1) {
        tcaSelect(OLED1_TCA_CHANNEL);
        oled1.clearDisplay();
        // ... OLED1 home screen drawing
        drawInverseBoxLabel(oled1, "Menu");
        oled1.display();
        needsRedrawOLED1 = false;
    }
}

void displayMenuNavScreen_ui() {
    // ... Implementation from Sect 7 ...
}
void displayRtcSettingsScreen_ui() {
    // ... Implementation from Sect 7 ...
}
void displayBatteryInfoScreen_ui() {
    // ... Implementation from Sect 7 ...
}

// --- From Sect 9: Display Router & Status LEDs ---
void updateDisplay_ui() {
    unsigned long curT = millis();
    if (currentMenuState == STATE_HOME_SCREEN) {
        if (curT - lastAnimTime > 400) {
            lastAnimTime = curT;
            idleAnimPhase = (idleAnimPhase + 1) % 4;
            errorBlinkState = !errorBlinkState;
            calibAnimPhase = (calibAnimPhase + 1) % 4;
            measuringAnimPhase = (measuringAnimPhase + 1) % 2;
            needsRedrawOLED3 = true;
        }
    }
    if (isTextScrollingActive) {
        if (curT - lastScrollUpdateTime > SCROLL_DELAY_MS) {
            lastScrollUpdateTime = curT;
            int tl = strlen(currentScrollingItemText);
            if (tl > 0) scrollTextOffset = (scrollTextOffset + SCROLL_CHAR_STEP) % (tl + 3);
            else scrollTextOffset = 0;
            if (currentMenuState == STATE_MENU_NAV) {
                if (currentMenuLevel == 1 && needsRedrawOLED3) oled3.display();
                else if (currentMenuLevel == 2 && needsRedrawOLED2) oled2.display();
                else if (currentMenuLevel >= 3 && needsRedrawOLED1) oled1.display();
            }
        }
    }

    switch (currentMenuState) {
        case STATE_HOME_SCREEN: displayHomeScreen_ui(); break;
        case STATE_MENU_NAV: displayMenuNavScreen_ui(); break;
        case STATE_SETTINGS_RTC_ADJUST: displayRtcSettingsScreen_ui(); break;
        case STATE_SETTINGS_BATTERY_INFO: displayBatteryInfoScreen_ui(); break;
        default:
            if (needsRedrawOLED2) {
                tcaSelect(OLED2_TCA_CHANNEL);
                oled2.clearDisplay();
                oled2.setTextSize(1);
                oled2.setTextColor(1);
                oled2.setCursor(0, 0);
                oled2.print("Err: Unk State");
                drawInverseBoxLabel(oled2, "RESET?");
                oled2.display();
                needsRedrawOLED2 = false;
            }
            break;
    }
}