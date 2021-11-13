import serial
import crccheck
import argparse
import os
import math
import stm32_crc
from enum import IntEnum

BOOTLOADER_OK = b"OK!"
BOOTLOADER_ERR = b"ERR"


class BootloaderCommand(IntEnum):
    INVALID = 0x00,
    ECHO = 0x01,
    SETSIZE = 0x02,
    UPDATE = 0x03,
    CHECK = 0x04,
    JUMP = 0x05


def create_command(command: BootloaderCommand, data: int = 0) -> bytes:
    return bytes([command]) + data.to_bytes(4, 'big')

def send_command(command: bytes, port: serial.Serial) -> bool:
    print(command)
    port.write(command)
    stm_response = port.read(3)
    print(stm_response)
    return stm_response == BOOTLOADER_OK

def is_bootloader_running(port: serial.Serial) -> bool:
    return send_command(create_command(BootloaderCommand.ECHO), port)

def send_firmware_size(port: serial.Serial, size: int) -> bool:
    return send_command(create_command(BootloaderCommand.SETSIZE, size), port)

def flash_firmware(port: serial.Serial, firmware: bytes, packet_size: int = 1024) -> bool:
    if not send_command(create_command(BootloaderCommand.UPDATE), port):
        print('Cannot enter into update mode!')
        return False

    bytes_sent = 0
    firmware_size = len(firmware)
    packets_amount = math.ceil(firmware_size / packet_size)
    packets_sent = 0
    
    while bytes_sent < firmware_size:
        # calculate next packet length
        bytes_left = firmware_size - bytes_sent
        next_packet_length = packet_size if bytes_left > packet_size else bytes_left
        firmware_slice = firmware[bytes_sent:(bytes_sent + next_packet_length)]

        print(f'Sending {len(firmware_slice)} bytes')
        port.write(firmware_slice)
        stm_response = port.read(3)

        if stm_response != BOOTLOADER_OK:
            print('Bootloader did not respond or returned an error while programming!')
            return False
        else:
            packets_sent += 1
            print(f'Progress: {packets_sent}/{packets_amount}')
        
        bytes_sent += next_packet_length

    final_response = port.read(3)
    return final_response == BOOTLOADER_OK

def verify_firmware(port: serial.Serial, checksum: int) -> bool:
    return send_command(create_command(BootloaderCommand.CHECK, checksum), port)

def main():
    # Stwórz interfejs do obsługi argumentów
    parser = argparse.ArgumentParser(
        description='Update STM32 microcontroller firmware via UART')
    parser.add_argument('update_file', type=str,
                        help='Path to compiled firmware file (*.bin)')
    parser.add_argument('com_port', type=str,
                        help='COM port of the microcontroller')
    args = parser.parse_args()

    # Sprawdź czy podana ścieżka istnieje
    if not os.path.isfile(args.update_file):
        print(f'File {args.update_file} does not exist, exiting...')
        exit(1)

    print(f'Updating from {args.update_file}')

    update_data = bytes()
    # Otwieramy plik w trybie odczytu bajtów i wrzucamy jego zawartość
    # do zmiennej `update_data`
    with open(args.update_file, 'rb') as update_file:
        update_data = update_file.read()

    crc_calculator = stm32_crc.STM32CRC()
    firmware_checksum = crc_calculator.calculate(update_data)
    print(f'Firmware checksum: {hex(firmware_checksum)}')

    try:
        stm_uart = serial.Serial(
            port=args.com_port, baudrate=921600, timeout=20)
    except serial.SerialException as ex:
        print(
            f'Cannot open STM32 port: {ex}. Make sure it\'s not opened by another application!')
        exit(2)

    if not stm_uart.isOpen():
        print('Couldn\'t open STM32 UART port! Exiting...')
        exit(2)

    print('Connected to STM32 microcontroller!')
    if is_bootloader_running(stm_uart):
        print('Bootloader is running, ready to flash')
    else:
        print('Bootloader not detected, exiting...')
        exit(3)

    if send_firmware_size(stm_uart, len(update_data)):
        print(f'Firmware size set to {len(update_data)}')
    else:
        print('Cannot set the firmware size, exiting...')
        exit(4)

    if flash_firmware(stm_uart, update_data):
        print('Update finished, verifying firmware...')
        if verify_firmware(stm_uart, firmware_checksum):
            print('Firmware verified! UPDATE SUCCESSFUL!')
            print('Jumping to application...')
            if send_command(create_command(BootloaderCommand.JUMP), stm_uart):
                print('Application is running!')
            else:
                print('Jumping to application failed!')
        else:
            print('Firmware unverified, invalid checksum.')
    else:
        print('Update failed!')



if __name__ == '__main__':
    main()