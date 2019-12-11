/**
 * \file
 *
 * \brief CAN FD Example.
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include <atmel_start.h>
#include <hal_gpio.h> /// LED0


static void CAN_0_std_tx( void );


//-----------------------------------------------------------------------------
// Timer examples:
//  mcu-starter-projects\same54\main.c
//-----------------------------------------------------------------------------
#define PERIOD_FAST    100
#define PERIOD_SLOW    500

#define F_CPU  120000000

//#define                     MCU_TIMER_EXAMPLE            1

//HAL_GPIO_PIN(LED,      C, 18)
//HAL_GPIO_PIN(BUTTON,   B, 31)
//HAL_GPIO_PIN(UART_TX,  B, 25)
//HAL_GPIO_PIN(UART_RX,  B, 24)

//-----------------------------------------------------------------------------
static void timer_set_period(uint16_t i)
{
	TC0->COUNT16.CC[0].reg = (F_CPU / 1000ul / 1024) * i;
	TC0->COUNT16.COUNT.reg = 0;
}

//-----------------------------------------------------------------------------
void __TC0_Handler( void ) // void irq_handler_tc0(void)
{
	if (TC0->COUNT16.INTFLAG.reg & TC_INTFLAG_MC(1))
	{
		gpio_toggle_pin_level(LED0); // HAL_GPIO_LED_toggle();
		TC0->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);

        CAN_0_std_tx( );
	}
}

//-----------------------------------------------------------------------------
static void __timer_init(void)
{
	MCLK->APBAMASK.reg |= MCLK_APBAMASK_TC0;

	GCLK->PCHCTRL[TC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN(0) | GCLK_PCHCTRL_CHEN;
	while (0 == (GCLK->PCHCTRL[TC0_GCLK_ID].reg & GCLK_PCHCTRL_CHEN));

	TC0->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1024 |
	TC_CTRLA_PRESCSYNC_RESYNC;

	TC0->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;

	TC0->COUNT16.COUNT.reg = 0;

	timer_set_period(PERIOD_SLOW);

	TC0->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;

	TC0->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);

	NVIC_EnableIRQ(TC0_IRQn);
}

//-----------------------------------------------------------------------------

//TIMER_0_example (copied from driver_example.c)

static struct timer_task TIMER_0_task1, TIMER_0_task2;

/**
 * Example of using TIMER_0.
 */
static void TIMER_0_task1_cb(const struct timer_task *const timer_task)
{
//				  gpio_toggle_pin_level(LED0);
    CAN_0_std_tx( ); 
}

static void TIMER_0_task2_cb(const struct timer_task *const timer_task)
{
					  gpio_toggle_pin_level(LED0);
}

void __TIMER_0_example(void)
{
	TIMER_0_task1.interval = 100;
	TIMER_0_task1.cb       = TIMER_0_task1_cb;
	TIMER_0_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_0_task2.interval = 200;
	TIMER_0_task2.cb       = TIMER_0_task2_cb;
	TIMER_0_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_0, &TIMER_0_task1);
	timer_add_task(&TIMER_0, &TIMER_0_task2);
	timer_start(&TIMER_0);
}

//-----------------------------------------------------------------------------


/**
 * \brief display configuration menu.
 */
static void display_menu(void)
{
	printf("Menu :\r\n"
	       "  -- Select the action:\r\n"
	       "  0: Set standard filter ID 0: 0x45A, store into Rx buffer. \r\n"
	       "  1: Set standard filter ID 1: 0x469, store into Rx FIFO 0. \r\n"
	       "  2: Send FD standard message with ID: 0x45A and 64 byte data 0 to 63. \r\n"
	       "  3: Send FD standard message with ID: 0x469 and 64 byte data 128 to 191. \r\n"
	       "  4: Set extended filter ID 0: 0x100000A5, store into Rx buffer. \r\n"
	       "  5: Set extended filter ID 1: 0x10000096, store into Rx FIFO 1. \r\n"
	       "  6: Send FD extended message with ID: 0x100000A5 and 64 byte data 0 to 63. \r\n"
	       "  7: Send FD extended message with ID: 0x10000096 and 64 byte data 128 to 191. \r\n"
	       "  a: Send normal standard message with ID: 0x469 and 8 byte data 0 to 7. \r\n"
	       "  h: Display menu \r\n\r\n");
}

