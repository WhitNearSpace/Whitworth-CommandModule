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
    if (status & UART_STATUS_RX_DATA) printf("\tdata available for rx\n"); // is data sheet wrong?
    if (status & UART_STATUS_BREAK) printf("\tbreak received\n");
    if (status & UART_STATUS_DATA_OVERRUN) printf("\tERROR: data over run\n");
    if (status & UART_STATUS_FRAME_ERROR) printf("\tERROR: frame error\n");
    if (status & UART_STATUS_PARITY_ERROR) printf("\tERROR: parity error\n");
  }
  printf("\n");
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

bool Zilog_SerialBridge::set_tx_watermark(char num_bytes, int uart) {
  int nak;
  char cmd[2];
  if ((num_bytes>=1) && (num_bytes<=64)) {
    cmd[0] = reg_for_uart(REG_WRITE_TX_WATERMARK, uart);
    cmd[1] = num_bytes;
    _i2c->lock();
    nak = _i2c->write(_addr, cmd, 2);
    _i2c->unlock();
    return !nak;
  } else {
    return false;
  }
}

bool Zilog_SerialBridge::set_rx_watermark(char num_bytes, int uart) {
  int nak;
  char cmd[2];
  if ((num_bytes>=1) && (num_bytes<=64)) {
    cmd[0] = reg_for_uart(REG_WRITE_RX_WATERMARK, uart);
    cmd[1] = num_bytes;
    _i2c->lock();
    nak = _i2c->write(_addr, cmd, 2);
    _i2c->unlock();
    return !nak;
  } else {
    return false;
  }
}

char Zilog_SerialBridge::get_tx_watermark(int uart) {
  int nak;
  char buff[1];
  buff[0] = reg_for_uart(REG_READ_TX_WATERMARK, uart);
  _i2c->lock();
  nak = _i2c->write(_addr, buff, 1, true);
  if (!nak) nak = _i2c->read(_addr, buff, 1);
  _i2c->unlock();
  if (!nak) {
    return buff[0];
  } else {
    return 0;
  }
}

char Zilog_SerialBridge::get_rx_watermark(int uart) {
  int nak;
  char buff[1];
  buff[0] = reg_for_uart(REG_READ_RX_WATERMARK, uart);
  _i2c->lock();
  nak = _i2c->write(_addr, buff, 1, true);
  if (!nak) nak = _i2c->read(_addr, buff, 1);
  _i2c->unlock();
  if (!nak) {
    return buff[0];
  } else {
    return 0;
  }
}

bool Zilog_SerialBridge::enable_interrupts(uint8_t interrupt_byte, int uart) {
  int nak;
  char cmd[2];
  cmd[0] = reg_for_uart(REG_UART_INT_ENABLE, uart);
  cmd[1] = interrupt_byte;
  _i2c->lock();
  nak = _i2c->write(_addr, cmd, 2);
  _i2c->unlock();
  return !nak;
}

uint8_t Zilog_SerialBridge::get_interrupt_status(int uart) {
  int nak;
  char buff[2];
  uint8_t status_byte;
  buff[0] = reg_for_uart(REG_UART_INT_STATUS, uart);
  _i2c->lock();
  nak = _i2c->write(_addr, buff, 1, true);
  if (!nak) nak = _i2c->read(_addr, buff, 1);
  _i2c->unlock();
  if (!nak) {
    status_byte = buff[0];
  } else {
    status_byte = INT_INVALID;
  }
  return status_byte;
}

bool Zilog_SerialBridge::config_data_bits(char num_bits, int uart) {
  int nak;
  char cmd[3];
  if ((num_bits >= 5) && (num_bits <= 9)) {
    cmd[0] = reg_for_uart(REG_WRITE_UART_CONFIG, uart);
    cmd[1] = UART_CONFIG_SUB_DATA_BITS;
    switch (num_bits) {
      case 5: cmd[2] = UART_CONFIG_DATA_BITS_5; break;
      case 6: cmd[2] = UART_CONFIG_DATA_BITS_6; break;
      case 7: cmd[2] = UART_CONFIG_DATA_BITS_7; break;
      case 8: cmd[2] = UART_CONFIG_DATA_BITS_8; break;
      case 9: cmd[2] = UART_CONFIG_DATA_BITS_9; break;
      default: cmd[2] = UART_CONFIG_DATA_BITS_8;
    }
    _i2c->lock();
    nak = _i2c->write(_addr, cmd, 3);
    _i2c->unlock();
    return !nak;
  } else {
    return false;
  }
}

bool Zilog_SerialBridge::config_parity(char parity, int uart) {
  int nak;
  char cmd[3];
  if ((parity >= 0) && (parity <= 3)) {
    cmd[0] = reg_for_uart(REG_WRITE_UART_CONFIG, uart);
    cmd[1] = UART_CONFIG_SUB_PARITY;
    switch (parity) {
      case 0: cmd[2] = UART_CONFIG_PARITY_NONE; break;
      case 1: cmd[2] = UART_CONFIG_PARITY_ODD; break;
      case 2: cmd[2] = UART_CONFIG_PARITY_EVEN; break;
      case 3: cmd[2] = UART_CONFIG_PARITY_ODD; break;
      default: cmd[2] = UART_CONFIG_PARITY_NONE;
    }
    _i2c->lock();
    nak = _i2c->write(_addr, cmd, 3);
    _i2c->unlock();
    return !nak;
  } else {
    return false;
  }
}

bool Zilog_SerialBridge::config_stop_bits(char num_bits, int uart) {
  int nak;
  char cmd[3];
  if ((num_bits == 1) || (num_bits==2)) {
    cmd[0] = reg_for_uart(REG_WRITE_UART_CONFIG, uart);
    cmd[1] = UART_CONFIG_SUB_STOP_BITS;
    cmd[2] = num_bits - 1;
    _i2c->lock();
    nak = _i2c->write(_addr, cmd, 3);
    _i2c->unlock();
    return !nak;
  } else {
    return false;
  }
}

bool Zilog_SerialBridge::config_flow_control(char flow_ctrl_code, int uart) {
  int nak;
  char cmd[3];
  if ((flow_ctrl_code >= 0) && (flow_ctrl_code <= 2)) {
    cmd[0] = reg_for_uart(REG_WRITE_UART_CONFIG, uart);
    cmd[1] = UART_CONFIG_SUB_FLOW_CTRL;
    cmd[2] = flow_ctrl_code;
    _i2c->lock();
    nak = _i2c->write(_addr, cmd, 3);
    _i2c->unlock();
    return !nak;
  } else {
    return false;
  }
}

bool Zilog_SerialBridge::reset_buffers(bool tx_reset, bool rx_reset, int uart) {
  int nak;
  char cmd[3];
  cmd[0] = reg_for_uart(REG_WRITE_UART_CONFIG, uart);
  cmd[1] = UART_CONFIG_SUB_RESET_FIFOS;
  cmd[2] = 0;
  if (tx_reset) cmd[2] = cmd[2] + 1;
  if (rx_reset) cmd[2] = cmd[2] + 2;
  _i2c->lock();
  nak = _i2c->write(_addr, cmd, 3);
  _i2c->unlock();
  return !nak;
}

bool Zilog_SerialBridge::reset_uart(int uart) {
  int nak;
  char cmd[2];
  cmd[0] = reg_for_uart(REG_WRITE_UART_CONFIG, uart);
  cmd[1] = UART_CONFIG_SUB_RESET;
  _i2c->lock();
  nak = _i2c->write(_addr, cmd, 2);
  _i2c->unlock();
  return !nak;
}

/***** PRIVATE METHODS *****/

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

