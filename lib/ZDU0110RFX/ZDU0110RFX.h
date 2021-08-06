/****************************************************************************
 * Class for interfacing with a Zilog ZDU0110RFX module via I2C
 * Module provides a I2C-to-UART bridge
 * 
 * Written for Mbed OS 6 by John M. Larkin (August 2021)
 * 
 * Released under the MIT License (http://opensource.org/licenses/MIT).
 * 	
 * The MIT License (MIT)
 * Copyright (c) 2021 John M. Larkin
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
 * associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *******************************************************************************/

#ifndef ZILOG_ZDU0110RFX_H
#define ZILOG_ZDU0110RFX_H

#include <mbed.h>


namespace Zilog {
  struct fifo_level {
    char tx_bytes;
    char rx_bytes;
  };

  // Registers
  static const uint8_t REG_UART_STATUS = 0x21;             // Read UART status
  static const uint8_t REG_UART_INT_ENABLE = 0x22;         // Enable interrupts
  static const uint8_t REG_UART_INT_STATUS = 0x23;         // Interrupt status
  static const uint8_t REG_WRITE_UART_TX_BUFF = 0x24;      // Write data to tx buffer
  static const uint8_t REG_READ_UART_RX_BUFF = 0x25;       // Read data from rx buffer
  static const uint8_t REG_SET_UART_BAUD = 0x26;           // Set UART baud
  static const uint8_t REG_READ_UART_BAUD = 0x27;          // Read UART baud
  static const uint8_t REG_WRITE_UART_CONFIG = 0x28;       // Write UART config
  static const uint8_t REG_READ_UART_CONFIG = 0x29;        // Read UART config
  static const uint8_t REG_WRITE_TX_WATERMARK = 0x2A;      // Write tx watermark
  static const uint8_t REG_READ_TX_WATERMARK = 0x2B;       // read tx watermark
  static const uint8_t REG_WRITE_RX_WATERMARK = 0x2C;      // write rx watermark
  static const uint8_t REG_READ_RX_WATERMARK = 0x2D;       // read rx watermark
  static const uint8_t REG_ENABLE_UART = 0x2E;             // enable UART
  static const uint8_t REG_READ_FIFO_LVL = 0x31;           // read tx and rx buffer level

  /***** Flags *****/
  // for UART status reads
  static const uint8_t UART_STATUS_TX_EMPTY = 1 << 7;      // transmit buffer is empty
  static const uint8_t UART_STATUS_TX_FULL = 1 << 6;       // transmit buffer is full
  static const uint8_t UART_STATUS_RX_DATA = 1 << 5;       // receive buffer has data (this is what data sheet says but behavior doesn't match)
  static const uint8_t UART_STATUS_INVALID = 1 << 4;       // No valid status bit 4 so use to flag error
  static const uint8_t UART_STATUS_BREAK = 1 << 3;         // break received
  static const uint8_t UART_STATUS_DATA_OVERRUN = 1 << 2;  // data over run error
  static const uint8_t UART_STATUS_FRAME_ERROR = 1 << 1;   // frame error
  static const uint8_t UART_STATUS_PARITY_ERROR = 1;       // parity error
  // for interrupt enables and status
  static const uint8_t INT_TX_EMPTY = 1 << 7;
  static const uint8_t INT_TX_WATERMARK = 1 << 6;
  static const uint8_t INT_TX_FULL = 1 << 5;
  static const uint8_t INT_BREAK = 1 << 4;
  static const uint8_t INT_RX_DATA = 1 << 3;
  static const uint8_t INT_RX_WATERMARK = 1 << 2;
  static const uint8_t INT_RX_FULL = 1 << 1;
  static const uint8_t INT_TX_RX_ERROR = 1;
  // for UART config
  static const uint8_t UART_CONFIG_ALL_AT_ONCE = 1 << 7;     // set to config all parameters at once, otherwise subcommands (set only one)
  static const uint8_t UART_CONFIG_SUB_DATA_BITS = 0x01;     
  static const uint8_t UART_CONFIG_SUB_PARITY = 0x02;
  static const uint8_t UART_CONFIG_SUB_STOP_BITS = 0x03;
  static const uint8_t UART_CONFIG_SUB_FLOW_CTRL = 0x04;
  static const uint8_t UART_CONFIG_SUB_RESET_FIFOS = 0x06;
  static const uint8_t UART_CONFIG_SUB_LOOPBACK = 0x08;
  static const uint8_t UART_CONFIG_SUB_RESET = 0x0A;
  static const uint8_t UART_CONFIG_PARITY_NONE = 0x00;
  static const uint8_t UART_CONFIG_PARITY_ODD = 0x03;
  static const uint8_t UART_CONFIG_PARITY_EVEN = 0x02;
  static const uint8_t UART_CONFIG_DATA_BITS_5 = 0x00;          // 5 bit data
  static const uint8_t UART_CONFIG_DATA_BITS_6 = 0x01;
  static const uint8_t UART_CONFIG_DATA_BITS_7 = 0x02;
  static const uint8_t UART_CONFIG_DATA_BITS_8 = 0x03;
  static const uint8_t UART_CONFIG_DATA_BITS_9 = 0x07;
  static const uint8_t UART_CONFIG_ONE_STOP_BIT = 0;
  static const uint8_t UART_CONFIG_TWO_STOP_BITS = 1;
  static const uint8_t UART_CONFIG_FLOW_CTRL_NONE = 0;
  static const uint8_t UART_CONFIG_FLOW_CTRL_HW = 1;
  static const uint8_t UART_CONFIG_FLOW_CTRL_SW = 2;
  static const uint8_t UART_CONFIG_RESET_TX = 1;
  static const uint8_t UART_CONFIG_RESET_RX = 2;
  static const uint8_t UART_CONFIG_RESET_BOTH = 3;
  static const uint8_t UART_CONFIG_LOOPBACK_OFF = 0;
  static const uint8_t UART_CONFIG_LOOPBACK_ON = 1;
}



class Zilog_SerialBridge
{
public:
  Zilog_SerialBridge(I2C* i2c, uint8_t three_bit_address);
  bool enable_uart(bool tx=true, bool rx=true, int uart=0);
  bool set_baud(int baud, int uart=0);
  int get_baud(int uart=0);
  
  bool send(char* data, int len, int uart=0);
  int recv(char* data, int max_len, int uart=0);

  uint8_t read_uart_status(int uart=0);
  void print_uart_status(int uart=0);
  Zilog::fifo_level get_buffer_level(int uart=0);

  // Variables



private:
  uint8_t reg_for_uart(uint8_t base_reg, int uart=0);
  // Variables
  I2C* _i2c;  // Potentially shared connection to user's chosen I2C hardware
  uint8_t _addr; // Shifted I2C address for commands (from 7 bits to 8 bits)
};


#endif