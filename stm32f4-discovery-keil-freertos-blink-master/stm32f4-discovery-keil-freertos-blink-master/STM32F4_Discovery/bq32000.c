#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stm32f4xx.h>
#include <stm32f4xx_i2c.h>
#include "discoveryf4utils.h"
#include "rtc.h"
#include "bq32000.h"
#include "uprintf.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
#include "semphr.h"
#include "portmacro.h"

#define BCD2BIN(val) (((val)&15) + ((val)>>4)*10)
#define BIN2BCD(val) ((((val)/10)<<4) + (val)%10)

rtc_bq32000_datetime_t data_time;
uint8_t data_second;
int g_error_code;
unsigned int g_return_value;

int bq32000_set_rtc_data (char register_value, char data) {
    uprintf("bq32000_set_rtc_data A entered\r\n");
    I2C_start(I2C1, (BQ32000_ADDRESS << 1), I2C_Direction_Transmitter);
    uprintf( "bq32000_set_rtc_data B entered\r\n");
    I2C_write(I2C1, 0x00);
    uprintf( "bq32000_set_rtc_data B entered\r\n");
    I2C_write(I2C1, data);
    uprintf("bq32000_set_rtc_data B entered\r\n");
    I2C_stop(I2C1);
    uprintf("bq32000_set_rtc_data C entered\r\n");

    // otherwise success
    return RTC_BQ32000_OK;
}

int bq32000_get_rtc_data (char register_value, char register_mask,
    unsigned int *return_value) {
    uint8_t out_buff[1];
    uint8_t in_buff[1];
    uint32_t status;

    I2C_start(I2C1, (BQ32000_ADDRESS << 1), I2C_Direction_Transmitter);
    I2C_write(I2C1, register_value);
    I2C_stop(I2C1);

    I2C_start(I2C1, (BQ32000_ADDRESS << 1), I2C_Direction_Receiver);
    in_buff[0] = I2C_read_nack(I2C1);
    
    // otherwise success
    *return_value = BCD2BIN(in_buff[0] & register_mask);
    return RTC_BQ32000_OK;
}

void bq32000_set_rtc_second (unsigned int value) {
    bq32000_set_rtc_data(SECOND_REGISTER, (BIN2BCD(value) & SECOND_MASK));
}

void bq32000_set_rtc_minute (unsigned int value) {
    bq32000_set_rtc_data(MINUTE_REGISTER, (BIN2BCD(value) & MINUTE_MASK));
}

void bq32000_set_rtc_hour (unsigned int value) {
    bq32000_set_rtc_data(HOUR_REGISTER, (BIN2BCD(value) & HOUR_MASK));
}

void bq32000_set_rtc_day (unsigned int value) {
    bq32000_set_rtc_data(DAY_REGISTER, (BIN2BCD(value) & DAY_MASK));
}

void bq32000_set_rtc_date (unsigned int value) {
    bq32000_set_rtc_data(DATE_REGISTER, (BIN2BCD(value) & DATE_MASK));
}
void bq32000_set_rtc_month (unsigned int value) {
    bq32000_set_rtc_data(MONTH_REGISTER, (BIN2BCD(value) & MONTH_MASK));
}

void bq32000_set_rtc_year (unsigned int value) {
    bq32000_set_rtc_data(YEAR_REGISTER, (BIN2BCD(value) & YEAR_MASK));
}

