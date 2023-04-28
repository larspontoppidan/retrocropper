
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

// ---- Common

typedef uint8_t bool_t;
#define TRUE 1
#define FALSE 0



// ---- HAL

#define HAL_LED1_PIN 3
#define HAL_BTN_PIN 4
#define HAL_LED2_PIN 7

// #define HAL_VID_SW_PIN 2

#define HAL_VID_SW_PIN 6


void ioInit(void)
{
	DDRA = (1 << HAL_LED1_PIN) | (1 << HAL_LED2_PIN) | (1 << HAL_VID_SW_PIN);
    PORTA = (1 << HAL_BTN_PIN) | (1 << HAL_LED1_PIN) | (1 << HAL_LED2_PIN) | (1 << HAL_VID_SW_PIN);

	// Video switch
	// DDRB = (1 << HAL_VID_SW_PIN);
	// PORTB = (1 << HAL_VID_SW_PIN);
}

void led1Set(bool_t value)
{
	if (value)
	{
		PORTA &= ~(1 << HAL_LED1_PIN);
	}
	else
	{
		PORTA |= (1 << HAL_LED1_PIN);
	}
}

void led2Set(bool_t value)
{
	if (value)
	{
		PORTA &= ~(1 << HAL_LED2_PIN);
	}
	else
	{
		PORTA |= (1 << HAL_LED2_PIN);
	}
}

void videoSwitchSet(bool_t value) // value = 1 chooses video IN, value = 0 chooses black
{
	//PORTB = value ? (1 << HAL_VID_SW_PIN) : 0;
}

bool_t switchRead(void)
{
	return (PINA & (1 << HAL_BTN_PIN)) == 0;
}

// ----


static volatile uint8_t Mode;

// ----

static uint8_t FieldCount;
static uint16_t FieldLine;

// ----


typedef struct {
	uint16_t LineFieldStart;
	uint16_t LineFieldEnd;
	uint16_t LineScreenStart;
	uint16_t LineScreenEnd;
	uint16_t CropStart;
	uint16_t BorderCropLength;
	uint16_t ScreenCropLength;
	int16_t  ScreenCropAlternate;

} cropSpec_t;

#define MODES 6

#define DTV_EARLY_START 20

static const cropSpec_t CropSpecs[MODES] = 
{
	// MODE: 0, no cropping at all
	{ 0, 0, 0, 0, 0, 0, 0, 0}, 

	// MODE: 1 C64, only crop left line
	{ 7, 307,    // LineFieldStart, LineFieldEnd
	  0, 400,    // LineScreenStart, LineScreenEnd
	 99,         // CropStart
	 1055,       // BorderCropLength
	 35, 12      // ScreenCropLength, ScreenCropAlternate
	},

	// MODE: 2, full C64 cropping
	{ 7, 307,    // LineFieldStart, LineFieldEnd
	 25, 293,    // LineScreenStart, LineScreenEnd
	 99,         // CropStart
	 1055,       // BorderCropLength
	 35, 12      // ScreenCropLength, ScreenCropAlternate
	},

	// MODE: 3, full C64 cropping without alternating left (a)   <--- best mode for C64!
	{ 7, 307,    // LineFieldStart, LineFieldEnd
	 25, 293,    // LineScreenStart, LineScreenEnd
	 99,         // CropStart
	 1055,       // BorderCropLength
	 35, 0       // ScreenCropLength, ScreenCropAlternate
	},

	// MODE: 4, full C64 cropping without alternating left (b)
	{ 7, 307,    // LineFieldStart, LineFieldEnd
	 25, 293,    // LineScreenStart, LineScreenEnd
	 99,         // CropStart
	 1055,       // BorderCropLength
	 41, 0       // ScreenCropLength, ScreenCropAlternate
	},

	// MODE: 5, full DTV cropping with alternating left
	{ 7, 307,    // LineFieldStart, LineFieldEnd
	 25, 293,    // LineScreenStart, LineScreenEnd
	 99 - DTV_EARLY_START,         // CropStart
	 1055 + DTV_EARLY_START,       // BorderCropLength
	 35 + DTV_EARLY_START, 12      // ScreenCropLength, ScreenCropAlternate
	},

};

// Comparator

static uint16_t CropStart;
static uint16_t CropLength;