static void CAN_0_tx_callback(struct can_async_descriptor *const descr)
{
	(void)descr;
	printf("  CAN Transmission done \r\n");
}

static void CAN_std_tx_callback(struct can_async_descriptor *const descr)
{
	(void)descr;

	/* Enable the FDOE and BRSE in register configuration*/
	hri_can_set_CCCR_INIT_bit(CAN_0.dev.hw);
	while (hri_can_get_CCCR_INIT_bit(CAN_0.dev.hw) == 0)
		;
	hri_can_set_CCCR_CCE_bit(CAN_0.dev.hw);

	hri_can_set_CCCR_FDOE_bit(CAN_0.dev.hw);
	hri_can_set_CCCR_BRSE_bit(CAN_0.dev.hw);

	printf("  CAN Transmission done. \r\n");
}

static void CAN_0_rx_callback(struct can_async_descriptor *const descr)
{
	struct can_message msg;
	uint8_t            data[64];
	msg.data = data;
	can_async_read(descr, &msg);

	printf("\n\r CAN Message received . The received data is: \r\n");
	for (uint8_t i = 0; i < msg.len; i++) {
		printf("  %d", msg.data[i]);
	}
	printf("\r\n\r\n");
	return;
}

static void CAN_0_std_tx( void ){
		static int ctr = 0xa5;
			struct can_message msg;
				uint8_t            data[8];
				msg.data = data;
		
			msg.id   = 0xFECA;
			msg.type = CAN_TYPE_DATA;
//			msg.data = tx_message_2;
			msg.len  = 8;
			msg.fmt  = CAN_FMT_EXTID;

			msg.data[7] = ctr++; // tmp test

			/* Disable the FDOE and BRSE from register configuration
			 * and enable them again in callback  ^H^H^H^H^H^H^H^H^H^H^H^    no leave FD unconfigured */
			hri_can_set_CCCR_INIT_bit(CAN_0.dev.hw);
			while (hri_can_get_CCCR_INIT_bit(CAN_0.dev.hw) == 0)
				;
			hri_can_set_CCCR_CCE_bit(CAN_0.dev.hw);

			hri_can_clear_CCCR_FDOE_bit(CAN_0.dev.hw);
			hri_can_clear_CCCR_BRSE_bit(CAN_0.dev.hw);

//			can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_std_tx_callback);
 // GN: use normal callback (use CAN STD only so no need re-enable FD/BR
            can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_0_tx_callback);

			can_async_enable(&CAN_0);
			can_async_write(&CAN_0, &msg);
}


/////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	uint8_t key;

	struct can_message msg;
	struct can_filter  filter;

	uint8_t tx_message_0[64];
	uint8_t tx_message_1[64];
	uint8_t tx_message_2[8];

	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	display_menu();

	/* Initialize the data to be used later */
	for (uint8_t i = 0; i < 64; i++) {
		tx_message_0[i] = i;
	}

	for (uint8_t j = 128; j < 192; j++) {
		tx_message_1[j - 128] = j;
	}

	for (uint8_t k = 0; k < 8; k++) {
		tx_message_2[k] = k;
	}


	can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback);
#if 0 // optional filter+mask
	filter.id   = 0x469;
	filter.mask = 0x07ff;
#endif
	can_async_set_filter(&CAN_0, 0, CAN_FMT_EXTID, &filter);


int fast = 1;
#ifdef MCU_TIMER_EXAMPLE 
 __timer_init();
 timer_set_period(fast ? PERIOD_FAST : PERIOD_SLOW);
#endif


__TIMER_0_example();



//////////////////////////////////////////////////////////////////////////	
#if 1
		  while (true) {

 #if 0 // #ifndef MCU_TIMER_EXAMPLE
			  delay_ms(500);
			  gpio_toggle_pin_level(LED0);

			  msg.data = tx_message_2;
              CAN_0_std_tx( );
 #endif
		  }


