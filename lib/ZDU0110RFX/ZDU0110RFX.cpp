#include <mbed.h>
#include "ZDU0110RFX.h"

using namespace Zilog;

Zilog_SerialBridge::Zilog_SerialBridge(I2C* i2c, uint8_t three_bit_address) {
  _i2c = i2c;
  _addr = 0xB0 | (three_bit_address<<1);
}

bool Zilog_SerialBridge::enable_uart(bool tx, bool rx, int uart) {
  int nak;
  char cmd[2];
  cmd[0] = reg_for_uart(REG_ENABLE_UART, uart);
  cmd[1] = 0;
  if (tx) cmd[1] = cmd[1] | (1 << 1);
  if (rx) cmd[1] = cmd[1] | 1;
  _i2c->lock(); // Acquire I2C bus
  nak = _i2c->write(_addr, cmd, 2);
  _i2c->unlock(); // Release I2C bus
  return !nak;
}

bool Zilog_SerialBridge::set_baud(int baud, int uart) {
  int nak;
  char cmd[3];
  cmd[0] = reg_for_uart(REG_SET_UART_BAUD, uart);
  uint16_t baud_mult = baud/100;
  cmd[1] = baud_mult >> 8;
  cmd[2] = baud_mult & 0x00FF;
  _i2c->lock(); // Acquire I2C bus
  nak = _i2c->write(_addr, cmd, 3);
  _i2c->unlock(); // Release I2C bus
  return !nak;
}

int Zilog_SerialBridge::get_baud(int uart) {
  int nak;
  char buff[2];
  int baud = 0;
  buff[0] = reg_for_uart(REG_READ_UART_BAUD, uart);
  _i2c->lock(); // Acquire I2C bus
  nak = _i2c->write(_addr, buff, 1, true);
  if (!nak) nak = _i2c->read(_addr, buff, 2);
  _i2c->unlock(); // Release I2C bus
  if (!nak) {
    baud = (buff[0] << 8) | buff[1];
    baud = baud*100;
  }
  return baud;
}

fifo_level Zilog_SerialBridge::get_buffer_level(int uart) {
  fifo_level lvl;
  int nak;
  char buff[2];
  buff[0] = reg_for_uart(REG_READ_FIFO_LVL, uart);
  _i2c->lock();
  nak = _i2c->write(_addr, buff, 1, true);
  if (!nak) nak = _i2c->read(_addr, buff, 2);
  _i2c->unlock();
  if (!nak) {
    lvl.rx_bytes = buff[0];
    lvl.tx_bytes = buff[1];
  }
  return lvl;
}

bool Zilog_SerialBridge::send(char* data, int len, int uart) {
  int ack;
  int i = 0;
  uint8_t reg = reg_for_uart(REG_WRITE_UART_TX_BUFF, uart);
  _i2c->lock(); // Acquire I2C bus
  _i2c->start();
  ack = _i2c->write(_addr);
  if (ack == 1) ack = _i2c->write(reg);
  if (ack == 1) {
    while (i < len) {
      ack = _i2c->write(data[i]);
      if (ack == 1) { // data byte was received so advance
        i++;
      } else { // buffer was full so give it some time to make room
        wait_us(100);
      }
    }
  }
  _i2c->stop();
  _i2c->unlock();
  return (ack == 1);
}

int Zilog_SerialBridge::recv(char* data, int max_len, int uart) {
  int ack;
  int i = 0;
  char buff;
  uint8_t reg = reg_for_uart(REG_READ_UART_RX_BUFF, uart);
  _i2c->lock();
  _i2c->start();
  ack = _i2c->write(_addr);
  if (ack == 1) ack = _i2c->write(reg);
  if (ack == 1) {
    _i2c->start();
    ack = _i2c->write(_addr | 1); // Setting bit to signal read
    while (i < max_len) {
      buff = _i2c->read(1);
      data[i] = buff;
      if (buff == 0x00) break;
      i++;
    }
  }
  return i;
}



uint8_t Zilog_SerialBridge::read_uart_status(int uart) {
  int nak;
  char buff;
  buff = reg_for_uart(REG_UART_STATUS, uart);
  _i2c->lock();
  nak = _i2c->write(_addr, &buff, 1, true);
  if (!nak) _i2c->read(_addr, &buff, 1);
  _i2c->unlock();
  if (!nak) {
    return buff;
  } else {
    return (1<<4);
  }
}

void Zilog_SerialBridge::print_uart_status(int uart) {
  uint8_t status = read_uart_status(uart);
  if (status == UART_STATUS_INVALID) {
    printf("Error getting status\n");
  } else {
    printf("UART %d Status Report\n", uart);
    if (status == 0) printf("\tno flags set\n");
    if (status & UART_STATUS_TX_FULL) printf("\ttx buffer is full\n");
    if (status & UART_STATUS_TX_EMPTY) printf("\ttx buffer is empty\n");
    if (status & UART_STATUS_RX_DATA) printf("\tdata available for rx\n");
    if (status & UART_STATUS_BREAK) printf("\tbreak received\n");
    if (status & UART_STATUS_DATA_OVERRUN) printf("\tERROR: data over run\n");
    if (status & UART_STATUS_FRAME_ERROR) printf("\tERROR: frame error\n");
    if (status & UART_STATUS_PARITY_ERROR) printf("\tERROR: parity error\n");
  }
  printf("\n");
}

uint8_t Zilog_SerialBridge::reg_for_uart(uint8_t base_reg, int uart) {
  switch (uart) {
    case 0:
      return base_reg;
      break;
    case 1:
      return base_reg + 0x20;
      break;
    default:
      printf("Invalid UART: %d\n", uart);
      return 0;
  }
}

