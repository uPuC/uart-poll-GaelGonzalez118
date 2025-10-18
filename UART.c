#include <avr/io.h>
#include <stdio.h>
#include "UART.h"

typedef struct {
	volatile uint8_t ucsra;
	volatile uint8_t ucsrb;
	volatile uint8_t ucsrc;
	volatile uint8_t reserved;
	volatile uint8_t ubrrl;
	volatile uint8_t ubrrh;
	volatile uint8_t udr;
} UART_reg_t;

#define F_CPU 16000000UL

volatile UART_reg_t * const uartAdrr[] = {
	(volatile UART_reg_t *)&UCSR0A,   // Direccion base UART0-0xC0
	(volatile UART_reg_t *)&UCSR1A,   // Direccion base UART1-0xC8
	(volatile UART_reg_t *)&UCSR2A,   // Direccion base UART2-0xD0
	(volatile UART_reg_t *)&UCSR3A   // Direccion base UART3-0x130
};

// =============================================
// Inicializacion de UART
// =============================================
void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop)
{
	if (com > 3) return;  // UART invalido

	volatile UART_reg_t *uart = uartAdrr[com];
	uint16_t ubrr = (F_CPU / (16 * baudrate)) - 1;

	// Baud rate
	*((volatile uint8_t*)&uart->ubrrl) = (uint8_t)ubrr;
	*((volatile uint8_t*)&uart->ubrrh) = (uint8_t)(ubrr >> 8);

	// Habilita Tx y Rx
	uart->ucsrb = (1 << TXEN0) | (1 << RXEN0);

	// UCSRC
	uint8_t ucsrc_val = 0;

	// Bits de datos (5 a 8 bits)
	switch (size) {
		case 6: 
			ucsrc_val |= (1 << UCSZ00); 
			break;
		case 7: 
			ucsrc_val |= (1 << UCSZ01); 
			break;
		case 8: 
			ucsrc_val |= (1 << UCSZ01) | (1 << UCSZ00); 
			break;
		default: 
			break; // 5 bits (00)
	}

	// Paridad
	if (parity == 1) 
		ucsrc_val |= (1 << UPM00) | (1 << UPM01); // Impar
	else if (parity == 2) 
		ucsrc_val |= (1 << UPM01);           // Par

	// Bits de parada
	if (stop == 2) 
		ucsrc_val |= (1 << USBS0);

	uart->ucsrc = ucsrc_val;
}

// =============================================
void UART_putchar(uint8_t com, char data)
{
	switch (com)
	{
		case 0:
			while (!(UCSR0A & (1 << UDRE0)));
			UDR0 = data;
			break;
		case 1:
			while (!(UCSR1A & (1 << UDRE1)));
			UDR1 = data;
			break;
		case 2:
			while (!(UCSR2A & (1 << UDRE2)));
			UDR2 = data;
			break;
		case 3:
			while (!(UCSR3A & (1 << UDRE3)));
			UDR3 = data;
			break;
		default:
			break;
	}
}

// =============================================
uint8_t UART_available(uint8_t com)
{
	switch (com)
	{
		case 0:
			return (UCSR0A & (1 << RXC0)) ? 1 : 0;
		case 1:
			return (UCSR1A & (1 << RXC1)) ? 1 : 0;
		case 2:
			return (UCSR2A & (1 << RXC2)) ? 1 : 0;
		case 3:
			return (UCSR3A & (1 << RXC3)) ? 1 : 0;
		default:
			return 0;
	}
}

// =============================================
char UART_getchar(uint8_t com)
{
	switch (com)
	{
		case 0:
			while (!UART_available(0));
			return UDR0;
		case 1:
			while (!UART_available(1));
			return UDR1;
		case 2:
			while (!UART_available(2));
			return UDR2;
		case 3:
			while (!UART_available(3));
			return UDR3;
		default:
			return 0;
	}
}

// =============================================
// Recibe una cadena
// =============================================
void UART_gets(uint8_t com, char *str)
{
	char c;
	uint8_t i = 0;

	while (1)
	{
		c = UART_getchar(com); // Espera caracter

		// Enter
		if (c == '\r' || c == '\n')
		{
			UART_puts(com, "\r\n");  // Salto de linea en la terminal
			break;
		}
		
		// Backspace
		else if (c == 8 || c == 127) // ASCII 8 = BS, 127 = DEL
		{
			if (i > 0 )
			{
				i--;	// Borra numero
				UART_puts(com, "\b \b");
			}
		}
		
		else if ((i < 19) && ((c != 8 && c != 127)))
		{
			str[i++] = c;
			UART_putchar(com, c);     // Eco
		}
	}

	str[i] = '\0';  // Termina cadena
}
// =============================================
void UART_puts(uint8_t com, char *str)
{
	while (*str)
	{
		UART_putchar(com, *str++);
	}
}

// =============================================
// Entero a texto (itoa)
// =============================================
void itoa(uint16_t number, char* str, uint8_t base)
{
	// 2 (binario), 10 (decimal), 16 (hexadecimal)
	char *ptr = str;
	char *ptr1 = str;
	char tmp_char;
	uint16_t tmp_value;

	do {
		tmp_value = number;
		number /= base;
		*ptr++ = "0123456789ABCDEF"[tmp_value % base];
	} while (number);

	*ptr-- = '\0';

	// Invertir cadena
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
}

// =============================================
// Texto a entero (atoi)
// =============================================
uint16_t atoi(char *str)
{
	uint16_t res = 0;
	while (*str >= '0' && *str <= '9') {
		res = res * 10 + (*str - '0');
		str++;
	}
	return res;
}

// =============================================
// Funciones de secuencia de escape ANSI
// =============================================

void UART_clrscr(uint8_t com)
{
	UART_puts(com, "\033[2J");   // Limpia pantalla
	UART_puts(com, "\033[H");    // Cursor a home
}

void UART_setColor(uint8_t com, uint8_t color)
{
	char cmd[10];
	sprintf(cmd, "\033[%dm", 30 + color);
	UART_puts(com, cmd);
}

void UART_gotoxy(uint8_t com, uint8_t x, uint8_t y)
{
	char cmd[10];
	sprintf(cmd, "\033[%d;%dH", y, x); // Mueve a (x,y)
	UART_puts(com, cmd);
}
