#include "stm32l4xx.h" /* microcontroller information */

/* Define global variables */

int keys [100];

static unsigned int button; //value of button press
static unsigned int col; //what column has been pressed
static unsigned int row; //what row has been pressed
static unsigned int colNum; //what column # has been pressed
static unsigned int rowNum; //what row # has been pressed
static unsigned int pwm; //handler variable
unsigned int i,j,n,k,w; //delay variables
static unsigned int adc_in;
static unsigned int keypad_map [4][4] = {
	{0x01,0x02,0x03,0x0A}, //0,0;1st row
	{0x04,0x05,0x06,0x0B}, //1,0;2nd row
	{0x07,0x08,0x09,0x0C}, //2,0;3rd row
	{0x0E,0x00,0x0F,0x0D} //3,0;4th row
};

static unsigned int pwm_map [4][4]={
	{0x64,0xC8,0x12C,0x3E8},
	{0x190,0x1F4,0x258,0x0},
	{0x2BC,0x320,0x384,0x0},
	{0x0,0x0,0x0,0x0}
};


void Setup() {
	RCC->AHB2ENR |= 0x03;

	GPIOB->MODER &= ~(0x3FC3); //inputs// display and AND, PB[6:3,0] = 00
	GPIOB->MODER |= (0x1540); //outputs// display, PB[6:3] = 01
		
	GPIOA->MODER &= ~0xF;
	GPIOA->MODER |= 0x0E; //pa1 set 11 and pa0 10
	GPIOA->AFR[0] &= ~0xF;
	GPIOA->AFR[0] |= 0x1;

}

void PinSetup1() {
    Setup();

	 //NO NEED FOR OUTPUTS??
	GPIOA->MODER &= ~(0x0FF0FF0); //inputs// column and row, PA[11:8,5:2] = 00
	GPIOA->MODER |= (0x550000); //outputs// column, PA[11:8] = 01

// configure push-pull pins 
	GPIOA->PUPDR &= (0xFFFFF00F); //pull-reset// row, PA[5:2] = 00
	GPIOA->PUPDR |= (0x0550); //pull-up// row, PA[5:2] = 01  
}


void PinSetup2() {
	Setup();
// configure GPIOA pins 
	GPIOA->MODER &= ~(0x0FF0FF0); //inputs// column and row, PA[11:8,5:2] = 00
	GPIOA->MODER |= (0x0550); //outputs// row, PA[5:2] = 01

///configure push-pull pins 
	GPIOA->PUPDR &= ~(0x0FF0000); //pull-reset// row, PA[11:8] = 00
	GPIOA->PUPDR |= (0x00550000); //pull-up// row, PA[11:8] = 01  
} 


/* initialize interrupts used in the program */
void InterruptSetup() { //maybe void in ()
/* Enable SYSCFG clock */
	RCC->APB2ENR |= 0x01; //enable interrupt clock SYSCFG

/* configure port PA0 as input source of EXTI0 */
	SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;//clear EXTI1 bit in config reg ~(0xF)
	SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB;//PB configuration in EXTI0

/* configure and enable EXTI0 as falling-edge triggered */
	EXTI->FTSR1 |= 0x0001; //falling edge trigger enabled
	EXTI->IMR1 |= 0x0001;     //enable (unmask) EXTI0
	EXTI->PR1 = EXTI_PR1_PIF1;

/* Program NVIC to clear pending bit and enable EXTI0 */
	NVIC_ClearPendingIRQ(EXTI0_IRQn); //Clear NVIC pending bit 
	NVIC_EnableIRQ(EXTI0_IRQn); //enable IRQ 

}

void InterruptSetup2() { 
	
	RCC->APB1ENR1 |= 0x01;
	
		TIM2->CR1 |= 0x01;
	
	//TIM2->CCMR1 &= ~0x72;
	TIM2->CCMR1 |= 0x60;
	
	//TIM2-> CCER &= ~0x03;
	TIM2-> CCER |= 0x01;
	
	TIM2->PSC = 0x3;	//((PSC+1)(4000))/(4E6)=T(PERIOD)
	TIM2->ARR=0x3E7;	//PERIOD OF 1000 
	
	//TIM2->CCR1 = 0x0;//SET AS A FRACTION OF 0x3E7 (1000 BASE 10) TO GET DUTYCYCLE
	//NVIC_EnableIRQ(TIM2_IRQn);
}


void samplingtimer() { 
	
	RCC->APB1ENR1 |= 0x020; //set up to trigger every 0.05 seconds, so as to save adc values
	TIM7->PSC = 0x63;
	TIM7->ARR=0x64;
	TIM7->DIER |= TIM_DIER_UIE;
	
	NVIC_EnableIRQ(TIM7_IRQn);
	TIM7->CR1 |= 0x01;
}
//DEBOUNCE

void debounce() { //
	for (i=0; i<15; i++) { //outer loop
		for (j=0; j<40; j++) { //inner loop
			n = j; //dummy operation for single-step test
		} //do nothing
	}
}

