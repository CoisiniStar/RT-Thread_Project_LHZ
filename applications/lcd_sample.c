/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-08-28     WillianChan   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef BSP_USING_SPI_LCD
#include <drv_lcd.h>
#include <rttlogo.h>


static int lcd_sample(void)
{
    /* ���� */
    lcd_clear(WHITE);

    /* ��ʾ RT-Thread logo */
    lcd_show_image(0, 0, 240, 69, image_rttlogo);
    
    /* ���ñ���ɫ��ǰ��ɫ */
    lcd_set_color(WHITE, BLACK);

    /* �� LCD ����ʾ�ַ� */
    lcd_show_string(10, 69, 16, "Hello, RT-Thread!");
    lcd_show_string(10, 69+16, 24, "RT-Thread");
    lcd_show_string(10, 69+16+24, 32, "RT-Thread");
    
    /* �� LCD �ϻ��� */
    lcd_draw_line(0, 69+16+24+32, 240, 69+16+24+32);
    
    /* �� LCD �ϻ�һ��ͬ��Բ */
    lcd_draw_point(120, 194);
    for (int i = 0; i < 46; i += 4)
    {
        lcd_draw_circle(120, 194, i);
    }
    
    return RT_EOK;
}
INIT_APP_EXPORT(lcd_sample);
#endif /* BSP_USING_SPI_LCD */