#else
	while (1) {
		scanf("%c", (char *)&key);

		switch (key) {
		case 'h':
			display_menu();
			break;

		case '0':
			printf("  0: Set standard filter ID 0: 0x45A, store into Rx buffer. \r\n");
			can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback);
			filter.id   = 0x45A;
			filter.mask = 0;
			can_async_set_filter(&CAN_0, 0, CAN_FMT_STDID, &filter);
			break;

		case '1':
			printf("  1: Set standard filter ID 1: 0x469, store into Rx FIFO 0. \r\n");
			can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback);
			filter.id   = 0x469;
			filter.mask = 0;
			can_async_set_filter(&CAN_0, 1, CAN_FMT_STDID, &filter);
			break;

		case '2':
			printf("  2: Send standard message with ID: 0x45A and 64 byte data 0 to 63. \r\n");
			msg.id   = 0x45A;
			msg.type = CAN_TYPE_DATA;
			msg.data = tx_message_0;
			msg.len  = 64;
			msg.fmt  = CAN_FMT_STDID;

			can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_0_tx_callback);
			can_async_enable(&CAN_0);
			can_async_write(&CAN_0, &msg);
			break;

		case '3':
			printf("  3: Send standard message with ID: 0x469 and 64 byte data 128 to 191. \r\n");
			msg.id   = 0x469;
			msg.type = CAN_TYPE_DATA;
			msg.data = tx_message_1;
			msg.len  = 64;
			msg.fmt  = CAN_FMT_STDID;

			can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_0_tx_callback);
			can_async_enable(&CAN_0);
			can_async_write(&CAN_0, &msg);
			break;

		case '4':
			printf("  4: Set extended filter ID 0: 0x100000A5, store into Rx buffer. \r\n");
			can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback);
			filter.id   = 0x100000A5;
			filter.mask = 0;
			can_async_set_filter(&CAN_0, 0, CAN_FMT_EXTID, &filter);
			break;

		case '5':
			printf("  5: Set extended filter ID 1: 0x10000096, store into Rx FIFO 1. \r\n");
			can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)CAN_0_rx_callback);
			filter.id   = 0x10000096;
			filter.mask = 0;
			can_async_set_filter(&CAN_0, 1, CAN_FMT_EXTID, &filter);
			break;

		case '6':
			printf("  6: Send extended message with ID: 0x100000A5 and 64 byte data 0 to 63. \r\n");
			msg.id   = 0x100000A5;
			msg.type = CAN_TYPE_DATA;
			msg.data = tx_message_0;
			msg.len  = 64;
			msg.fmt  = CAN_FMT_EXTID;

			can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_0_tx_callback);
			can_async_enable(&CAN_0);
			can_async_write(&CAN_0, &msg);
			break;

		case '7':
			printf("  7: Send extended message with ID: 0x10000096 and 64 byte data 128 to 191. \r\n");
			msg.id   = 0x10000096;
			msg.type = CAN_TYPE_DATA;
			msg.data = tx_message_1;
			msg.len  = 64;
			msg.fmt  = CAN_FMT_EXTID;

			can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_0_tx_callback);
			can_async_enable(&CAN_0);
			can_async_write(&CAN_0, &msg);
			break;

		case 'a':
			printf("  a: Send normal standard message with ID: 0x469 and 8 byte data 0 to 7. \r\n");
			msg.id   = 0x469;
			msg.type = CAN_TYPE_DATA;
			msg.data = tx_message_2;
			msg.len  = 8;
			msg.fmt  = CAN_FMT_STDID;

			/* Disable the FDOE and BRSE from register configuration
			 * and enable them again in callback */
			hri_can_set_CCCR_INIT_bit(CAN_0.dev.hw);
			while (hri_can_get_CCCR_INIT_bit(CAN_0.dev.hw) == 0)
				;
			hri_can_set_CCCR_CCE_bit(CAN_0.dev.hw);

			hri_can_clear_CCCR_FDOE_bit(CAN_0.dev.hw);
			hri_can_clear_CCCR_BRSE_bit(CAN_0.dev.hw);

			can_async_register_callback(&CAN_0, CAN_ASYNC_TX_CB, (FUNC_PTR)CAN_std_tx_callback);
			can_async_enable(&CAN_0);
			can_async_write(&CAN_0, &msg);
			break;

		default:
			break;
		}
	}
#endif
}
