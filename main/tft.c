/* TFT demo

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <time.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "components/tft/tftspi.h"
#include "components/tft/tft.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_attr.h"
#include <sys/time.h>
#include <unistd.h>
#include "lwip/err.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "main.h"
#include "components/tft/dw_font.h"

/* ============================================================================== */
/* ============================================================================== */
//##########################################
bool SelectSW_Push = false;
bool PaidSW_Push = false;

unsigned char SelectPush_Count = 0;
unsigned char SelectReld_Count = 0;
unsigned char PaidPush_Count = 0;
unsigned char PaidReld_Count = 0;

unsigned char DisplayMode = 0;
unsigned char PageDisplay = 0;
unsigned char OldDisplayMode = 0xFF;

unsigned int WaittingTime = 0;
unsigned int CounterTime = 0;

bool Changed_Display = false;
bool ReduceTime = false;


// The structure to manage the QR code
QRCode qrcode;

extern dw_font_info_t font_th_sarabunpsk_regular40;
dw_font_t myfont;

// ==========================================================
// Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST
#define SPI_BUS TFT_HSPI_HOST


// ==========================================================
//static int _demo_pass = 0;
//static uint8_t doprint = 1;
//static uint8_t run_gs_demo = 0; // Run gray scale demo if set to 1
static struct tm* tm_info;
static char tmp_buff[64];
static time_t time_now, time_last = 0;

#define GDEMO_TIME 1000
#define GDEMO_INFO_TIME 5000

static const char tag[] = "[TFT_DISPLAY]";


extern uint8_t bluepay_jpg_start[] asm("_binary_bluepay_jpg_start");
extern uint8_t bluepay_jpg_end[] asm("_binary_bluepay_jpg_end");

extern uint8_t bluepay_press_jpg_start[] asm("_binary_bluepay_press_jpg_start");
extern uint8_t bluepay_press_jpg_end[] asm("_binary_bluepay_press_jpg_end");

extern uint8_t bottom1_jpg_start[] asm("_binary_bottom1_jpg_start");
extern uint8_t bottom1_jpg_end[] asm("_binary_bottom1_jpg_end");

extern uint8_t bottom1_press_jpg_start[] asm("_binary_bottom1_press_jpg_start");
extern uint8_t bottom1_press_jpg_end[] asm("_binary_bottom1_press_jpg_end");

extern uint8_t bottom2_jpg_start[] asm("_binary_bottom2_jpg_start");
extern uint8_t bottom2_jpg_end[] asm("_binary_bottom2_jpg_end");

extern uint8_t bottom2_press_jpg_start[] asm("_binary_bottom2_press_jpg_start");
extern uint8_t bottom2_press_jpg_end[] asm("_binary_bottom2_press_jpg_end");

extern uint8_t promppay_jpg_start[] asm("_binary_promppay_jpg_start");
extern uint8_t promppay_jpg_end[] asm("_binary_promppay_jpg_end");

extern uint8_t promppay_press_jpg_start[] asm("_binary_promppay_press_jpg_start");
extern uint8_t promppay_press_jpg_end[] asm("_binary_promppay_press_jpg_end");

extern uint8_t rabbitline_jpg_start[] asm("_binary_rabbitline_jpg_start");
extern uint8_t rabbitline_jpg_end[] asm("_binary_rabbitline_jpg_end");

extern uint8_t rabbitline_press_jpg_start[] asm("_binary_rabbitline_press_jpg_start");
extern uint8_t rabbitline_press_jpg_end[] asm("_binary_rabbitline_press_jpg_end");

extern uint8_t topmenu_jpg_start[] asm("_binary_topmenu_jpg_start");
extern uint8_t topmenu_jpg_end[] asm("_binary_topmenu_jpg_end");

extern uint8_t truemoney_jpg_start[] asm("_binary_truemoney_jpg_start");
extern uint8_t truemoney_jpg_end[] asm("_binary_truemoney_jpg_end");

extern uint8_t truemoney_press_jpg_start[] asm("_binary_truemoney_press_jpg_start");
extern uint8_t truemoney_press_jpg_end[] asm("_binary_truemoney_press_jpg_end");

extern uint8_t header_coins_jpg_start[] asm("_binary_header_coins_jpg_start");
extern uint8_t header_coins_jpg_end[] asm("_binary_header_coins_jpg_end");

extern uint8_t footer_coins_jpg_start[] asm("_binary_footer_coins_jpg_start");
extern uint8_t footer_coins_jpg_end[] asm("_binary_footer_coins_jpg_end");