int bq32000_set_rtc_datetime (rtc_bq32000_datetime_t* datetime) {
    // Set RTC datetime, the write format is documented on bq32000 pg. 13
    // byte0: word address = 0x00
    // byte1: (seconds)
    // byte2: (minutes)
    // 3: day
    // 4: date (day of month)
    // 5: month
    // 6: year (0-99)
    I2C_start(I2C1, (BQ32000_ADDRESS << 1), I2C_Direction_Transmitter);
    I2C_write(I2C1, 0x00);
    I2C_write(I2C1, BIN2BCD(datetime->seconds) & SECOND_MASK);
    I2C_write(I2C1, BIN2BCD(datetime->minutes) & MINUTE_MASK);
    I2C_write(I2C1, BIN2BCD(datetime->hour) & HOUR_MASK);
    I2C_write(I2C1, BIN2BCD(datetime->day) & DAY_MASK);
    I2C_write(I2C1, BIN2BCD(datetime->date) & DATE_MASK);
    I2C_write(I2C1, BIN2BCD(datetime->month) & MONTH_MASK);
    I2C_write(I2C1, BIN2BCD(datetime->year) & YEAR_MASK);
    I2C_stop(I2C1);

    // otherwise success
    return RTC_BQ32000_OK;
}


int bq32000_get_rtc_second (void) {
    return bq32000_get_rtc_data(SECOND_REGISTER, SECOND_MASK, &g_return_value);
}

int bq32000_get_rtc_minute (void) {
    return bq32000_get_rtc_data(MINUTE_REGISTER, MINUTE_MASK, &g_return_value);
}

int bq32000_get_rtc_hour (void) {
    return bq32000_get_rtc_data(HOUR_REGISTER, HOUR_MASK, &g_return_value);
}

int bq32000_get_rtc_day (void) {
    return bq32000_get_rtc_data(DAY_REGISTER, DAY_MASK, &g_return_value);
}

int bq32000_get_rtc_date (void) {
    return bq32000_get_rtc_data(DATE_REGISTER, DATE_MASK, &g_return_value);
}

int bq32000_get_rtc_month (void) {
    return bq32000_get_rtc_data(MONTH_REGISTER, MONTH_MASK, &g_return_value);
}

int bq32000_get_rtc_year (void) {
    return bq32000_get_rtc_data(YEAR_REGISTER, YEAR_MASK, &g_return_value);
}

int bq32000_get_rtc_datetime (rtc_bq32000_datetime_t *datetime)
{
    // seconds
    if (bq32000_get_rtc_second() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->seconds = g_return_value;

    // minutes
    if (bq32000_get_rtc_minute() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->minutes = g_return_value;

    // hours
    if (bq32000_get_rtc_hour() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->hour = g_return_value;

    // day
    if (bq32000_get_rtc_day() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->day = g_return_value;

    // date
    if (bq32000_get_rtc_date() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->date = g_return_value;

    // month
    if (bq32000_get_rtc_month() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->month = g_return_value;

    // year
    if (bq32000_get_rtc_year() != RTC_BQ32000_OK)
        return RTC_BQ32000_BAD;
    else
        datetime->year = g_return_value;

    // otherwise, success
    return RTC_BQ32000_OK;
}

int bq32000_init_rtc (int first_run_flag) {
    // build the default (zero) starting state for the RTC
    // datetime structure.
    rtc_bq32000_datetime_t init_datetime;

    init_datetime.year = 0;
    init_datetime.month = 0;
    init_datetime.date = 0;
    init_datetime.day = 0;
    init_datetime.hour = 0;
    init_datetime.minutes = 0;
    init_datetime.seconds = 0;


    uprintf("bq32000_init_rtc entered\r\n");
//    if (bq32000_set_rtc_data(CONTROL_REGISTER, 0) == -1) {
//        return RTC_BQ32000_BAD;
//    }

    uprintf("bq32000_set_rtc_data passed\r\n");

    if (first_run_flag) {
        return bq32000_set_rtc_datetime(&init_datetime);
        uprintf("bq32000_set_rtc_datetime passed\r\n");
    }

    // otherwise return 0 to indicate success
    return RTC_BQ32000_OK;
}

void xBQ32000_Read(void *pvParameters)
{
    for(;;)
    {
        bq32000_get_rtc_datetime(&data_time);

        if(data_second != data_time.seconds)
        {
//            uprintf("%d %d %d\r\n",data_time.hour,data_time.minutes ,data_time.seconds);
            data_second = data_time.seconds;
        }
        vTaskDelay( 200 / portTICK_RATE_MS );
    }
}