static void setupCrop(uint16_t field_line, const cropSpec_t *spec)
{
	if ((field_line > spec->LineFieldEnd) || (field_line < spec->LineFieldStart))
	{
		// Outside field, don't crop
		CropStart = 0;
	}
	else 
	{
		CropStart = spec->CropStart;

		if ((field_line < spec->LineScreenStart) || (field_line > spec->LineScreenEnd))
		{
			// Above and below screen			
			CropLength = spec->BorderCropLength;
		}
		else
		{
			// Center
			CropLength = spec->ScreenCropLength;

			// Adjust every second line
			if ((field_line & 1) == 0)
			{
				CropLength += spec->ScreenCropAlternate;
			}
		}
	}
}

static bool_t NewField = FALSE;

ISR(ANA_COMP_vect) 
{
	// Sync pulse falling edge. 

	TCNT1 = 0;
	OCR1A = 0xFFFF;

	// Make sure we have the right phase on the output compare pin 
	// controlling video switch:

	if ((PINA & (1 << HAL_VID_SW_PIN)) == 0)
	{
		TCCR1C |= (1 << FOC1A);
	}

	uint8_t x = 0;

	while ((ACSR & (1 << ACO)) == 0)
	{
		// Waiting for rising edge, with timeout
		x += 1;

		if (x > 50)
		{
			break;
		}		
	}

	if (x > 50)
	{
		
		// Timeout, this is a long pulse

		if (NewField == FALSE)
		{
			NewField = TRUE;
			// There are multiple pulses like this, first time
			FieldLine = 0;
			FieldCount += 1;

			if (FieldCount == 100)
			{
				FieldCount = 0;
				led2Set(FALSE);
			}
			else if (FieldCount == 50)
			{
				led2Set(TRUE);
			}

			setupCrop(FieldLine, &CropSpecs[Mode]);

			// Enable output compare
			TCCR1A = (1 << COM1A0);
			
			led1Set(FALSE);		
		}
	}
	else
	{
		uint16_t d;
		d = ICR1;

		if (CropStart != 0)
		{
			d += CropStart;

			OCR1A = d;

			while (TCNT1 < d);
			
			OCR1A = d + CropLength;
		}

		// Prepare next line now
		if (FieldLine < 500)
		{
			FieldLine += 1;
			
			setupCrop(FieldLine, &CropSpecs[Mode]);
		}
		else
		{
			// Way too many lines, don't crop anything
			CropStart = 0;
		}

		NewField = FALSE;
	}
}

ISR(TIM1_OVF_vect)
{
	// Timer overflows, no signal, make sure capture output is disabled
	TCCR1A = 0; 
	led1Set(TRUE);
	led2Set(FALSE);
}


void comparatorInit(void)
{

	// Disable input buffers on ADC0, 1, 2 (covers AIN0 AIN1)
	DIDR0 = (1 << ADC0D) | (1 << ADC1D) | (1 << ADC2D);

    ACSR = (0 << ACD)   | // Comparator ON
		   (1 << ACIE)  | // Comparator Interrupt enable
		   (1 << ACIC)  | // Input capture enabled
		   (1 << ACIS1) | (0 << ACIS0); // Set interrupt on output toggle		
}


void timerInit(void)
{
	TCCR1A = 0; // Output compare disabled (enable with: TCCR1A = (1 << COM1A0); )
	

	TCCR1B = 
		(1 << CS10) |  // CS10 only, selects direct clock, no prescaling
		(0 << ICNC1) | // Input capture noise cancelling, not necessary
		(1 << ICES1);  // Input capture edge select. Setting this bit makes the CAPT interrupt fire at rising edge of sync pulse

	
	TIMSK1 = (1 << TOIE1); // (1 << ICIE1);

	OCR1A = 0xFFFF;
}

// UI

static uint8_t SwitchDebounce;
static bool_t SwitchPushed;

#define DEBOUNCE_COUNT 5

void updateButton(void)
{

	if (switchRead())
	{
		if (SwitchDebounce < DEBOUNCE_COUNT);
		{
			SwitchDebounce += 1;
			if (SwitchDebounce == DEBOUNCE_COUNT)
			{
				SwitchPushed = TRUE;
			}
		} 
	}
	else
	{
		SwitchDebounce = 0;
	}

}



// 
int main(void)
{
	ioInit();
	comparatorInit();
	timerInit();

	// Globally enable interrupts
    sei(); 
	
    while (TRUE)
    {
		_delay_ms(10);
		updateButton();

		if (SwitchPushed)		
		{
			SwitchPushed = FALSE;

			Mode = (Mode + 1) % MODES;
		}


    }

    return 0;
}