//-------------------------------
static void initialize_sntp(void)
{
    ESP_LOGI(tag, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}


//--------------------------
static int obtain_time(void)
{
	int res = 1;

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    initialize_sntp();

    // wait for time to be set
    int retry = 0;
    const int retry_count = 20;

    time(&time_now);
	tm_info = localtime(&time_now);

    while(tm_info->tm_year < (2019 - 1900) && ++retry < retry_count)
    {
		sprintf(tmp_buff, "Wait %0d/%d", retry, retry_count);
    	TFT_print(tmp_buff, CENTER, LASTY);
		vTaskDelay(500 / portTICK_RATE_MS);
        time(&time_now);
    	tm_info = localtime(&time_now);
    }

    if (tm_info->tm_year < (2019 - 1900)) {
    	ESP_LOGI(tag, "System time NOT set.");
    	res = 0;
    }
    else {
    	ESP_LOGI(tag, "System time is set.");
    }

    return res;
}


//==================================================================================
//----------------------
static void _checkTime()
{
	time(&time_now);
	if (time_now > time_last) {
		color_t last_fg, last_bg;
		time_last = time_now;
		tm_info = localtime(&time_now);
		sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

		TFT_saveClipWin();
		TFT_resetclipwin();

		Font curr_font = cfont;
		last_bg = _bg;
		last_fg = _fg;
		_fg = TFT_YELLOW;
		_bg = (color_t){ 64, 64, 64 };
		TFT_setFont(DEFAULT_FONT, NULL);

		TFT_fillRect(1, _height-TFT_getfontheight()-8, _width-3, TFT_getfontheight()+6, _bg);
		TFT_print(tmp_buff, CENTER, _height-TFT_getfontheight()-5);

		cfont = curr_font;
		_fg = last_fg;
		_bg = last_bg;

		TFT_restoreClipWin();
	}
}


//---------------------
static int Wait(int ms)
{
	uint8_t tm = 1;
	if (ms < 0) {
		tm = 0;
		ms *= -1;
	}
	if (ms <= 50) {
		vTaskDelay(ms / portTICK_RATE_MS);
		//if (_checkTouch()) return 0;
	}
	else {
		for (int n=0; n<ms; n += 50) {
			vTaskDelay(50 / portTICK_RATE_MS);
			if (tm) _checkTime();
		}
	}
	return 1;
}


//-------------------------------------------------------------------
//static unsigned int rand_interval(unsigned int min, unsigned int max)
//{
//    int r;
//    const unsigned int range = 1 + max - min;
//    const unsigned int buckets = RAND_MAX / range;
//    const unsigned int limit = buckets * range;
//
//    /* Create equal size buckets all in a row, then fire randomly towards
//     * the buckets until you land in one of them. All buckets are equally
//     * likely. If you land off the end of the line of buckets, try again. */
//    do
//    {
//        r = rand();
//    } while (r >= limit);
//
//    return min + (r / buckets);
//}
//

// Generate random color
//-----------------------------
//static color_t random_color() {
//
//	color_t color;
//	color.r  = (uint8_t)rand_interval(8,252);
//	color.g  = (uint8_t)rand_interval(8,252);
//	color.b  = (uint8_t)rand_interval(8,252);
//	return color;
//}


//---------------------
static void _dispTime()
{
	Font curr_font = cfont;
    if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	else TFT_setFont(DEFAULT_FONT, NULL);

    time(&time_now);
	time_last = time_now;
	tm_info = localtime(&time_now);
	sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
	TFT_print(tmp_buff, CENTER, _height-TFT_getfontheight()-5);

    cfont = curr_font;
}


//---------------------------------
static void disp_header(char *info)
{
	TFT_fillScreen(TFT_BLACK);
	TFT_resetclipwin();

	_fg = TFT_YELLOW;
	_bg = (color_t){ 64, 64, 64 };

    if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	else TFT_setFont(DEFAULT_FONT, NULL);
	TFT_fillRect(0, 0, _width-1, TFT_getfontheight()+8, _bg);
	TFT_drawRect(0, 0, _width-1, TFT_getfontheight()+8, TFT_CYAN);

	TFT_fillRect(0, _height-TFT_getfontheight()-9, _width-1, TFT_getfontheight()+8, _bg);
	TFT_drawRect(0, _height-TFT_getfontheight()-9, _width-1, TFT_getfontheight()+8, TFT_CYAN);

	TFT_print(info, CENTER, 4);
	_dispTime();

	_bg = TFT_BLACK;
	TFT_setclipwin(0,TFT_getfontheight()+9, _width-1, _height-TFT_getfontheight()-10);
}


//---------------------------------------------
static void update_header(char *hdr, char *ftr)
{
	color_t last_fg, last_bg;

	TFT_saveClipWin();
	TFT_resetclipwin();

	Font curr_font = cfont;
	last_bg = _bg;
	last_fg = _fg;
	_fg = TFT_YELLOW;
	_bg = (color_t){ 64, 64, 64 };
    if (_width < 240) TFT_setFont(DEF_SMALL_FONT, NULL);
	else TFT_setFont(DEFAULT_FONT, NULL);

	if (hdr) {
		TFT_fillRect(1, 1, _width-3, TFT_getfontheight()+6, _bg);
		TFT_print(hdr, CENTER, 4);
	}

	if (ftr) {
		TFT_fillRect(1, _height-TFT_getfontheight()-8, _width-3, TFT_getfontheight()+6, _bg);
		if (strlen(ftr) == 0) _dispTime();
		else TFT_print(ftr, CENTER, _height-TFT_getfontheight()-5);
	}

	cfont = curr_font;
	_fg = last_fg;
	_bg = last_bg;

	TFT_restoreClipWin();
}


//------------------------
//static void test_times() {
//
//	if (doprint) {
//	    uint32_t tstart, t1, t2;
//		disp_header("TIMINGS");
//		// ** Show Fill screen and send_line timings
//		tstart = clock();
//		TFT_fillWindow(TFT_BLACK);
//		t1 = clock() - tstart;
//		printf("     Clear screen time: %u ms\r\n", t1);
//		TFT_setFont(SMALL_FONT, NULL);
//		sprintf(tmp_buff, "Clear screen: %u ms", t1);
//		TFT_print(tmp_buff, 0, 140);
//
//		color_t *color_line = heap_caps_malloc((_width*3), MALLOC_CAP_DMA);
//		color_t *gsline = NULL;
//		if (gray_scale) gsline = malloc(_width*3);
//		if (color_line) {
//			float hue_inc = (float)((10.0 / (float)(_height-1) * 360.0));
//			for (int x=0; x<_width; x++) {
//				color_line[x] = HSBtoRGB(hue_inc, 1.0, (float)x / (float)_width);
//				if (gsline) gsline[x] = color_line[x];
//			}
//			disp_select();
//			tstart = clock();
//			for (int n=0; n<1000; n++) {
//				if (gsline) memcpy(color_line, gsline, _width*3);
//				send_data(0, 40+(n&63), dispWin.x2-dispWin.x1, 40+(n&63), (uint32_t)(dispWin.x2-dispWin.x1+1), color_line);
//				wait_trans_finish(1);
//			}
//			t2 = clock() - tstart;
//			disp_deselect();
//
//			printf("Send color buffer time: %u us (%d pixels)\r\n", t2, dispWin.x2-dispWin.x1+1);
//			free(color_line);
//
//			sprintf(tmp_buff, "   Send line: %u us", t2);
//			TFT_print(tmp_buff, 0, 144+TFT_getfontheight());
//		}
//		Wait(GDEMO_INFO_TIME);
//    }
//}

//---------------------
//static void rect_demo()
//{
//	int x, y, w, h, n;
//
//	disp_header("RECTANGLE DEMO");
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(4, dispWin.x2-4);
//		y = rand_interval(4, dispWin.y2-2);
//		w = rand_interval(2, dispWin.x2-x);
//		h = rand_interval(2, dispWin.y2-y);
//		TFT_drawRect(x,y,w,h,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d RECTANGLES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("FILLED RECTANGLE", "");
//	TFT_fillWindow(TFT_BLACK);
//	end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(4, dispWin.x2-4);
//		y = rand_interval(4, dispWin.y2-2);
//		w = rand_interval(2, dispWin.x2-x);
//		h = rand_interval(2, dispWin.y2-y);
//		TFT_fillRect(x,y,w,h,random_color());
//		TFT_drawRect(x,y,w,h,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d RECTANGLES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}
//

//----------------------
//static void pixel_demo()
//{
//	int x, y, n;
//
//	disp_header("DRAW PIXEL DEMO");
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(0, dispWin.x2);
//		y = rand_interval(0, dispWin.y2);
//		TFT_drawPixel(x,y,random_color(),1);
//		n++;
//	}
//	sprintf(tmp_buff, "%d PIXELS", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}


//---------------------
//static void line_demo()
//{
//	int x1, y1, x2, y2, n;
//
//	disp_header("LINE DEMO");
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x1 = rand_interval(0, dispWin.x2);
//		y1 = rand_interval(0, dispWin.y2);
//		x2 = rand_interval(0, dispWin.x2);
//		y2 = rand_interval(0, dispWin.y2);
//		TFT_drawLine(x1,y1,x2,y2,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d LINES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}


//----------------------
//static void aline_demo()
//{
//	int x, y, len, angle, n;
//
//	disp_header("LINE BY ANGLE DEMO");
//
//	x = (dispWin.x2 - dispWin.x1) / 2;
//	y = (dispWin.y2 - dispWin.y1) / 2;
//	if (x < y) len = x - 8;
//	else len = y -8;
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		for (angle=0; angle < 360; angle++) {
//			TFT_drawLineByAngle(x,y, 0, len, angle, random_color());
//			n++;
//		}
//	}
//
//	TFT_fillWindow(TFT_BLACK);
//	end_time = clock() + GDEMO_TIME;
//	while ((clock() < end_time) && (Wait(0))) {
//		for (angle=0; angle < 360; angle++) {
//			TFT_drawLineByAngle(x, y, len/4, len/4,angle, random_color());
//			n++;
//		}
//		for (angle=0; angle < 360; angle++) {
//			TFT_drawLineByAngle(x, y, len*3/4, len/4,angle, random_color());
//			n++;
//		}
//	}
//	sprintf(tmp_buff, "%d LINES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}


//--------------------
//static void arc_demo()
//{
//	uint16_t x, y, r, th, n, i;
//	float start, end;
//	color_t color, fillcolor;
//
//	disp_header("ARC DEMO");
//
//	x = (dispWin.x2 - dispWin.x1) / 2;
//	y = (dispWin.y2 - dispWin.y1) / 2;
//
//	th = 6;
//	uint32_t end_time = clock() + GDEMO_TIME;
//	i = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		if (x < y) r = x - 4;
//		else r = y - 4;
//		start = 0;
//		end = 20;
//		n = 1;
//		while (r > 10) {
//			color = random_color();
//			TFT_drawArc(x, y, r, th, start, end, color, color);
//			r -= (th+2);
//			n++;
//			start += 30;
//			end = start + (n*20);
//			i++;
//		}
//	}
//	sprintf(tmp_buff, "%d ARCS", i);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("OUTLINED ARC", "");
//	TFT_fillWindow(TFT_BLACK);
//	th = 8;
//	end_time = clock() + GDEMO_TIME;
//	i = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		if (x < y) r = x - 4;
//		else r = y - 4;
//		start = 0;
//		end = 350;
//		n = 1;
//		while (r > 10) {
//			color = random_color();
//			fillcolor = random_color();
//			TFT_drawArc(x, y, r, th, start, end, color, fillcolor);
//			r -= (th+2);
//			n++;
//			start += 20;
//			end -= n*10;
//			i++;
//		}
//	}
//	sprintf(tmp_buff, "%d ARCS", i);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}


//-----------------------
//static void circle_demo()
//{
//	int x, y, r, n;
//
//	disp_header("CIRCLE DEMO");
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(8, dispWin.x2-8);
//		y = rand_interval(8, dispWin.y2-8);
//		if (x < y) r = rand_interval(2, x/2);
//		else r = rand_interval(2, y/2);
//		TFT_drawCircle(x,y,r,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d CIRCLES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("FILLED CIRCLE", "");
//	TFT_fillWindow(TFT_BLACK);
//	end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(8, dispWin.x2-8);
//		y = rand_interval(8, dispWin.y2-8);
//		if (x < y) r = rand_interval(2, x/2);
//		else r = rand_interval(2, y/2);
//		TFT_fillCircle(x,y,r,random_color());
//		TFT_drawCircle(x,y,r,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d CIRCLES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}


//------------------------
//static void ellipse_demo()
//{
//	int x, y, rx, ry, n;
//
//	disp_header("ELLIPSE DEMO");
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(8, dispWin.x2-8);
//		y = rand_interval(8, dispWin.y2-8);
//		if (x < y) rx = rand_interval(2, x/4);
//		else rx = rand_interval(2, y/4);
//		if (x < y) ry = rand_interval(2, x/4);
//		else ry = rand_interval(2, y/4);
//		TFT_drawEllipse(x,y,rx,ry,random_color(),15);
//		n++;
//	}
//	sprintf(tmp_buff, "%d ELLIPSES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("FILLED ELLIPSE", "");
//	TFT_fillWindow(TFT_BLACK);
//	end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(8, dispWin.x2-8);
//		y = rand_interval(8, dispWin.y2-8);
//		if (x < y) rx = rand_interval(2, x/4);
//		else rx = rand_interval(2, y/4);
//		if (x < y) ry = rand_interval(2, x/4);
//		else ry = rand_interval(2, y/4);
//		TFT_fillEllipse(x,y,rx,ry,random_color(),15);
//		TFT_drawEllipse(x,y,rx,ry,random_color(),15);
//		n++;
//	}
//	sprintf(tmp_buff, "%d ELLIPSES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("ELLIPSE SEGMENTS", "");
//	TFT_fillWindow(TFT_BLACK);
//	end_time = clock() + GDEMO_TIME;
//	n = 0;
//	int k = 1;
//	while ((clock() < end_time) && (Wait(0))) {
//		x = rand_interval(8, dispWin.x2-8);
//		y = rand_interval(8, dispWin.y2-8);
//		if (x < y) rx = rand_interval(2, x/4);
//		else rx = rand_interval(2, y/4);
//		if (x < y) ry = rand_interval(2, x/4);
//		else ry = rand_interval(2, y/4);
//		TFT_fillEllipse(x,y,rx,ry,random_color(), (1<<k));
//		TFT_drawEllipse(x,y,rx,ry,random_color(), (1<<k));
//		k = (k+1) & 3;
//		n++;
//	}
//	sprintf(tmp_buff, "%d SEGMENTS", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}
//

//-------------------------
//static void triangle_demo()
//{
//	int x1, y1, x2, y2, x3, y3, n;
//
//	disp_header("TRIANGLE DEMO");
//
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x1 = rand_interval(4, dispWin.x2-4);
//		y1 = rand_interval(4, dispWin.y2-2);
//		x2 = rand_interval(4, dispWin.x2-4);
//		y2 = rand_interval(4, dispWin.y2-2);
//		x3 = rand_interval(4, dispWin.x2-4);
//		y3 = rand_interval(4, dispWin.y2-2);
//		TFT_drawTriangle(x1,y1,x2,y2,x3,y3,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d TRIANGLES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("FILLED TRIANGLE", "");
//	TFT_fillWindow(TFT_BLACK);
//	end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		x1 = rand_interval(4, dispWin.x2-4);
//		y1 = rand_interval(4, dispWin.y2-2);
//		x2 = rand_interval(4, dispWin.x2-4);
//		y2 = rand_interval(4, dispWin.y2-2);
//		x3 = rand_interval(4, dispWin.x2-4);
//		y3 = rand_interval(4, dispWin.y2-2);
//		TFT_fillTriangle(x1,y1,x2,y2,x3,y3,random_color());
//		TFT_drawTriangle(x1,y1,x2,y2,x3,y3,random_color());
//		n++;
//	}
//	sprintf(tmp_buff, "%d TRIANGLES", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}
//

//---------------------
//static void poly_demo()
//{
//	uint16_t x, y, rot, oldrot;
//	int i, n, r;
//	uint8_t sides[6] = {3, 4, 5, 6, 8, 10};
//	color_t color[6] = {TFT_WHITE, TFT_CYAN, TFT_RED,       TFT_BLUE,     TFT_YELLOW,     TFT_ORANGE};
//	color_t fill[6]  = {TFT_BLUE,  TFT_NAVY,   TFT_DARKGREEN, TFT_DARKGREY, TFT_LIGHTGREY, TFT_OLIVE};
//
//	disp_header("POLYGON DEMO");
//
//	x = (dispWin.x2 - dispWin.x1) / 2;
//	y = (dispWin.y2 - dispWin.y1) / 2;
//
//	rot = 0;
//	oldrot = 0;
//	uint32_t end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		if (x < y) r = x - 4;
//		else r = y - 4;
//		for (i=5; i>=0; i--) {
//			TFT_drawPolygon(x, y, sides[i], r, TFT_BLACK, TFT_BLACK, oldrot, 1);
//			TFT_drawPolygon(x, y, sides[i], r, color[i], color[i], rot, 1);
//			r -= 16;
//            if (r <= 0) break;
//			n += 2;
//		}
//		Wait(100);
//		oldrot = rot;
//		rot = (rot + 15) % 360;
//	}
//	sprintf(tmp_buff, "%d POLYGONS", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//
//	update_header("FILLED POLYGON", "");
//	rot = 0;
//	end_time = clock() + GDEMO_TIME;
//	n = 0;
//	while ((clock() < end_time) && (Wait(0))) {
//		if (x < y) r = x - 4;
//		else r = y - 4;
//		TFT_fillWindow(TFT_BLACK);
//		for (i=5; i>=0; i--) {
//			TFT_drawPolygon(x, y, sides[i], r, color[i], fill[i], rot, 2);
//			r -= 16;
//            if (r <= 0) break;
//			n += 2;
//		}
//		Wait(500);
//		rot = (rot + 15) % 360;
//	}
//	sprintf(tmp_buff, "%d POLYGONS", n);
//	update_header(NULL, tmp_buff);
//	Wait(-GDEMO_INFO_TIME);
//}



//##########################################
void draw_pixel(int16_t x, int16_t y)
{
	TFT_drawPixel(x,y,TFT_RED,1);
}


//##########################################
void clear_pixel(int16_t x, int16_t y)
{
	TFT_drawPixel(x,y,TFT_WHITE,1);
}

//#########################################################################################
void Display_QRcode(int offset_x, int offset_y, uint8_t element_size, uint8_t qrversion, char* Message)
{
	// Create the QR code ~120 char maximum
	uint8_t qrcodeData[qrcode_getBufferSize(qrversion)];
	qrcode_initText(&qrcode, qrcodeData, qrversion, 0, Message);

	for (int y = 0; y < qrcode.size; y++) {
		for (int x = 0; x < qrcode.size; x++) {
			  if (qrcode_getModule(&qrcode, x, y))
			  {
				    TFT_fillRect(x*element_size+offset_x,y*element_size+offset_y,element_size,element_size, TFT_BLACK);
			  }
			  else 
			  {
				    TFT_fillRect(x*element_size+offset_x,y*element_size+offset_y,element_size,element_size, TFT_WHITE);
			  }
		}
	}
}


//static void ReturntoHome(void)
//{
//	ReduceTime = false;
//
//	CounterTime = 0;
//	WaittingTime = 0;
//
//	SelectPush_Count = 0;
//
//	SelectSW_Push = true;
//	PageDisplay = homepage;
//	OldDisplayMode = 0xFF;
//
//	if(FDSTATUS.StartCheckPayment == true){
//		FDSTATUS.StartCheckPayment = false;
//		FDSTATUS.PaymentSuccess = false;
//		vTaskDelete(ChPayment);
//	}
//
//	TFT_fillScreen(TFT_BLACK);
//	TFT_resetclipwin();
//
//	TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);
//
//	TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);
//
//	TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
//
//	TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
//
//	TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
//
//	TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//
//	TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
//
//}

//===============
//static void TFT_Display_Old(void *pvParameters)
//{
//	font_rotate = 0;
//	text_wrap = 0;
//	font_transparent = 0;
//	font_forceFixed = 0;
//	TFT_resetclipwin();
//
//	image_debug = 0;
//
//    char dtype[16];
//
//    switch (tft_disp_type) {
//        case DISP_TYPE_ILI9341:
//            sprintf(dtype, "ILI9341");
//            break;
//        case DISP_TYPE_ILI9488:
//            sprintf(dtype, "ILI9488");
//            break;
//        case DISP_TYPE_ST7789V:
//            sprintf(dtype, "ST7789V");
//            break;
//        case DISP_TYPE_ST7735:
//            sprintf(dtype, "ST7735");
//            break;
//        case DISP_TYPE_ST7735R:
//            sprintf(dtype, "ST7735R");
//            break;
//        case DISP_TYPE_ST7735B:
//            sprintf(dtype, "ST7735B");
//            break;
//        default:
//            sprintf(dtype, "Unknown");
//    }
//
//	TFT_fillScreen(TFT_BLACK);
//	TFT_resetclipwin();
//
//	TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);
//
//	TFT_jpg_image(0, 49, 0, NULL, promppay_press_jpg_start, promppay_press_jpg_end-promppay_press_jpg_start);
//
//	TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
//
//	TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
//
//	TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
//
//	TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//
//	TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
//
//	while (1) {
//		vTaskDelay(1 / portTICK_PERIOD_MS);
//
//		switch(PageDisplay){
//
//			/***************************************************************/
//			/***************************************************************/
//			case homepage:
//
//				if(gpio_get_level(SWSELECT_PIN) == 0){
//					if(SelectSW_Push == false){
//						if(++SelectPush_Count > 50){
//							SelectPush_Count = 0;
//
//							SelectSW_Push = true;
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_press_jpg_start, bottom1_press_jpg_end-bottom1_press_jpg_start);
//
//							if(++DisplayMode > 3){
//								DisplayMode = 0;
//							}
//						}
//					}
//					SelectReld_Count = 0;
//
//				}else{
//					if(SelectSW_Push == true){
//						if(++SelectReld_Count > 50){
//							SelectReld_Count = 0;
//
//							SelectSW_Push = false;
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//						}
//					}
//					SelectPush_Count = 0;
//				}
//
//
//				switch(DisplayMode){
//					case promppay_disp:
//						if(OldDisplayMode != promppay_disp){
//							OldDisplayMode = promppay_disp;
//							TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
//							TFT_jpg_image(0, 49, 0, NULL, promppay_press_jpg_start, promppay_press_jpg_end-promppay_press_jpg_start);
//						}
//					break;
//
//					case truemoney_disp:
//						if(OldDisplayMode != truemoney_disp){
//							OldDisplayMode = truemoney_disp;
//							TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);
//							TFT_jpg_image(0, 141, 0, NULL, truemoney_press_jpg_start, truemoney_press_jpg_end-truemoney_press_jpg_start);
//						}
//					break;
//
//					case rabbitline_disp:
//						if(OldDisplayMode != rabbitline_disp){
//							OldDisplayMode = rabbitline_disp;
//							TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
//							TFT_jpg_image(0, 239, 0, NULL, rabbitline_press_jpg_start, rabbitline_press_jpg_end-rabbitline_press_jpg_start);
//						}
//					break;
//
//					case bluepay_disp:
//						if(OldDisplayMode != bluepay_disp){
//							OldDisplayMode = bluepay_disp;
//							TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
//							TFT_jpg_image(0, 331, 0, NULL, bluepay_press_jpg_start, bluepay_press_jpg_end-bluepay_press_jpg_start);
//						}
//					break;
//				}
//
//
//				if(gpio_get_level(SWPAIDS_PIN) == 0){
//					if(PaidSW_Push == false){
//						if(++PaidPush_Count > 50){
//							PaidPush_Count = 0;
//
//							PaidSW_Push = true;
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);
//							PageDisplay = qrpage;
//							Changed_Display = true;
//
//							TFT_fillScreen(TFT_WHITE);
//							TFT_resetclipwin();
//						}
//					}
//					PaidReld_Count = 0;
//
//				}else{
//					if(PaidSW_Push == true){
//						if(++PaidReld_Count > 50){
//							PaidReld_Count = 0;
//
//							PaidSW_Push = false;
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
//						}
//					}
//					PaidPush_Count = 0;
//				}
//
//			break;
//
//
//			/***************************************************************/
//			/***************************************************************/
//			case qrpage:
//				if(Changed_Display == true){
//					switch(DisplayMode){
//						case promppay_disp:
//							TFT_jpg_image(0, 0, 0, NULL, promppay_press_jpg_start, promppay_press_jpg_end-promppay_press_jpg_start);
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);
//
//							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//							dw_font_goto(&myfont, 75, 225);
//							dw_font_print(&myfont, "กรุณารอสักครู่...");
//
//							if(API_GenQRCode(2, (DevPrices - AllMoney)) == true){
//
//								cJSON *Response = cJSON_Parse(DataResponse);
//
//								char ResRef1[20];
//								char ResRef2[20];
//								char ResPrice[5];
//								char ResQR[500];
//
//								const cJSON *code = NULL;
//        						const cJSON *ref1 = NULL;
//        						const cJSON *ref2 = NULL;
//        						const cJSON *price = NULL;
//        						const cJSON *qr_data = NULL;
//
//						        if (Response == NULL)
//						        {
//						            const char *error_ptr = cJSON_GetErrorPtr();
//						            if (error_ptr != NULL)
//						            {
//						                fprintf(stderr, "Error before: %s\n", error_ptr);
//						            }
//
//						        }else{
//
//						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
//						            if (cJSON_IsString(code) && (code->valuestring != NULL))
//						            {
//						            	if(strcmp(code->valuestring, "E00") == 0){
//
//						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
//							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
//							                {
//							                    memset(ResRef1, 0, sizeof(ResRef1));
//							                    strcpy(ResRef1, ref1->valuestring);
//							                    strcat(ResRef1, DEOF);
//							                }
//
//							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
//							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
//							                {
//							                    memset(ResRef2, 0, sizeof(ResRef2));
//							                    strcpy(ResRef2, ref2->valuestring);
//							                    strcat(ResRef2, DEOF);
//							                }
//
//							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
//							                if (cJSON_IsString(price) && (price->valuestring != NULL))
//							                {
//							                    memset(ResPrice, 0, sizeof(ResPrice));
//							                    strcpy(ResPrice, price->valuestring);
//							                    strcat(ResPrice, DEOF);
//							                }
//
//							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
//							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
//							                {
//							                    memset(ResQR, 0, sizeof(ResQR));
//							                    strcpy(ResQR, qr_data->valuestring);
//							                    strcat(ResQR, DEOF);
//							                }
//
//							                if(API_CheckPayment() == true){
//
//							                	// printf("CheckingResponse : %s\n", CheckingResponse);
//
//							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);
//
//												const cJSON *chcode = NULL;
//				        						const cJSON *transid = NULL;
//				        						const cJSON *price = NULL;
//
//				        						if (CheckResponse == NULL)
//										        {
//										            const char *error_ptr = cJSON_GetErrorPtr();
//										            if (error_ptr != NULL)
//										            {
//										                fprintf(stderr, "Error before: %s\n", error_ptr);
//										                FDSTATUS.PrintTransError = true;
//										            }
//
//										        }else{
//
//					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
//										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
//										            {
//										            	if(strcmp(chcode->valuestring, "E00") == 0){
//
//										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
//											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
//											                    strcpy(OldTransectionID, transid->valuestring);
//											                    strcat(OldTransectionID, DEOF);
//											                    printf("OldTransectionID:%s\n", OldTransectionID);
//
//											                }
//
//											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
//											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
//											                    strcpy(OldPriceQR, price->valuestring);
//											                    strcat(OldPriceQR, DEOF);
//											                    printf("OldPriceQR:%s\n", OldPriceQR);
//
//											                }
//
//											                printf("ResQR=%d\n", strlen(ResQR));
//											                Display_QRcode(45, 135, 4, 10, ResQR);
//
//											                _fg = TFT_BLUE;
//															_bg = TFT_WHITE;
//															TFT_setFont(DEJAVU18_FONT, NULL);
//
//															char dt[100];
//
//															sprintf(dt, "REF1:%s", ResRef1);
//															TFT_print(dt, CENTER, 380);
//
//															sprintf(dt, "REF2:%s", ResRef2);
//															TFT_print(dt, CENTER, 405);
//
//															ReduceTime = true;
//															WaittingTime = 60;
//															FDSTATUS.StartCheckPayment = true;
//															vTaskDelay(250/portTICK_PERIOD_MS);
//															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
//														}
//
//													}else{
//
//														FDSTATUS.PrintTransError = true;
//													}
//												}
//
//												cJSON_Delete(CheckResponse);
//
//											}else{
//
//												FDSTATUS.PrintTransError = true;
//											}
//
//											if(FDSTATUS.PrintTransError == true){
//
//												FDSTATUS.PrintTransError = false;
//
//												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//												dw_font_goto(&myfont, 75, 225);
//												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
//											}
//
//
//						            	}else{
//						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//											dw_font_goto(&myfont, 75, 225);
//											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//						            	}
//						            }
//						        }
//
//						        cJSON_Delete(Response);
//
//							}else{
//								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//								dw_font_goto(&myfont, 75, 225);
//								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//
//								if(FDSTATUS.FailtoReset == true){
//									vTaskDelay(3000 / portTICK_RATE_MS);
//									esp_restart();
//
//								}else
//								{
//									FDSTATUS.ReturnHomeScreen = true;
//								}
//
//								// else{
//								// 	vTaskDelay(3000 / portTICK_RATE_MS);
//								// 	// ReturntoHome();
//								// }
//							}
//						break;
//
//						case truemoney_disp:
//							TFT_jpg_image(0, 0, 0, NULL, truemoney_press_jpg_start, truemoney_press_jpg_end-truemoney_press_jpg_start);
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);
//
//							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//							dw_font_goto(&myfont, 75, 225);
//							dw_font_print(&myfont, "กรุณารอสักครู่...");
//
//							if(API_GenQRCode(6, (DevPrices - AllMoney)) == true){
//
//								cJSON *Response = cJSON_Parse(DataResponse);
//
//								char ResRef1[20];
//								char ResRef2[20];
//								char ResPrice[5];
//								char ResQR[200];
//
//								const cJSON *code = NULL;
//        						const cJSON *ref1 = NULL;
//        						const cJSON *ref2 = NULL;
//        						const cJSON *price = NULL;
//        						const cJSON *qr_data = NULL;
//
//						        if (Response == NULL)
//						        {
//						            const char *error_ptr = cJSON_GetErrorPtr();
//						            if (error_ptr != NULL)
//						            {
//						                fprintf(stderr, "Error before: %s\n", error_ptr);
//						            }
//
//						        }else{
//
//						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
//						            if (cJSON_IsString(code) && (code->valuestring != NULL))
//						            {
//						            	if(strcmp(code->valuestring, "E00") == 0){
//
//						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
//							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
//							                {
//							                    memset(ResRef1, 0, sizeof(ResRef1));
//							                    strcpy(ResRef1, ref1->valuestring);
//							                    strcat(ResRef1, DEOF);
//							                }
//
//							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
//							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
//							                {
//							                    memset(ResRef2, 0, sizeof(ResRef2));
//							                    strcpy(ResRef2, ref2->valuestring);
//							                    strcat(ResRef2, DEOF);
//							                }
//
//							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
//							                if (cJSON_IsString(price) && (price->valuestring != NULL))
//							                {
//							                    memset(ResPrice, 0, sizeof(ResPrice));
//							                    strcpy(ResPrice, price->valuestring);
//							                    strcat(ResPrice, DEOF);
//							                }
//
//							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
//							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
//							                {
//							                    memset(ResQR, 0, sizeof(ResQR));
//							                    strcpy(ResQR, qr_data->valuestring);
//							                    strcat(ResQR, DEOF);
//							                }
//
//							                if(API_CheckPayment() == true){
//
//							                	// printf("CheckingResponse : %s\n", CheckingResponse);
//
//							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);
//
//												const cJSON *chcode = NULL;
//				        						const cJSON *transid = NULL;
//				        						const cJSON *price = NULL;
//
//				        						if (CheckResponse == NULL)
//										        {
//										            const char *error_ptr = cJSON_GetErrorPtr();
//										            if (error_ptr != NULL)
//										            {
//										                fprintf(stderr, "Error before: %s\n", error_ptr);
//										                FDSTATUS.PrintTransError = true;
//										            }
//
//										        }else{
//
//					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
//										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
//										            {
//										            	if(strcmp(chcode->valuestring, "E00") == 0){
//
//										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
//											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
//											                    strcpy(OldTransectionID, transid->valuestring);
//											                    strcat(OldTransectionID, DEOF);
//											                    printf("OldTransectionID:%s\n", OldTransectionID);
//
//											                }
//
//											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
//											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
//											                    strcpy(OldPriceQR, price->valuestring);
//											                    strcat(OldPriceQR, DEOF);
//											                    printf("OldPriceQR:%s\n", OldPriceQR);
//
//											                }
//
//											                printf("ResQR=%d\n", strlen(ResQR));
//											                Display_QRcode(47, 135, 5, 7, ResQR);
//
//											                _fg = TFT_BLUE;
//															_bg = TFT_WHITE;
//															TFT_setFont(DEJAVU18_FONT, NULL);
//
//															char dt[100];
//
//															sprintf(dt, "REF1:%s", ResRef1);
//															TFT_print(dt, CENTER, 380);
//
//															sprintf(dt, "REF2:%s", ResRef2);
//															TFT_print(dt, CENTER, 405);
//
//															ReduceTime = true;
//															WaittingTime = 60;
//															FDSTATUS.StartCheckPayment = true;
//															vTaskDelay(250/portTICK_PERIOD_MS);
//															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
//														}
//
//													}else{
//
//														FDSTATUS.PrintTransError = true;
//													}
//												}
//
//												cJSON_Delete(CheckResponse);
//
//											}else{
//
//												FDSTATUS.PrintTransError = true;
//											}
//
//											if(FDSTATUS.PrintTransError == true){
//
//												FDSTATUS.PrintTransError = false;
//												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//												dw_font_goto(&myfont, 75, 225);
//												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
//											}
//
//
//						            	}else{
//						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//											dw_font_goto(&myfont, 75, 225);
//											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//						            	}
//						            }
//						        }
//
//						        cJSON_Delete(Response);
//
//							}else{
//								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//								dw_font_goto(&myfont, 35, 225);
//								// dw_font_print(&myfont, "                         ");
//								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//
//								if(FDSTATUS.FailtoReset == true){
//									vTaskDelay(3000 / portTICK_RATE_MS);
//									esp_restart();
//
//								}else
//								{
//									FDSTATUS.ReturnHomeScreen = true;
//								}
//							}
//						break;
//
//						case rabbitline_disp:
//							TFT_jpg_image(0, 0, 0, NULL, rabbitline_press_jpg_start, rabbitline_press_jpg_end-rabbitline_press_jpg_start);
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);
//
//							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//							dw_font_goto(&myfont, 75, 225);
//							dw_font_print(&myfont, "กรุณารอสักครู่...");
//
//							if(API_GenQRCode(3, (DevPrices - AllMoney)) == true){
//
//								cJSON *Response = cJSON_Parse(DataResponse);
//
//								char ResRef1[20];
//								char ResRef2[20];
//								char ResPrice[5];
//								char ResQR[200];
//
//								const cJSON *code = NULL;
//        						const cJSON *ref1 = NULL;
//        						const cJSON *ref2 = NULL;
//        						const cJSON *price = NULL;
//        						const cJSON *qr_data = NULL;
//
//						        if (Response == NULL)
//						        {
//						            const char *error_ptr = cJSON_GetErrorPtr();
//						            if (error_ptr != NULL)
//						            {
//						                fprintf(stderr, "Error before: %s\n", error_ptr);
//						            }
//
//						        }else{
//
//						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
//						            if (cJSON_IsString(code) && (code->valuestring != NULL))
//						            {
//						            	if(strcmp(code->valuestring, "E00") == 0){
//
//						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
//							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
//							                {
//							                    memset(ResRef1, 0, sizeof(ResRef1));
//							                    strcpy(ResRef1, ref1->valuestring);
//							                    strcat(ResRef1, DEOF);
//							                }
//
//							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
//							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
//							                {
//							                    memset(ResRef2, 0, sizeof(ResRef2));
//							                    strcpy(ResRef2, ref2->valuestring);
//							                    strcat(ResRef2, DEOF);
//							                }
//
//							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
//							                if (cJSON_IsString(price) && (price->valuestring != NULL))
//							                {
//							                    memset(ResPrice, 0, sizeof(ResPrice));
//							                    strcpy(ResPrice, price->valuestring);
//							                    strcat(ResPrice, DEOF);
//							                }
//
//							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
//							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
//							                {
//							                    memset(ResQR, 0, sizeof(ResQR));
//							                    strcpy(ResQR, qr_data->valuestring);
//							                    strcat(ResQR, DEOF);
//							                }
//
//							                if(API_CheckPayment() == true){
//
//							                	// printf("CheckingResponse : %s\n", CheckingResponse);
//
//							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);
//
//												const cJSON *chcode = NULL;
//				        						const cJSON *transid = NULL;
//				        						const cJSON *price = NULL;
//
//				        						if (CheckResponse == NULL)
//										        {
//										            const char *error_ptr = cJSON_GetErrorPtr();
//										            if (error_ptr != NULL)
//										            {
//										                fprintf(stderr, "Error before: %s\n", error_ptr);
//										                FDSTATUS.PrintTransError = true;
//										            }
//
//										        }else{
//
//					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
//										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
//										            {
//										            	if(strcmp(chcode->valuestring, "E00") == 0){
//
//										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
//											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
//											                    strcpy(OldTransectionID, transid->valuestring);
//											                    strcat(OldTransectionID, DEOF);
//											                    printf("OldTransectionID:%s\n", OldTransectionID);
//
//											                }
//
//											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
//											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
//											                    strcpy(OldPriceQR, price->valuestring);
//											                    strcat(OldPriceQR, DEOF);
//											                    printf("OldPriceQR:%s\n", OldPriceQR);
//
//											                }
//
//											                printf("ResQR=%d\n", strlen(ResQR));
//											                Display_QRcode(47, 135, 5, 7, ResQR);
//
//											                _fg = TFT_BLUE;
//															_bg = TFT_WHITE;
//															TFT_setFont(DEJAVU18_FONT, NULL);
//
//															char dt[100];
//
//															sprintf(dt, "REF1:%s", ResRef1);
//															TFT_print(dt, CENTER, 380);
//
//															sprintf(dt, "REF2:%s", ResRef2);
//															TFT_print(dt, CENTER, 405);
//
//															ReduceTime = true;
//															WaittingTime = 60;
//															FDSTATUS.StartCheckPayment = true;
//															vTaskDelay(250/portTICK_PERIOD_MS);
//															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
//														}
//
//													}else{
//
//														FDSTATUS.PrintTransError = true;
//													}
//												}
//
//												cJSON_Delete(CheckResponse);
//
//											}else{
//
//												FDSTATUS.PrintTransError = true;
//											}
//
//											if(FDSTATUS.PrintTransError == true){
//
//												FDSTATUS.PrintTransError = false;
//												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//												dw_font_goto(&myfont, 75, 225);
//												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
//											}
//
//
//						            	}else{
//						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//											dw_font_goto(&myfont, 75, 225);
//											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//						            	}
//						            }
//						        }
//
//						        cJSON_Delete(Response);
//
//							}else{
//								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//								dw_font_goto(&myfont, 35, 225);
//								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//							}
//						break;
//
//						case bluepay_disp:
//							TFT_jpg_image(0, 0, 0, NULL, bluepay_press_jpg_start, bluepay_press_jpg_end-bluepay_press_jpg_start);
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);
//
//							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//							dw_font_goto(&myfont, 75, 225);
//							dw_font_print(&myfont, "กรุณารอสักครู่...");
//
//							if(API_GenQRCode(5, (DevPrices - AllMoney)) == true){
//
//								cJSON *Response = cJSON_Parse(DataResponse);
//
//								char ResRef1[20];
//								char ResRef2[20];
//								char ResPrice[5];
//								char ResQR[200];
//
//								const cJSON *code = NULL;
//        						const cJSON *ref1 = NULL;
//        						const cJSON *ref2 = NULL;
//        						const cJSON *price = NULL;
//        						const cJSON *qr_data = NULL;
//
//						        if (Response == NULL)
//						        {
//						            const char *error_ptr = cJSON_GetErrorPtr();
//						            if (error_ptr != NULL)
//						            {
//						                fprintf(stderr, "Error before: %s\n", error_ptr);
//						            }
//
//						        }else{
//
//						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
//						            if (cJSON_IsString(code) && (code->valuestring != NULL))
//						            {
//						            	if(strcmp(code->valuestring, "E00") == 0){
//
//						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
//							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
//							                {
//							                    memset(ResRef1, 0, sizeof(ResRef1));
//							                    strcpy(ResRef1, ref1->valuestring);
//							                    strcat(ResRef1, DEOF);
//							                }
//
//							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
//							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
//							                {
//							                    memset(ResRef2, 0, sizeof(ResRef2));
//							                    strcpy(ResRef2, ref2->valuestring);
//							                    strcat(ResRef2, DEOF);
//							                }
//
//							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
//							                if (cJSON_IsString(price) && (price->valuestring != NULL))
//							                {
//							                    memset(ResPrice, 0, sizeof(ResPrice));
//							                    strcpy(ResPrice, price->valuestring);
//							                    strcat(ResPrice, DEOF);
//							                }
//
//							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
//							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
//							                {
//							                    memset(ResQR, 0, sizeof(ResQR));
//							                    strcpy(ResQR, qr_data->valuestring);
//							                    strcat(ResQR, DEOF);
//							                }
//
//							                if(API_CheckPayment() == true){
//
//							                	// printf("CheckingResponse : %s\n", CheckingResponse);
//
//							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);
//
//												const cJSON *chcode = NULL;
//				        						const cJSON *transid = NULL;
//				        						const cJSON *price = NULL;
//
//				        						if (CheckResponse == NULL)
//										        {
//										            const char *error_ptr = cJSON_GetErrorPtr();
//										            if (error_ptr != NULL)
//										            {
//										                fprintf(stderr, "Error before: %s\n", error_ptr);
//										                FDSTATUS.PrintTransError = true;
//										            }
//
//										        }else{
//
//					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
//										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
//										            {
//										            	if(strcmp(chcode->valuestring, "E00") == 0){
//
//										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
//											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
//											                    strcpy(OldTransectionID, transid->valuestring);
//											                    strcat(OldTransectionID, DEOF);
//											                    printf("OldTransectionID:%s\n", OldTransectionID);
//
//											                }
//
//											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
//											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
//											                {
//											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
//											                    strcpy(OldPriceQR, price->valuestring);
//											                    strcat(OldPriceQR, DEOF);
//											                    printf("OldPriceQR:%s\n", OldPriceQR);
//
//											                }
//
//											                printf("ResQR=%d\n", strlen(ResQR));
//											                Display_QRcode(47, 135, 5, 7, ResQR);
//
//											                _fg = TFT_BLUE;
//															_bg = TFT_WHITE;
//															TFT_setFont(DEJAVU18_FONT, NULL);
//
//															char dt[100];
//
//															sprintf(dt, "REF1:%s", ResRef1);
//															TFT_print(dt, CENTER, 380);
//
//															sprintf(dt, "REF2:%s", ResRef2);
//															TFT_print(dt, CENTER, 405);
//
//															ReduceTime = true;
//															WaittingTime = 60;
//															FDSTATUS.StartCheckPayment = true;
//															vTaskDelay(250/portTICK_PERIOD_MS);
//															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
//														}
//
//													}else{
//
//														FDSTATUS.PrintTransError = true;
//													}
//												}
//
//												cJSON_Delete(CheckResponse);
//
//											}else{
//
//												FDSTATUS.PrintTransError = true;
//											}
//
//											if(FDSTATUS.PrintTransError == true){
//
//												FDSTATUS.PrintTransError = false;
//												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//												dw_font_goto(&myfont, 75, 225);
//												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
//											}
//
//
//						            	}else{
//						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//											dw_font_goto(&myfont, 75, 225);
//											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//						            	}
//						            }
//						        }
//
//						        cJSON_Delete(Response);
//
//							}else{
//								TFT_resetclipwin();
//								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
//
//								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
//								dw_font_goto(&myfont, 35, 225);
//								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
//							}
//						break;
//					}
//
//					Changed_Display = false;
//				}
//
//				if(ReduceTime == true){
//					if(FDSTATUS.PaymentSuccess == false){
//
//						if(++CounterTime >= 1000){
//							CounterTime = 0;
//
//							if(--WaittingTime > 0){
//								_fg = TFT_RED;
//								_bg = TFT_WHITE;
//								TFT_setFont(DEJAVU18_FONT, NULL);
//
//								char TimeStr[10];
//
//								memset(TimeStr, 0, sizeof(TimeStr));
//
//								sprintf(TimeStr, "%02d Sec", WaittingTime);
//								TFT_print(TimeStr, CENTER, 110);
//								printf("WaittingTime:%d\n", WaittingTime);
//
//							}else{
//								ReduceTime = false;
//								WaittingTime = 0;
//								PageDisplay = homepage;
//								OldDisplayMode = 0xFF;
//
//								TFT_fillScreen(TFT_BLACK);
//								TFT_resetclipwin();
//
//								TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);
//
//								TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);
//
//								TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
//
//								TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
//
//								TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
//
//								TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//
//								TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
//
//								FDSTATUS.StartCheckPayment = false;
//								vTaskDelete(ChPayment);
//							}
//						}
//
//					}else{
//
//						ReduceTime = false;
//						WaittingTime = 0;
//						PageDisplay = homepage;
//						OldDisplayMode = 0xFF;
//
//						TFT_fillScreen(TFT_BLACK);
//						TFT_resetclipwin();
//
//						TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);
//
//						TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);
//
//						TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
//
//						TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
//
//						TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
//
//						TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//
//						TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
//
//						FDSTATUS.StartCheckPayment = false;
//						FDSTATUS.PaymentSuccess = false;
//						vTaskDelete(ChPayment);
//					}
//				}else{
//					CounterTime = 0;
//					WaittingTime = 0;
//					FDSTATUS.StartCheckPayment = false;
//
//				}
//
//				if(gpio_get_level(SWSELECT_PIN) == 0){
//					if(SelectSW_Push == false){
//						if(++SelectPush_Count > 50){
//							ReduceTime = false;
//
//							CounterTime = 0;
//							WaittingTime = 0;
//
//							SelectPush_Count = 0;
//
//							SelectSW_Push = true;
//							PageDisplay = homepage;
//							OldDisplayMode = 0xFF;
//
//							if(FDSTATUS.StartCheckPayment == true){
//								FDSTATUS.StartCheckPayment = false;
//								FDSTATUS.PaymentSuccess = false;
//								vTaskDelete(ChPayment);
//							}
//
//							TFT_fillScreen(TFT_BLACK);
//							TFT_resetclipwin();
//
//							TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);
//
//							TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);
//
//							TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
//
//							TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
//
//							TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
//
//							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
//
//							TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
//						}
//					}
//					SelectReld_Count = 0;
//
//				}else{
//					if(SelectSW_Push == true){
//						if(++SelectReld_Count > 50){
//							SelectReld_Count = 0;
//
//							SelectSW_Push = false;
//						}
//					}
//					SelectPush_Count = 0;
//				}
//
//			break;
//		}
//	}
//}


