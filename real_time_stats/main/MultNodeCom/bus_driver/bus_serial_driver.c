#include "bus_serial_driver.h"
#include "bus_driver.h"
#include "ringbuffer.h"
#include "bus_type.h"
#include "utilsPrintf.h"
#include "utilsAssert.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <string.h>

#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

MODULE_TAG("SERICL"); 

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

static int serial_init(void *self);
static int serial_open(void *self);
static int serial_close(void *self);
static int serial_write(void *self, uint8_t *buffer, uint16_t length);
static int serial_read(void *self, uint8_t *buffer, uint16_t length);
static int serial_write_buff(void *self, uint8_t *buffer, uint16_t length);
static int serial_read_buff(void *self, uint8_t *buffer, uint16_t length);
static int rb_serial_write_sync(void *self);
static int rb_serial_read_sync(void *self);
static int rb_serial_read_sync(void *self);

static const bus_interface_i bus_serial_interface = {
    .init = serial_init,
    .open = serial_open,
    .close = serial_close,
    .write = serial_write_buff,
    .read = serial_read_buff,
    .sync_rx = rb_serial_read_sync,
    .sync_tx = rb_serial_write_sync
};

static int serial_init(void *self)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, 256 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    return 0;
}

static int serial_open(void *self)
{

    return 0;
}

static int serial_close(void *self)
{
    return 0;
}

static int serial_write(void *self, uint8_t *data, uint16_t length)
{
    int ret = 0;
    // bus_serial_driver_t* dev = self;
    // int fd = dev->bus_device.fd;
    // ret = write(fd, data, length);
    ret = uart_write_bytes(UART_NUM_1, data, length);
    return ret;
}

static int serial_read(void *self, uint8_t *data, uint16_t length)
{
    int ret = 0;
    // bus_serial_driver_t* dev = self;
    // int fd = dev->bus_device.fd;
    // ret = read(fd, data, length);
    ret = uart_read_bytes(UART_NUM_1, data, 128, 2);
    return ret;
}

static int serial_write_buff(void *self, uint8_t *data, uint16_t length)
{
    bus_serial_driver_t* dev = self;
    struct rt_ringbuffer* write_rb = dev->bus_driver->serial_write_rb;
    int write_rb_ret = 0;

    write_rb_ret = rt_ringbuffer_put_force(write_rb, (uint8_t *)data, (uint32_t)length);

    if(write_rb_ret > 0){
        TI_DEBUG_NO_N("serial %s write :",dev->dev_name);
        utils_buff_print(data,length);
    }
    
    return write_rb_ret;
}

static int serial_read_buff(void *self, uint8_t *data, uint16_t length)
{
    bus_serial_driver_t* dev = self;
    struct rt_ringbuffer* read_rb = dev->bus_driver->serial_read_rb;
    int read_rb_ret = 0;

    read_rb_ret = rt_ringbuffer_get(read_rb, (uint8_t *)data, (uint32_t)length);
    
    if(read_rb_ret > 0){
        TI_DEBUG_NO_N("serial %s read:",dev->dev_name);
        utils_buff_print(data,read_rb_ret);
    }
   
    return read_rb_ret;
}


static int rb_serial_write_sync(void *self)
{
    bus_serial_driver_t* dev = self;
    struct rt_ringbuffer* write_rb = dev->bus_driver->serial_write_rb;
    int write_serial_len = 0, write_rb_ret = 0;
    uint8_t recv_buffer[dev->bus_driver->bus_tx_buffer_size];
    memset(recv_buffer, 0, sizeof(recv_buffer));

    write_serial_len = rt_ringbuffer_get(write_rb, (uint8_t *)recv_buffer, (uint32_t)dev->bus_driver->bus_tx_buffer_size);

    if(write_serial_len > 0){
        write_rb_ret = serial_write(dev, (uint8_t *)recv_buffer, write_serial_len);
    }

    if(write_rb_ret != write_serial_len)
        return -1;

    return write_rb_ret;
}

static int rb_serial_read_sync(void *self)
{
    bus_serial_driver_t* dev = self;
    struct rt_ringbuffer* read_rb = dev->bus_driver->serial_read_rb;
    int read_serial_len = 0, serial_read_ret = 0;
    uint8_t recv_buffer[dev->bus_driver->bus_rx_buffer_size];
    memset(recv_buffer, 0, sizeof(recv_buffer));

    read_serial_len = serial_read(dev,recv_buffer, (uint32_t)dev->bus_driver->bus_rx_buffer_size);
    
    if( read_serial_len > 0){
        serial_read_ret = rt_ringbuffer_put_force(read_rb, (uint8_t *)recv_buffer, (uint32_t)read_serial_len);
    }

    if(serial_read_ret != read_serial_len)
        return -1;

    return serial_read_ret;
}

bus_serial_driver_t* bus_serial_driver_register(const char* dev_name, uint8_t bus_id)
{
    
    bus_serial_driver_t* serial_driver = NULL;
    serial_driver = malloc(sizeof(bus_serial_driver_t));

    strncpy(serial_driver->dev_name, dev_name, sizeof(serial_driver->dev_name) - 1);
    serial_driver->dev_name[sizeof(serial_driver->dev_name) - 1] = '\0'; // 确保字符串以空字符结尾

    int8_t ret = bus_driver_register(&serial_driver->bus_driver, &bus_serial_interface, 256, 256, "serial", SERIAL, bus_id); 
    if(ret < 0)
        goto free_serial_driver;

    return serial_driver;

free_serial_driver:
    free(serial_driver);

    return 0;
}