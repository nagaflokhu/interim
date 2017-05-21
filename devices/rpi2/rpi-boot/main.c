/* Copyright (C) 2013-2016 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfdt.h>
#include "uart.h"
#include "atag.h"
#include "fb.h"
#include "console.h"
#include "block.h"
#include "vfs.h"
#include "memchunk.h"
#include "usb.h"
#include "dwc_usb.h"
#include "output.h"
#include "log.h"
#include "rpifdt.h"

#define UNUSED(x) (void)(x)

uintptr_t _atags;
unsigned long _arm_m_type;
#ifdef __aarch64__
uintptr_t base_adjust = BASE_ADJUST_V2;
#else
uintptr_t base_adjust = 0;
#endif

char rpi_boot_name[] = "rpi_boot";

const char *atag_cmd_line;

static char *boot_cfg_names[] =
{
	"/boot/rpi_boot.cfg",
	"/boot/rpi-boot.cfg",
	"/boot/grub/grub.cfg",
	0
};

static void mem_cb(uint32_t addr, uint32_t len)
{
#ifdef DEBUG
	printf("MEMORY: addr: %x, len: %x\n", addr, len);
#endif

	if(addr < 0x100000)
	{
		addr = 0x100000;
		len -= 0x100000;
	}
	chunk_register_free(addr, len);
}

void libfs_init();

extern int (*stdout_putc)(int);
extern int (*stderr_putc)(int);
extern int (*stream_putc)(int, FILE*);
extern int def_stream_putc(int, FILE*);

int multiboot_cfg_parse(char *buf);

int conf_source = 0;

__attribute__((__weak__)) void find_and_run_config(void)
{
	// Look for a boot configuration file, starting with the default device,
	// then iterating through all devices

	FILE *f = (void*)0;

	// Default device
	char **fname = boot_cfg_names;
	char *found_cfg;
	while(*fname)
	{
		f = fopen(*fname, "r");
		if(f)
		{
			found_cfg = *fname;
			break;
		}

		fname++;
	}

	if(!f)
	{
		// Try other devices
		char **dev = vfs_get_device_list();
		while(*dev)
		{
			int dev_len = strlen(*dev);

			fname = boot_cfg_names;
			while(*fname)
			{
				int fname_len = strlen(*fname);
				char *new_str = (char *)malloc(dev_len + fname_len + 3);
				new_str[0] = 0;
				strcat(new_str, "(");
				strcat(new_str, *dev);
				strcat(new_str, ")");
				strcat(new_str, *fname);

				f = fopen(new_str, "r");

				if(f)
				{
					found_cfg = new_str;
					break;
				}

				free(new_str);
				fname++;
			}

			if(f)
				break;

			dev++;
		}
	}

	if(!f)
	{
		printf("MAIN: No bootloader configuration file found\n");
	}
	else
	{
		long flen = fsize(f);
		printf("MAIN: Found bootloader configuration: %s\n", found_cfg);
		char *buf = (char *)malloc(flen+1);
		buf[flen] = 0;		// null terminate
		fread(buf, 1, flen, f);
		fclose(f);
		multiboot_cfg_parse(buf);
	}
}

void kernel_main(unsigned long boot_dev, unsigned long arm_m_type,
                 unsigned long atags)
{
	// Hack for newer firmware - assume if atags not specified then they are at 0x100
	// We also check the zero address in case this is true - see later
	if(atags == 0x0)
		atags = 0x100;
	
	atag_cmd_line = (void *)0;
	_atags = atags;
	_arm_m_type = arm_m_type;
	UNUSED(boot_dev);

	// First use the serial console
	stdout_putc = split_putc;
	stderr_putc = split_putc;
	stream_putc = def_stream_putc;

	output_init();
	output_enable_uart();

	// dump arguments to main
#ifdef DEBUG
	printf("MAIN: boot_dev: %x, arm_m_type: %i, atags: %x\n", boot_dev,
			arm_m_type, atags);
#endif

	// try and interpret device tree/atags
	if(fdt_check_header((const void *)atags) == 0)
		conf_source = 3;
	else if(fdt_check_header((const void *)0) == 0)
		conf_source = 4;
	else if(check_atags((const void *)atags) == 0)
		conf_source = 1;
	else if(check_atags((const void *)0) == 0)
		conf_source = 2;

	switch(conf_source)
	{
		case 1:
		case 2:
			// If using ATAGs, need to set up base adjust
			// manually
			if(arm_m_type == 0xc42)
				base_adjust = BASE_ADJUST_V1;
			else if(arm_m_type == 0xc43)
				base_adjust = BASE_ADJUST_V2;
			uart_init();
#ifdef DEBUG
			printf("ATAGS: detected\n");
#endif

			// Fall through
		case 3:
		case 4:
			parse_atag_or_dtb(mem_cb);
			break;
		default:
			/* Assume we are at least on RPi2 */
			base_adjust = BASE_ADJUST_V2;
			mem_cb(0, 512 * 1024 * 1024);
			uart_init();
			break;
	}

#ifdef ENABLE_FRAMEBUFFER
	int result = fb_init();
	if(result == 0)
	{
		puts("Successfully set up frame buffer");
#ifdef DEBUG2
		printf("FB: width: %i, height: %i, bpp: %i\n",
			fb_get_width(), fb_get_height(),
			fb_get_bpp());
#endif
	}
	else
	{
		puts("Error setting up framebuffer:");
		puthex(result);
	}
#endif

	// Switch to the framebuffer for output
	output_enable_fb();

	// Allocate a log in memory
#ifdef ENABLE_CONSOLE_LOGFILE
	register_log_file(NULL, 0x1000);
	output_enable_log();
#endif

	printf("Welcome to Rpi bootloader\n");
	printf("Compiled on %s at %s\n", __DATE__, __TIME__);
	printf("ARM system type is %x\n", arm_m_type);
	if(atag_cmd_line != (void *)0)
		printf("Command line: %s\n", atag_cmd_line);

    // Register the various file systems
	libfs_init();

	// List devices
	printf("MAIN: device list: ");
	vfs_list_devices();
	printf("\n");

	find_and_run_config();
}