//===============
static void TFT_Display(void *pvParameters)
{
	font_rotate = 0;
	text_wrap = 0;
	font_transparent = 0;
	font_forceFixed = 0;
	TFT_resetclipwin();

	image_debug = 0;

    char dtype[16];
    
    switch (tft_disp_type) {
        case DISP_TYPE_ILI9341:
            sprintf(dtype, "ILI9341");
            break;
        case DISP_TYPE_ILI9488:
            sprintf(dtype, "ILI9488");
            break;
        case DISP_TYPE_ST7789V:
            sprintf(dtype, "ST7789V");
            break;
        case DISP_TYPE_ST7735:
            sprintf(dtype, "ST7735");
            break;
        case DISP_TYPE_ST7735R:
            sprintf(dtype, "ST7735R");
            break;
        case DISP_TYPE_ST7735B:
            sprintf(dtype, "ST7735B");
            break;
        default:
            sprintf(dtype, "Unknown");
    }

	PageDisplay = payment;
	FDSTATUS.printLCD = false;

	while (1) {

		vTaskDelay(10 / portTICK_PERIOD_MS);

		switch(PageDisplay){
			

			/***************************************************************/
			/***************************************************************/
			case payment:

				if(FDSTATUS.Insert1Coil == true){
					FDSTATUS.Insert1Coil = false;

				//	printf("THIS Page 111111 Display\n");

					char Money[10];
					int8_t InsertMoney = DevPrices - AllMoney;

					if(InsertMoney > 0){
						sprintf(Money, "%02d", InsertMoney);
						TFT_fillRect(0, 55, 320, 300, TFT_WHITE);
						TFT_print(Money, CENTER, 75);

						dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
						dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
						dw_font_goto(&myfont, 75, 275);
						dw_font_print(&myfont, "บาท");


					}else{
						TFT_fillRect(0, 55, 320, 300, TFT_WHITE);
						TFT_print("00", CENTER, 75);


					}

					FDSTATUS.printLCD = true;

				}else{
					if(FDSTATUS.printLCD == false){
						printf("THIS Page Display\n");

						TFT_fillScreen(TFT_WHITE);
						TFT_resetclipwin();



						_fg = TFT_RED;
						_bg = TFT_WHITE;
						TFT_setFont(DSDIGI_FONT, NULL);
						TFT_resetclipwin();

						TFT_jpg_image(0, 0, 0, NULL, header_coins_jpg_start, header_coins_jpg_end-header_coins_jpg_start);
						TFT_jpg_image(0, 417, 0, NULL, footer_coins_jpg_start, footer_coins_jpg_end-footer_coins_jpg_start);

						char Money[10];
						int8_t InsertMoney = DevPrices - AllMoney;

						if(InsertMoney > 0){
							sprintf(Money, "%02d", InsertMoney);
							TFT_resetclipwin();
							TFT_fillRect(10, 55, 300, 300, TFT_WHITE);
							TFT_print(Money, CENTER, 75);
							TFT_resetclipwin();

							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
							dw_font_goto(&myfont, 75, 275);
							dw_font_print(&myfont, "บาท");


						}else{
							_fg = TFT_RED;
							_bg = TFT_WHITE;
							TFT_setFont(DSDIGI_FONT, NULL);
							TFT_resetclipwin();
							TFT_fillRect(0, 55, 320, 300, TFT_WHITE);
							TFT_print(" ", CENTER, 75);
							TFT_resetclipwin();



						}

						FDSTATUS.printLCD = true;
					}
				}

				if(gpio_get_level(SWSELECT_PIN) == 0){
					if(SelectSW_Push == false){
						if(++SelectPush_Count > 3){
							SelectPush_Count = 0;

							SelectSW_Push = true;
							PageDisplay = homepage;
							DisplayMode = promppay_disp;

							TFT_fillScreen(TFT_WHITE);
							TFT_resetclipwin();

							TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);

							TFT_jpg_image(0, 49, 0, NULL, promppay_press_jpg_start, promppay_press_jpg_end-promppay_press_jpg_start);

							TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);

							TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);

							TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);

							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);

							TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);

							TFT_resetclipwin();
						}
					}
					SelectReld_Count = 0;

				}else{
					if(SelectSW_Push == true){
						if(++SelectReld_Count > 3){
							SelectReld_Count = 0;

							SelectSW_Push = false;
						}
					}
					SelectPush_Count = 0;
				}

			break;

			case running:
				if(gpio_get_level(COIN_POWER_IN) == 0){
					// printf("ON Coins\n");
					if(FDSTATUS.MachineisRun == true){

						if(++RunningCnt > 3){
							QR_WaitTime = 0;
							PageDisplay = payment;
							FDSTATUS.printLCD = false;
							AllMoney = 0;
							FDSTATUS.MachineisRun = false;
							RunningCnt = 0;
						}
						StopCnt = 0;
					}

				}else{
					if(FDSTATUS.MachineisRun == false){
						if(++StopCnt > 3){


							StopCnt = 0;
							//TFT_fillRect(0, 0, 320, 50, TFT_ORANGE);
							TFT_resetclipwin();
							TFT_fillRect(0, 0, 320, 480, TFT_WHITE);
							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
							dw_font_goto(&myfont, 75, 225);
							dw_font_print(&myfont, "เครื่องกำลังทำงาน");
							dw_font_goto(&myfont, 75, 275);
							dw_font_print(&myfont, "    กรุณารอ....");
							FDSTATUS.MachineisRun = true;
							TFT_resetclipwin();
						}
						RunningCnt = 0;
					}
				}
			break;

			/***************************************************************/
			/***************************************************************/
			case homepage:

				if(gpio_get_level(SWSELECT_PIN) == 0){
					if(SelectSW_Push == false){
						if(++SelectPush_Count > 3){
							SelectPush_Count = 0;

							SelectSW_Push = true;
							TFT_jpg_image(0, 438, 0, NULL, bottom1_press_jpg_start, bottom1_press_jpg_end-bottom1_press_jpg_start);

							if(++DisplayMode > 3){
								DisplayMode = 0;
							}

							QR_WaitTime = 0;
						}
					}
					SelectReld_Count = 0;

				}else{
					if(SelectSW_Push == true){
						if(++SelectReld_Count > 3){
							SelectReld_Count = 0;

							SelectSW_Push = false;
							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
							// QR_WaitTime = 0;
						}
					}
					SelectPush_Count = 0;
				}


				switch(DisplayMode){
					case promppay_disp:
						if(OldDisplayMode != promppay_disp){
							OldDisplayMode = promppay_disp;
							TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);
							TFT_jpg_image(0, 49, 0, NULL, promppay_press_jpg_start, promppay_press_jpg_end-promppay_press_jpg_start);
						}
					break;

					case truemoney_disp:
						if(OldDisplayMode != truemoney_disp){
							OldDisplayMode = truemoney_disp;
							TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);
							TFT_jpg_image(0, 141, 0, NULL, truemoney_press_jpg_start, truemoney_press_jpg_end-truemoney_press_jpg_start);
						}
					break;

					case rabbitline_disp:
						if(OldDisplayMode != rabbitline_disp){
							OldDisplayMode = rabbitline_disp;
							TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);
							TFT_jpg_image(0, 239, 0, NULL, rabbitline_press_jpg_start, rabbitline_press_jpg_end-rabbitline_press_jpg_start);
						}
					break;

					case bluepay_disp:
						if(OldDisplayMode != bluepay_disp){
							OldDisplayMode = bluepay_disp;
							TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);
							TFT_jpg_image(0, 331, 0, NULL, bluepay_press_jpg_start, bluepay_press_jpg_end-bluepay_press_jpg_start);
						}
					break;
				}


				if(gpio_get_level(SWPAIDS_PIN) == 0){
					if(PaidSW_Push == false){
						if(++PaidPush_Count > 3){
							PaidPush_Count = 0;

							PaidSW_Push = true;
							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);
							PageDisplay = qrpage;
							Changed_Display = true;

							TFT_fillScreen(TFT_WHITE);
							TFT_resetclipwin();
							QR_WaitTime = 0;
						}
					}
					PaidReld_Count = 0;

				}else{
					if(PaidSW_Push == true){
						if(++PaidReld_Count > 3){
							PaidReld_Count = 0;

							PaidSW_Push = false;
							TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
							// QR_WaitTime = 0;
						}
					}
					PaidPush_Count = 0;
				}

				// if(FDSTATUS.QR_StartWait == true){
				// 	if()
				// }
				if(++QR_WaitTime >= 1000){
					QR_WaitTime = 0;
					PageDisplay = payment;
					FDSTATUS.printLCD = false;
				}

			break;


			/***************************************************************/
			/***************************************************************/
			case qrpage:
				if(Changed_Display == true){
					switch(DisplayMode){
						case promppay_disp:
							TFT_jpg_image(0, 0, 0, NULL, promppay_press_jpg_start, promppay_press_jpg_end-promppay_press_jpg_start);
							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);

							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
							dw_font_goto(&myfont, 75, 225);
							dw_font_print(&myfont, "กรุณารอสักครู่...");

							if(API_GenQRCode(2, (DevPrices - AllMoney)) == true)
							{
								printf("prompay");
								cJSON *Response = cJSON_Parse(DataResponse);

								char ResRef1[20];
								char ResRef2[20];
								char ResPrice[5];
								char ResQR[500];


								const cJSON *code = NULL;
        						const cJSON *ref1 = NULL;
        						const cJSON *ref2 = NULL;
        						const cJSON *price = NULL;
        						const cJSON *qr_data = NULL;

						        if (Response == NULL)
						        {
						            const char *error_ptr = cJSON_GetErrorPtr();
						            if (error_ptr != NULL)
						            {
						                fprintf(stderr, "Error before: %s\n", error_ptr);
						            }

						        }else{

						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
						            if (cJSON_IsString(code) && (code->valuestring != NULL))
						            {
						            	if(strcmp(code->valuestring, "E00") == 0){

						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
							                {
							                    memset(ResRef1, 0, sizeof(ResRef1));
							                    strcpy(ResRef1, ref1->valuestring);
							                    strcat(ResRef1, DEOF);
							                    printf("ResRef1=%s",ResRef1);

							                }

							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
							                {
							                    memset(ResRef2, 0, sizeof(ResRef2));
							                    strcpy(ResRef2, ref2->valuestring);
							                    strcat(ResRef2, DEOF);
							                    printf("ResRef2=%s",ResRef2);

							                }

							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
							                if (cJSON_IsString(price) && (price->valuestring != NULL))
							                {
							                    memset(ResPrice, 0, sizeof(ResPrice));
							                    strcpy(ResPrice, price->valuestring);
							                    strcat(ResPrice, DEOF);
							                    printf("price=%s",ResPrice);

							                }

							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
							                {
							                    memset(ResQR, 0, sizeof(ResQR));
							                    strcpy(ResQR, qr_data->valuestring);
							                    strcat(ResQR, DEOF);
							                    printf("ResQR=%s",ResQR);
							                }


							                if (API_CheckPayment() == true)
							                {

							                	// printf("CheckingResponse : %s\n", CheckingResponse);
							                	//printf("Payment=true");
							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);

												const cJSON *chcode = NULL;
				        						const cJSON *transid = NULL;
				        						const cJSON *price = NULL;

				        						if (CheckResponse == NULL)
										        {
				        							//printf("Response == NULL");
										            const char *error_ptr = cJSON_GetErrorPtr();
										            if (error_ptr != NULL)
										            {
										                fprintf(stderr, "Error before: %s\n", error_ptr);
										                FDSTATUS.PrintTransError = true;
										            }

										        }else{
										        	//printf("Response != NULL");
					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
										            {
										            	if(strcmp(chcode->valuestring, "E00") == 0){

										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
											                if (cJSON_IsString(transid) /*&& (transid->valuestring != NULL)*/)
											                {
											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
											                    strcpy(OldTransectionID, transid->valuestring);
											                    strcat(OldTransectionID, DEOF);
											                    printf("OldTransectionID:%s\n", OldTransectionID);

											                }

											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
											                if (cJSON_IsString(price) /*&& (transid->valuestring != NULL)*/)
											                {
											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
											                    strcpy(OldPriceQR, price->valuestring);
											                    strcat(OldPriceQR, DEOF);
											                    printf("OldPriceQR:%s\n", OldPriceQR);

											                }

											                printf("ResQR=%d\n", strlen(ResQR));
											                Display_QRcode(45, 135, 4, 10, ResQR);

											                _fg = TFT_BLUE;
															_bg = TFT_WHITE;
															TFT_setFont(DEJAVU18_FONT, NULL);

															char dt[100];

															sprintf(dt, "REF1:%s", ResRef1);
															TFT_print(dt, CENTER, 380);

															sprintf(dt, "REF2:%s", ResRef2);
															TFT_print(dt, CENTER, 405);

															ReduceTime = true;
															WaittingTime = 60;
															FDSTATUS.StartCheckPayment = true;
															vTaskDelay(250/portTICK_PERIOD_MS);
															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
														}

													}else{

														FDSTATUS.PrintTransError = true;
													}
												}

												cJSON_Delete(CheckResponse);

											}else{

												FDSTATUS.PrintTransError = true;
											}

											if(FDSTATUS.PrintTransError == true){

												FDSTATUS.PrintTransError = false;

												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
												dw_font_goto(&myfont, 75, 225);
												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
											}


						            	}else{
						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
											dw_font_goto(&myfont, 75, 225);
											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
						            	}
						            }
						        }

						        cJSON_Delete(Response);

							}else{
								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
								dw_font_goto(&myfont, 75, 225);
								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");

								if(FDSTATUS.FailtoReset == true){
									vTaskDelay(3000 / portTICK_RATE_MS);
									esp_restart();
								
								}else
								{
									FDSTATUS.ReturnHomeScreen = true;
								}
								
								// else{
								// 	vTaskDelay(3000 / portTICK_RATE_MS);
								// 	// ReturntoHome();
								// }
							}
						break;

						case truemoney_disp:
							TFT_jpg_image(0, 0, 0, NULL, truemoney_press_jpg_start, truemoney_press_jpg_end-truemoney_press_jpg_start);
							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);

							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
							dw_font_goto(&myfont, 75, 225);
							dw_font_print(&myfont, "กรุณารอสักครู่...");

							if(API_GenQRCode(6, (DevPrices - AllMoney)) == true){

								cJSON *Response = cJSON_Parse(DataResponse);

								char ResRef1[20];
								char ResRef2[20];
								char ResPrice[5];
								char ResQR[200];

								const cJSON *code = NULL;
        						const cJSON *ref1 = NULL;
        						const cJSON *ref2 = NULL;
        						const cJSON *price = NULL;
        						const cJSON *qr_data = NULL;

						        if (Response == NULL)
						        {
						            const char *error_ptr = cJSON_GetErrorPtr();
						            if (error_ptr != NULL)
						            {
						                fprintf(stderr, "Error before: %s\n", error_ptr);
						            }

						        }else{

						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
						            if (cJSON_IsString(code) && (code->valuestring != NULL))
						            {
						            	if(strcmp(code->valuestring, "E00") == 0){

						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
							                {
							                    memset(ResRef1, 0, sizeof(ResRef1));
							                    strcpy(ResRef1, ref1->valuestring);
							                    strcat(ResRef1, DEOF);
							                }

							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
							                {
							                    memset(ResRef2, 0, sizeof(ResRef2));
							                    strcpy(ResRef2, ref2->valuestring);
							                    strcat(ResRef2, DEOF);
							                }

							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
							                if (cJSON_IsString(price) && (price->valuestring != NULL))
							                {
							                    memset(ResPrice, 0, sizeof(ResPrice));
							                    strcpy(ResPrice, price->valuestring);
							                    strcat(ResPrice, DEOF);
							                }

							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
							                {
							                    memset(ResQR, 0, sizeof(ResQR));
							                    strcpy(ResQR, qr_data->valuestring);
							                    strcat(ResQR, DEOF);
							                }

							                if(API_CheckPayment() == true){

							                	// printf("CheckingResponse : %s\n", CheckingResponse);

							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);

												const cJSON *chcode = NULL;
				        						const cJSON *transid = NULL;
				        						const cJSON *price = NULL;

				        						if (CheckResponse == NULL)
										        {
										            const char *error_ptr = cJSON_GetErrorPtr();
										            if (error_ptr != NULL)
										            {
										                fprintf(stderr, "Error before: %s\n", error_ptr);
										                FDSTATUS.PrintTransError = true;
										            }

										        }else{

					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
										            {
										            	if(strcmp(chcode->valuestring, "E00") == 0){

										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
											                {
											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
											                    strcpy(OldTransectionID, transid->valuestring);
											                    strcat(OldTransectionID, DEOF);
											                    printf("OldTransectionID:%s\n", OldTransectionID);

											                }

											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
											                {
											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
											                    strcpy(OldPriceQR, price->valuestring);
											                    strcat(OldPriceQR, DEOF);
											                    printf("OldPriceQR:%s\n", OldPriceQR);

											                }

											                printf("ResQR=%d\n", strlen(ResQR));
											                Display_QRcode(47, 135, 5, 7, ResQR);

											                _fg = TFT_BLUE;
															_bg = TFT_WHITE;
															TFT_setFont(DEJAVU18_FONT, NULL);

															char dt[100];

															sprintf(dt, "REF1:%s", ResRef1);
															TFT_print(dt, CENTER, 380);

															sprintf(dt, "REF2:%s", ResRef2);
															TFT_print(dt, CENTER, 405);

															ReduceTime = true;
															WaittingTime = 60;
															FDSTATUS.StartCheckPayment = true;
															vTaskDelay(250/portTICK_PERIOD_MS);
															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
														}

													}else{

														FDSTATUS.PrintTransError = true;
													}
												}

												cJSON_Delete(CheckResponse);

											}else{

												FDSTATUS.PrintTransError = true;
											}

											if(FDSTATUS.PrintTransError == true){

												FDSTATUS.PrintTransError = false;
												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
												dw_font_goto(&myfont, 75, 225);
												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
											}


						            	}else{
						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
											dw_font_goto(&myfont, 75, 225);
											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
						            	}
						            }
						        }

						        cJSON_Delete(Response);

							}else{
								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
								dw_font_goto(&myfont, 35, 225);
								// dw_font_print(&myfont, "                         ");
								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");

								if(FDSTATUS.FailtoReset == true){
									vTaskDelay(3000 / portTICK_RATE_MS);
									esp_restart();
								
								}else
								{
									FDSTATUS.ReturnHomeScreen = true;
								}
							}
						break;

						case rabbitline_disp:
							TFT_jpg_image(0, 0, 0, NULL, rabbitline_press_jpg_start, rabbitline_press_jpg_end-rabbitline_press_jpg_start);
							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);

							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
							dw_font_goto(&myfont, 75, 225);
							dw_font_print(&myfont, "กรุณารอสักครู่...");

							if(API_GenQRCode(3, (DevPrices - AllMoney)) == true){

								cJSON *Response = cJSON_Parse(DataResponse);

								char ResRef1[20];
								char ResRef2[20];
								char ResPrice[5];
								char ResQR[200];

								const cJSON *code = NULL;
        						const cJSON *ref1 = NULL;
        						const cJSON *ref2 = NULL;
        						const cJSON *price = NULL;
        						const cJSON *qr_data = NULL;

						        if (Response == NULL)
						        {
						            const char *error_ptr = cJSON_GetErrorPtr();
						            if (error_ptr != NULL)
						            {
						                fprintf(stderr, "Error before: %s\n", error_ptr);
						            }

						        }else{

						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
						            if (cJSON_IsString(code) && (code->valuestring != NULL))
						            {
						            	if(strcmp(code->valuestring, "E00") == 0){

						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
							                {
							                    memset(ResRef1, 0, sizeof(ResRef1));
							                    strcpy(ResRef1, ref1->valuestring);
							                    strcat(ResRef1, DEOF);
							                }

							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
							                {
							                    memset(ResRef2, 0, sizeof(ResRef2));
							                    strcpy(ResRef2, ref2->valuestring);
							                    strcat(ResRef2, DEOF);
							                }

							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
							                if (cJSON_IsString(price) && (price->valuestring != NULL))
							                {
							                    memset(ResPrice, 0, sizeof(ResPrice));
							                    strcpy(ResPrice, price->valuestring);
							                    strcat(ResPrice, DEOF);
							                }

							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
							                {
							                    memset(ResQR, 0, sizeof(ResQR));
							                    strcpy(ResQR, qr_data->valuestring);
							                    strcat(ResQR, DEOF);
							                }

							                if(API_CheckPayment() == true){

							                	// printf("CheckingResponse : %s\n", CheckingResponse);

							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);

												const cJSON *chcode = NULL;
				        						const cJSON *transid = NULL;
				        						const cJSON *price = NULL;

				        						if (CheckResponse == NULL)
										        {
										            const char *error_ptr = cJSON_GetErrorPtr();
										            if (error_ptr != NULL)
										            {
										                fprintf(stderr, "Error before: %s\n", error_ptr);
										                FDSTATUS.PrintTransError = true;
										            }

										        }else{

					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
										            {
										            	if(strcmp(chcode->valuestring, "E00") == 0){

										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
											                {
											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
											                    strcpy(OldTransectionID, transid->valuestring);
											                    strcat(OldTransectionID, DEOF);
											                    printf("OldTransectionID:%s\n", OldTransectionID);

											                }

											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
											                {
											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
											                    strcpy(OldPriceQR, price->valuestring);
											                    strcat(OldPriceQR, DEOF);
											                    printf("OldPriceQR:%s\n", OldPriceQR);

											                }

											                printf("ResQR=%d\n", strlen(ResQR));
											                Display_QRcode(47, 135, 5, 7, ResQR);

											                _fg = TFT_BLUE;
															_bg = TFT_WHITE;
															TFT_setFont(DEJAVU18_FONT, NULL);

															char dt[100];

															sprintf(dt, "REF1:%s", ResRef1);
															TFT_print(dt, CENTER, 380);

															sprintf(dt, "REF2:%s", ResRef2);
															TFT_print(dt, CENTER, 405);

															ReduceTime = true;
															WaittingTime = 60;
															FDSTATUS.StartCheckPayment = true;
															vTaskDelay(250/portTICK_PERIOD_MS);
															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
														}

													}else{

														FDSTATUS.PrintTransError = true;
													}
												}

												cJSON_Delete(CheckResponse);

											}else{

												FDSTATUS.PrintTransError = true;
											}

											if(FDSTATUS.PrintTransError == true){

												FDSTATUS.PrintTransError = false;
												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
												dw_font_goto(&myfont, 75, 225);
												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
											}


						            	}else{
						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
											dw_font_goto(&myfont, 75, 225);
											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
						            	}
						            }
						        }

						        cJSON_Delete(Response);

							}else{
								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
								dw_font_goto(&myfont, 35, 225);
								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
							}
						break;

						case bluepay_disp:
							TFT_jpg_image(0, 0, 0, NULL, bluepay_press_jpg_start, bluepay_press_jpg_end-bluepay_press_jpg_start);
							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);
							TFT_jpg_image(160, 438, 0, NULL, bottom2_press_jpg_start, bottom2_press_jpg_end-bottom2_press_jpg_start);

							dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);
							dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
							dw_font_goto(&myfont, 75, 225);
							dw_font_print(&myfont, "กรุณารอสักครู่...");

							if(API_GenQRCode(5, (DevPrices - AllMoney)) == true){

								cJSON *Response = cJSON_Parse(DataResponse);

								char ResRef1[20];
								char ResRef2[20];
								char ResPrice[5];
								char ResQR[200];

								const cJSON *code = NULL;
        						const cJSON *ref1 = NULL;
        						const cJSON *ref2 = NULL;
        						const cJSON *price = NULL;
        						const cJSON *qr_data = NULL;

						        if (Response == NULL)
						        {
						            const char *error_ptr = cJSON_GetErrorPtr();
						            if (error_ptr != NULL)
						            {
						                fprintf(stderr, "Error before: %s\n", error_ptr);
						            }

						        }else{

						            code = cJSON_GetObjectItemCaseSensitive(Response, "code");
						            if (cJSON_IsString(code) && (code->valuestring != NULL))
						            {
						            	if(strcmp(code->valuestring, "E00") == 0){

						            		ref1 = cJSON_GetObjectItemCaseSensitive(Response, "ref1");
							                if (cJSON_IsString(ref1) && (ref1->valuestring != NULL))
							                {
							                    memset(ResRef1, 0, sizeof(ResRef1));
							                    strcpy(ResRef1, ref1->valuestring);
							                    strcat(ResRef1, DEOF);
							                }

							                ref2 = cJSON_GetObjectItemCaseSensitive(Response, "ref2");
							                if (cJSON_IsString(ref2) && (ref2->valuestring != NULL))
							                {
							                    memset(ResRef2, 0, sizeof(ResRef2));
							                    strcpy(ResRef2, ref2->valuestring);
							                    strcat(ResRef2, DEOF);
							                }

							                price = cJSON_GetObjectItemCaseSensitive(Response, "price");
							                if (cJSON_IsString(price) && (price->valuestring != NULL))
							                {
							                    memset(ResPrice, 0, sizeof(ResPrice));
							                    strcpy(ResPrice, price->valuestring);
							                    strcat(ResPrice, DEOF);
							                }

							                qr_data = cJSON_GetObjectItemCaseSensitive(Response, "qr_data");
							                if (cJSON_IsString(qr_data) && (qr_data->valuestring != NULL))
							                {
							                    memset(ResQR, 0, sizeof(ResQR));
							                    strcpy(ResQR, qr_data->valuestring);
							                    strcat(ResQR, DEOF);
							                }

							                if(API_CheckPayment() == true){

							                	// printf("CheckingResponse : %s\n", CheckingResponse);

							                	cJSON *CheckResponse = cJSON_Parse(CheckingResponse);

												const cJSON *chcode = NULL;
				        						const cJSON *transid = NULL;
				        						const cJSON *price = NULL;

				        						if (CheckResponse == NULL)
										        {
										            const char *error_ptr = cJSON_GetErrorPtr();
										            if (error_ptr != NULL)
										            {
										                fprintf(stderr, "Error before: %s\n", error_ptr);
										                FDSTATUS.PrintTransError = true;
										            }

										        }else{

					        						chcode = cJSON_GetObjectItemCaseSensitive(CheckResponse, "code");
										            if (cJSON_IsString(chcode) && (chcode->valuestring != NULL))
										            {
										            	if(strcmp(chcode->valuestring, "E00") == 0){

										            		transid = cJSON_GetObjectItemCaseSensitive(CheckResponse, "transid");
											                if (cJSON_IsString(transid) && (transid->valuestring != NULL))
											                {
											                    memset(OldTransectionID, 0, sizeof(OldTransectionID));
											                    strcpy(OldTransectionID, transid->valuestring);
											                    strcat(OldTransectionID, DEOF);
											                    printf("OldTransectionID:%s\n", OldTransectionID);

											                }

											                price = cJSON_GetObjectItemCaseSensitive(CheckResponse, "price");
											                if (cJSON_IsString(price) && (transid->valuestring != NULL))
											                {
											                    memset(OldPriceQR, 0, sizeof(OldPriceQR));
											                    strcpy(OldPriceQR, price->valuestring);
											                    strcat(OldPriceQR, DEOF);
											                    printf("OldPriceQR:%s\n", OldPriceQR);

											                }

											                printf("ResQR=%d\n", strlen(ResQR));
											                Display_QRcode(47, 135, 5, 7, ResQR);

											                _fg = TFT_BLUE;
															_bg = TFT_WHITE;
															TFT_setFont(DEJAVU18_FONT, NULL);

															char dt[100];

															sprintf(dt, "REF1:%s", ResRef1);
															TFT_print(dt, CENTER, 380);

															sprintf(dt, "REF2:%s", ResRef2);
															TFT_print(dt, CENTER, 405);

															ReduceTime = true;
															WaittingTime = 60;
															FDSTATUS.StartCheckPayment = true;
															vTaskDelay(250/portTICK_PERIOD_MS);
															xTaskCreate(&CheckPayment, "CheckPayment", 4096, NULL, 4, &ChPayment);
														}

													}else{

														FDSTATUS.PrintTransError = true;
													}
												}

												cJSON_Delete(CheckResponse);

											}else{

												FDSTATUS.PrintTransError = true;
											}

											if(FDSTATUS.PrintTransError == true){

												FDSTATUS.PrintTransError = false;
												dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

												dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
												dw_font_goto(&myfont, 75, 225);
												dw_font_print(&myfont, "กรุณาทำรายการอีกครั้ง");
											}


						            	}else{
						            		dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

											dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
											dw_font_goto(&myfont, 75, 225);
											dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
						            	}
						            }
						        }

						        cJSON_Delete(Response);

							}else{
								TFT_resetclipwin();
								dw_font_init(&myfont, 128, 64, draw_pixel, clear_pixel);

								dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
								dw_font_goto(&myfont, 35, 225);
								dw_font_print(&myfont, "ระงับการใช้งานชั่วคราว");
							}
						break;
					}

					Changed_Display = false;
				}

				if(ReduceTime == true){
					if(FDSTATUS.PaymentSuccess == false){

						if(++CounterTime >= 100){
							CounterTime = 0;

							if(--WaittingTime > 0){
								_fg = TFT_RED;
								_bg = TFT_WHITE;
								TFT_setFont(DEJAVU18_FONT, NULL);

								char TimeStr[10];

								memset(TimeStr, 0, sizeof(TimeStr));

								sprintf(TimeStr, "%02d Sec", WaittingTime);
								TFT_print(TimeStr, CENTER, 110);
								printf("WaittingTime:%d\n", WaittingTime);

							}else{
								ReduceTime = false;
								WaittingTime = 0;
								PageDisplay = homepage;
								OldDisplayMode = 0xFF;

								TFT_fillScreen(TFT_BLACK);
								TFT_resetclipwin();

								TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);

								TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);

								TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);

								TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);

								TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);

								TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);

								TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);

								FDSTATUS.StartCheckPayment = false;
								vTaskDelete(ChPayment);
							}
						}

					}else{

						ReduceTime = false;
						WaittingTime = 0;
						PageDisplay = homepage;
						OldDisplayMode = 0xFF;

						TFT_fillScreen(TFT_BLACK);
						TFT_resetclipwin();

						TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);

						TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);

						TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);

						TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);

						TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);

						TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);

						TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);

						FDSTATUS.StartCheckPayment = false;
						FDSTATUS.PaymentSuccess = false;
						vTaskDelete(ChPayment);
					}
				}else{
					CounterTime = 0;
					WaittingTime = 0;
					FDSTATUS.StartCheckPayment = false;
					
				}

				if(gpio_get_level(SWSELECT_PIN) == 0){
					if(SelectSW_Push == false){
						if(++SelectPush_Count > 5){
							ReduceTime = false;

							CounterTime = 0;
							WaittingTime = 0;

							SelectPush_Count = 0;

							SelectSW_Push = true;
							PageDisplay = homepage;
							OldDisplayMode = 0xFF;

							if(FDSTATUS.StartCheckPayment == true){
								FDSTATUS.StartCheckPayment = false;
								FDSTATUS.PaymentSuccess = false;
								vTaskDelete(ChPayment);
							}

							TFT_fillScreen(TFT_BLACK);
							TFT_resetclipwin();

							TFT_jpg_image(0, 0, 0, NULL, topmenu_jpg_start, topmenu_jpg_end-topmenu_jpg_start);

							TFT_jpg_image(0, 49, 0, NULL, promppay_jpg_start, promppay_jpg_end-promppay_jpg_start);

							TFT_jpg_image(0, 141, 0, NULL, truemoney_jpg_start, truemoney_jpg_end-truemoney_jpg_start);

							TFT_jpg_image(0, 239, 0, NULL, rabbitline_jpg_start, rabbitline_jpg_end-rabbitline_jpg_start);

							TFT_jpg_image(0, 331, 0, NULL, bluepay_jpg_start, bluepay_jpg_end-bluepay_jpg_start);

							TFT_jpg_image(0, 438, 0, NULL, bottom1_jpg_start, bottom1_jpg_end-bottom1_jpg_start);

							TFT_jpg_image(160, 438, 0, NULL, bottom2_jpg_start, bottom2_jpg_end-bottom2_jpg_start);
						}
					}
					SelectReld_Count = 0;

				}else{
					if(SelectSW_Push == true){
						if(++SelectReld_Count > 5){
							SelectReld_Count = 0;

							SelectSW_Push = false;
						}
					}
					SelectPush_Count = 0;
				}

			break;
		}
	}
}


