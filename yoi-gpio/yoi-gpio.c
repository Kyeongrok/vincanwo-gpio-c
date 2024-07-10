#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <string.h>

#define HostStatusRegister 0xF040
#define HostCommandRegister 0xF043
#define TransmitSlaveAddressRegister 0xF044
#define HostControlRegister 0xF042
#define Data0Register 0xF045
#define Data1Register 0xF046
#define DEVICE_ADDRESS_WRITE 0x40
#define DEVICE_ADDRESS_READ 0x41

#define PCA9555_COMMAND_INPUT_PORT0 0
#define PCA9555_COMMAND_INPUT_PORT1 1
#define PCA9555_COMMAND_OUTPUT_PORT0 2
#define PCA9555_COMMAND_OUTPUT_PORT1 3
#define PCA9555_COMMAND_CONFIGURATION_PORT0 6
#define PCA9555_COMMAND_CONFIGURATION_PORT1 7

uint8_t Inp8(unsigned short int GpioAddress)
{
    return inb(GpioAddress);
}

void Out8(unsigned short int GpioAddress, uint8_t Data)
{
    outb(Data, GpioAddress);
}

void watie_smbus_completion()
{
    uint8_t dwPortVal = 0;
    int i = 0;
    dwPortVal = Inp8(HostStatusRegister);
    while (dwPortVal != 0x42)
    {
        dwPortVal = Inp8(HostStatusRegister);
        usleep(200000);
        i = i + 1;
        if (i > 200)
        {
            dwPortVal = 0x42;
            i = 0;
        }
    }
}

void pca9555_write_byte(uint8_t cmd, uint8_t value)
{
    Out8(HostStatusRegister, 0xFF);
    Out8(TransmitSlaveAddressRegister, DEVICE_ADDRESS_WRITE);
    Out8(HostCommandRegister, cmd);
    Out8(Data0Register, value);
    Out8(HostControlRegister, 0x48); // START, Byte Data

    watie_smbus_completion();
}

void pca9555_write_word(uint8_t cmd, uint16_t value)
{
    Out8(HostStatusRegister, 0xFF);
    Out8(TransmitSlaveAddressRegister, DEVICE_ADDRESS_WRITE);
    Out8(HostCommandRegister, cmd);
    Out8(Data0Register, value & 0xFF);
    Out8(Data1Register, (value >> 8) & 0xFF);
    Out8(HostControlRegister, 0x4C); // START, Byte Data

    watie_smbus_completion();
}

uint8_t pca9555_read_byte(uint8_t cmd)
{
    Out8(HostStatusRegister, (0xFF));
    Out8(TransmitSlaveAddressRegister, DEVICE_ADDRESS_READ);
    Out8(HostCommandRegister, cmd);
    Out8(HostControlRegister, (0x48));
    watie_smbus_completion();

    return Inp8(Data0Register);
}

uint16_t pca9555_read_word(uint8_t cmd)
{
    Out8(HostStatusRegister, (0xFF));
    Out8(TransmitSlaveAddressRegister, DEVICE_ADDRESS_READ);
    Out8(HostCommandRegister, cmd);
    Out8(HostControlRegister, (0x4C));
    watie_smbus_completion();

    return (Inp8(Data1Register) << 8) | Inp8(Data0Register);
}

uint16_t gpio_read()
{
    uint8_t dwPortValLow = 0;
    uint8_t dwPortVal = 0;

    //SET GPIO1 ~GPIO8 is Input
    pca9555_write_byte(PCA9555_COMMAND_CONFIGURATION_PORT0, 0xFF);

    //SET GPIO9 ~GPIO16 is Input
    pca9555_write_byte(PCA9555_COMMAND_CONFIGURATION_PORT1, 0xFF);

    //GET GPIO1 ~GPIO8
    dwPortValLow = pca9555_read_byte(PCA9555_COMMAND_INPUT_PORT0); //Get GPIO1 ~GPIO8 is High or Low

    //GET GPIO9 ~GPIO16
    dwPortVal = pca9555_read_byte(PCA9555_COMMAND_INPUT_PORT1); //Get GPIO9 ~GPIO16 is High or Low
    // return pca9555_read_word(PCA9555_COMMAND_INPUT_PORT0);
    return (dwPortVal << 8) | dwPortValLow;
}

void gpio_write_all(int Level)
{
    uint32_t dwPortVal = 0;
    uint32_t i = 0;
    uint8_t GPIO2 = 0;
    uint8_t GPIO4 = 0;

    //SET GPIO1 ~GPIO8 is Output
    pca9555_write_word(PCA9555_COMMAND_CONFIGURATION_PORT0, 0x00); //GPIO10 ~GPIO17  1 = Output

    if (Level > 0)
    {
        pca9555_write_word(PCA9555_COMMAND_OUTPUT_PORT0, 0xFFFF); //GPIO10 ~GPIO17  1 = Output
        printf("gpio out High!\n");
    }
    else
    {
        pca9555_write_word(PCA9555_COMMAND_OUTPUT_PORT0, 0); //GPIO10 ~GPIO17  1 = Output
        printf("gpio out Low!\n");
    }
}

void gpio_write(uint8_t high, int GPIO)
{
    uint32_t dwPortVal = 0;

    if (high > 0)
    {
        //SET GPIO1 ~GPIO8 is Output
        pca9555_write_byte(PCA9555_COMMAND_CONFIGURATION_PORT1, 0x00); // 1 = Output
        pca9555_write_byte(PCA9555_COMMAND_OUTPUT_PORT1, GPIO);
    }
    else
    {
        //SET GPIO1 ~GPIO8 is Output
        pca9555_write_byte(PCA9555_COMMAND_CONFIGURATION_PORT0, 0x00); // 1 = Output
        pca9555_write_byte(PCA9555_COMMAND_OUTPUT_PORT0, GPIO);
    }
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "write") == 0)
    {
        // gpio_num 1: high byte 0: low byte
        int gpio_num = atoi(argv[2]);
        int gpio_state = atoi(argv[3]);
        int ret = iopl(3);
        printf("ioperm ret = %d error = %s\n", ret, strerror(errno));
        gpio_write(gpio_num, gpio_state);
        iopl(0);
    }
    else if (argc == 2 && strcmp(argv[1], "read") == 0)
    {
        // int gpio_num = atoi(argv[2]);
        int ret = iopl(3);
        uint16_t value = gpio_read();
        printf("read gpio: %02X\n", value);
        iopl(0);
    }
    else
    {
        printf("argv error!\n");
    }
}
