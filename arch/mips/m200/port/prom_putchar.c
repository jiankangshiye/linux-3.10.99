#include <stdarg.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include "../inc/hw_memmap.h" /* UART3_BASE */
#include "../driverlib/uart.h" /* UARTCharPut */

extern void putstring(const char *str);
void prom_putchar(char c)
{
	/*
	while(!((*(volatile char *)0xb0033014 & ((1 << 5) | (1 << 6))) == 0x60 )) {}
	*(volatile char *)0xb003300C &= ~(1 << 7);
	*(volatile char *)0xb0033000 = c;
	*/
	UARTCharPut(UART3_BASE, c);
}

static char decimal_array[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static char hexadecimal_array[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                   'A', 'B', 'C', 'D', 'E', 'F'};
/*
 * buf:
 * x: a integer
 * count: a counter, number bytes to store x
 * cxt: a context, here use to store a decimal character
 */
void decimal2string_helper(char *buf, int x, int count, int *result)
{
	if (x == 0) {
		*result = count;
	} else {
		decimal2string_helper(buf, x / 10, count + 1, result);
		buf[*result - (count + 1)] = decimal_array[ x % 10 ];
	}
}

int decimal2string(char *buf, int x)
{
	int count = 0;
	decimal2string_helper(buf, x, 0, &count);
	return count;
}

int hexadecimal2string(char *buf, int x)
{
	int count = 0;
	int shift = 32;
	int half_byte;
	while (x != 0 && shift) {
		shift -= 4;
		half_byte = x >> shift & 0xf;
		buf[count++] = hexadecimal_array[half_byte];
	}
	return count;
}

int prom_vsprintf(char *string, const char *format, va_list ap)
{
	int d;
	char c;
	const char *p = format;
	char *str = string;
	char *pchar = NULL;
	int n;

	while (*p != '\0') {
		if (*p == '%') {
			p++;
			switch (*p) {
			case 'd':
				d = va_arg(ap, int);
				n = decimal2string(str, d);
				str += n;
				p++;
				continue;
			case 'x':
			case 'X':
				d = va_arg(ap, int);
				n = hexadecimal2string(str, d);
				str += n;
				p++;
				continue;
			case 'c':
				c = (char)va_arg(ap, int);
				*str = c;
				str++;
				p++;
				continue;
			case 's':
				pchar = va_arg(ap, char *);
				n = strlen(pchar);
				memcpy(str, pchar, n);
				str += n;
				p++;
				continue;
			default:
				break;
			}
		}
		*str = *p;
		p++;
		str++;
	}
	return str - string;
}

int prom_vprintf(const char *format, va_list ap)
{
	int retval;
	int i;
	char buf[1024];

	retval = prom_vsprintf(buf, format, ap);
	for (i = 0; i < retval; i++) prom_putchar(buf[i]);

	return retval;
}

int prom_printf(const char *format, ...)
{
	int retval;
	va_list args;

	va_start(args, format);
	retval = prom_vprintf(format, args);
	va_end(args);

	return retval;
}
EXPORT_SYMBOL_GPL(prom_printf);