//=============
void Display_Initial(void)
{
    // ========  PREPARE DISPLAY INITIALIZATION  =========
    esp_err_t ret;


    // === SET GLOBAL VARIABLES ==========================
    // ===================================================
	tft_disp_type = DISP_TYPE_ILI9488;


	// ===================================================
	// === Set display resolution if NOT using default ===
	_width = DEFAULT_TFT_DISPLAY_WIDTH;  // smaller dimension
	_height = DEFAULT_TFT_DISPLAY_HEIGHT; // larger dimension


	// ===================================================
	// ==== Set maximum spi clock for display read    ====
	//      operations, function 'find_rd_speed()'    ====
	//      can be used after display initialization  ====
	max_rdclock = 32000000;


    // ====================================================================
    // === Pins MUST be initialized before SPI interface initialization ===
    // ====================================================================
    TFT_PinsInit();


    // ====  CONFIGURE SPI DEVICES(s)  ====================================================================================
    spi_lobo_device_handle_t spi;
	
    spi_lobo_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,				// set SPI MISO pin
        .mosi_io_num=PIN_NUM_MOSI,				// set SPI MOSI pin
        .sclk_io_num=PIN_NUM_CLK,				// set SPI CLK pin
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
		.max_transfer_sz = 6*1024,
    };

    spi_lobo_device_interface_config_t devcfg={
        .clock_speed_hz=33000000,                // Initial clock out at 8 MHz
        .mode=0,                                // SPI mode 0
        .spics_io_num=-1,                       // we will use external CS pin
		.spics_ext_io_num=PIN_NUM_CS,           // external CS pin
		.flags=LB_SPI_DEVICE_HALFDUPLEX,        // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
    };

    // ====================================================================================================================
    vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n==============================\r\n");
    printf("TFT display DEMO, LoBo 11/2017\r\n");
	printf("==============================\r\n");
    printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\r\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);