/*----------------------------------------------------------*/
/* delay function - do nothing for about 1 second */
/*----------------------------------------------------------*/
void delay() { //
	for (i=0; i<15; i++) { //outer loop
		for (j=0; j<10000; j++) { //inner loop *4000*
			n = j; //dummy operation for single-step test
		}
	}
}

void keypad() {
	button = 0;
	rowNum = 0;
	colNum = 0;

	/* clear unwanted values */
	GPIOB->ODR &= 0xFF87; //mask PB[6:3] to 0

	/* columns output 0 and find which row is 0*/
	//PinSetup1();
	row=0;
	GPIOA->ODR &= (0xF0FF); //set column to output 0, PA[11:8] = 0

	for(k=0; k<4; k++); //delay for values to load

	row = (~GPIOA->IDR & 0x003C); //get row inputs, PA[5:2] = 0***check~
	row = row >> 2; //shift right by 2

	do {
		row = row << 1; //shift left by 1 to find row count
		rowNum++; //add to row count
	} while((row < 0x10) && rowNum<5) ; //can only shift four times

	/* rows output 0 and find which column is 0 */
	PinSetup2();
	debounce();
	col=0;
	GPIOA->ODR &= (0xFFC3); //set row to output 0, PA[5:2] = 0

	for(k=0; k<4; k++); //delay for values to load

	col = (~GPIOA->IDR & 0xF00); //get column inputs, PA[11:8] = 0
	col = col >> 8; //shift right by 8

	do {
		col = col << 1; //shift left by 1 to find column count
		colNum++; //add to 0column count
	} while((col < 0x10) && colNum<5) ; //can only shift four times
	
	button = keypad_map[--rowNum][--colNum];    //test and see if works****
	pwm = pwm_map[rowNum][colNum];
}



/*----------------------------------------------------------*/
/* interrupt handler EXTI0 - keypad has been pressed */
/*----------------------------------------------------------*/
void EXTI0_IRQHandler() { //maybe put void in ()

	//debounce();
	keypad(); //keypad logic
	//display_button(); //display button
	PinSetup1();
	debounce();
	debounce();
	NVIC_ClearPendingIRQ(EXTI0_IRQn); //clear NVIC pending bit with EXTI line 1 interrupt
	EXTI->PR1 |= 0x0001;                //clear EXTI0 pending bit*
	__enable_irq(); //enable interupts* Maybe this has to go before above line

}


void TIM2_IRQHandler(){ 
	
	GPIOB->ODR &= ~0x078;
	GPIOB->ODR |= (button<<3);
	TIM2->SR &= ~0x01;
	__enable_irq();
			
}
void TIM7_IRQHandler(){ 
	
	if(w<100){ //pulls data from adc_in to save to array
	  keys[w] = adc_in;
	  w++;
	}
	TIM7->SR &= ~0x01;
	__enable_irq();
			
}

void adc_setup(){
	//all this is redundant, done previously
	//RCC->AHB2ENR |= 0x01; //ENABLE GPIOA CLOCK
	//GPIOA->MODER &= ~0x0C; //SET PA0 TO ANALOG MODE '11'
	//GPIOA->MODER |= 0x0C; //
	
	
	RCC->AHB2ENR |= 0x2000; //ENABLE ADC CLOCK
	
	RCC->CCIPR &= ~0x30000000; //SLECTING CLOCK
	RCC->CCIPR |= 0x30000000;
	ADC1->CR &= ~0x20000000;//WAKEUP ADC, TURN OFF DEEP SLEEP
	ADC1->CR |= 0x10000000;
	
	delay();//ADD DELAY
	
	ADC1->CR |= 0x1; //ADC ENABLE BIT

	ADC1->CFGR |= 0x2000; //SET CONTINUOUS MODE
	//ADC1->CFGR &= ~0x18;//CHANGES RESOLUTION TO 8BIT
	//ADC1->CFGR |= 0x10;
	ADC1->SQR1 &= ~0xF; //DEFINE SEQUENCE LENGTH
	
	ADC1->SQR1 |= 0x1;
	
	ADC1->SQR1 &= ~0x7Cf; //SELECTING CHANNEL; 6 FOR PA1
	ADC1->SQR1 |= 0x180; 
	
  ADC1->CR |= 0x4; //SETTING THE START BIT FOR THE ADC	

}

/*------------------------------------------------*/
/* main program */
/*------------------------------------------------*/
int main(void) {
	w=0;
		
	GPIOA->PUPDR &= ~0xC; //pull-reset// row, PA[5:2] = 00
	GPIOA->PUPDR |= 0x8; //pull-up// row, PA[5:2] = 01  	
	
	Setup(); //configure clocks and GPIOB pins
	adc_setup();
		
	InterruptSetup();	//configure interrupts
	PinSetup1();
	InterruptSetup2();
	samplingtimer();
	button=0;

	while(1){		
		TIM2->CCR1 = pwm;
		GPIOB->ODR &= ~0x078;
		GPIOB->ODR |= (button<<3);	
				
		adc_in = ADC1->DR; 
		ADC1->ISR |= 0x1;//RESETS INTERRUPT FOR ADC

		}
	
}