#if USE_TOUCH > TOUCH_TYPE_NONE
    printf(" Touch CS: %d\r\n", PIN_NUM_TCS);
#endif
	printf("==============================\r\n\r\n");

	// ==================================================================
	// ==== Initialize the SPI bus and attach the LCD to the SPI bus ====
	ret=spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret==ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	disp_spi = spi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(spi, 1);
    assert(ret==ESP_OK);
	ret = spi_lobo_device_deselect(spi);
    assert(ret==ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");

#if USE_TOUCH > TOUCH_TYPE_NONE

	// =====================================================
    // ==== Attach the touch screen to the same SPI bus ====
	ret=spi_lobo_bus_add_device(SPI_BUS, &buscfg, &tsdevcfg, &tsspi);
    assert(ret==ESP_OK);
	printf("SPI: touch screen device added to spi bus (%d)\r\n", SPI_BUS);
	ts_spi = tsspi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(tsspi, 1);
    assert(ret==ESP_OK);
	ret = spi_lobo_device_deselect(tsspi);
    assert(ret==ESP_OK);

	printf("SPI: attached TS device, speed=%u\r\n", spi_lobo_get_speed(tsspi));

#endif

	// ================================
	// ==== Initialize the Display ====
	printf("SPI: display init...\r\n");
	TFT_display_init();
    printf("OK\r\n");
	
	// ---- Detect maximum read speed ----
	max_rdclock = find_rd_speed();
	printf("SPI: Max rd speed = %u\r\n", max_rdclock);

    // ==== Set SPI clock used for display operations ====
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

    printf("\r\n---------------------\r\n");
	printf("Graphics demo started\r\n");
	printf("---------------------\r\n");

	font_rotate = 0;
	text_wrap = 0;
	font_transparent = 0;
	font_forceFixed = 0;
	gray_scale = 0;
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(PORTRAIT);
	TFT_setFont(DEFAULT_FONT, NULL);
	TFT_resetclipwin();

	if(FDSTATUS.HaveWiFiSetting == true){

		// ===== Set time zone ======
		setenv("TZ", "WIB-7", 1);
		tzset();

		// ==========================
		disp_header(DevVersion);

		time(&time_now);
		tm_info = localtime(&time_now);

		// Is time set? If not, tm_year will be (1970 - 1900).
		if (tm_info->tm_year < (2019 - 1900)) {
			ESP_LOGI(tag, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
			_fg = TFT_CYAN;
			TFT_print("Time is not set yet", CENTER, CENTER);
			TFT_print("Connecting to WiFi", CENTER, LASTY+TFT_getfontheight()+2);
			TFT_print("Getting time over NTP", CENTER, LASTY+TFT_getfontheight()+2);
			_fg = TFT_YELLOW;
			TFT_print("Wait", CENTER, LASTY+TFT_getfontheight()+2);
			if (obtain_time()) {
				_fg = TFT_GREEN;
				TFT_print("System time is set.", CENTER, LASTY);
			}
			else {
				_fg = TFT_RED;
				TFT_print("ERROR.", CENTER, LASTY);
			}
			time(&time_now);
			update_header(NULL, "");
			Wait(-2000);
		}

		EventBits_t RTCConBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		if((RTCConBits & WIFI_CONNECTED_BIT) != 0){

			First_Online();

			vTaskDelay(100 / portTICK_RATE_MS);

			// disp_header(DevVersion);
			_fg = TFT_CYAN;
			TFT_print("Initializing SPIFFS...", CENTER, CENTER);

			// ==== Initialize the file system ====
			printf("\r\n\n");

			TFT_fillScreen(TFT_WHITE);
			TFT_resetclipwin();

			//=========
			// Start Run Display
			//=========
			xTaskCreate(&TFT_Display, "TFT_Display", 71680, NULL, 3, &Display);
		}
	}else{
		char *ResQR = (char*) calloc(200, sizeof(char));
		sprintf(ResQR, "%s,%s,%s", MAC_Address, AP_WIFI_SSID, AP_WIFI_PASS);
		TFT_fillScreen(TFT_WHITE);
		TFT_resetclipwin();
		_fg = TFT_RED;
		_bg = TFT_WHITE;
		TFT_setFont(DEJAVU24_FONT, NULL);
		TFT_resetclipwin();
		TFT_print("Register First", CENTER, 50);
		Display_QRcode(47, 120, 5, 7, ResQR);
		_fg = TFT_BLUE;
		_bg = TFT_WHITE;
		TFT_print("Future DTech", CENTER, 400);
		TFT_resetclipwin();
		free(ResQR);
	}
}
