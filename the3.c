//Murat Kara 2036010
//Yasin Furkan Aktas 2112134
#include <p18cxxx.h>
#include <p18f8722.h>
#include "LCD.h"
#include "Includes.h"
#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF
#define _XTAL_FREQ   40000000
#define TIMER_START 131;     // 65536-3036 = 62500 (used in the TIMER1 initial value)
#define TIMER1_START 3036;     // 65536-3036 = 62500 (used in the TIMER1 initial value)
#define TIMER_100ms 250;         //loop 250 times (250*125*32) = 1 000 000 instruction cycle
#define TIMER_250ms 625;        //loop 625 times (625*125*32) = 2 500 000 instruction cycle
#define TIMER_500ms 1250;       //loop 1250 times (1250*125*32) = 5 000 000 instruction cycle
#define TIMER_1sec  20;

void init(); 			        //in start and restart after 120sec initiliaze all variables
void first_message();	        //While starting wirte First String message on lCD 
void lighten_7_segment(int d0, int d1, int d2, int d3); //Write 4 numbers given as integers on LCD
void delay_3_sec();		        //After RB1 button delay 3 sec with busy waiting 
char num_to_char(int x,char y);	//Decide writing # or Entered password value 
void set_a_pin_message();		//Show current being setted pin on "LCD screen Set a pin : 43## "
void RE1button(); 				//Wait RE1button
void next_char();				//After user gives a one digit of password,it goes next index in setting
void next_char2();				//After user gives a one digit of password,it goes next index in Attempts
void show_password();			//Show setted password on LCD "The New pin is --4334--" 
void display120sec_Int();		//Write remaining time period of 120sec on 7 Display Segment
void interrupt isr();			//Interrupt Function for ADCON,RB,TMR1,TMR0 
void enter_pin();				//Show current being attempted pin on "LCD screen Enter a pin : 43## "
void main(void);				//Controll overall program 
void endLCD();					//Write End string on LCD "Safe is Opnenning"
void write_wait_20sec();		//Write Wait 20sec after 3 wrong attempts  on LCD "Enter a pin After 20sec. "
int  check_correct_pass();		//Check if entered password and setted password match
int val_inPot_2Int();			//Convert to int(0-9) from potentiometer value

//Function declarations and their explanations are in the "Includes.h" file

unsigned int counter_to_read_val;		//Current active index to enter and set the pin value 
char blank_val[4];				//Helps to apply blink in changing from "#" " "  
bit pin_full;					//Determine if password is entered fully
signed int password_int[4];		//Set password 
signed int entered_pass[4];		//Entered password in attempts (it can have the -1 value to check the blink and 0..9)
unsigned int blink_timer;		//Initial value for blink 250msec in TMR0
unsigned int ADC_timer;			//Initial value for blink 100msec in TMR0
int cur_val_inPot;				//Current value taken from potentiometer
int pre_val_inPot;				//Previous value taken from potentiometer
int diff_val2;					//difference between current and previos value of potentiometer
int diff_val;					//difference between current and previos value of potentiometer
int val_inPot;					//Potentiometer value to use in coverting potVal
bit b6_pressed;					//Check if b6 pressed
int sh_pass_timer;				//In?t?al timer value(for the 500msec blink)
bit isBlank;					//in 500msec blink case, to check if the LCD is clean 
int sh_pass_count;				//Used to Blink 3 times after setting Pin   
unsigned int counter_120sec;	//Counter value to count 120sec 
unsigned int counter_20sec;		//Counter value to count 20sec
unsigned int flag120sec;		//Check if in 120sec interval or not 
unsigned int flag20sec;			//Check if in 20sec interval or not 
unsigned int timer_1sec;		//Initial value for blink 1sec in TMR1
unsigned int attempt_num;		//User remaining attempt number
bit line_toggle;				//flag line will make toggle(to make 'show password' work once at a time in the loop)
bit toggle ;					//flag password digit will make toggle(to make 'enter a pin' work once at a time in the loop)
bit isEnterPin;					//If User entered a pin before or not (in the enter pin function, it will just change the #### part for optimization )


void init()
{
    TRISH4 = 1;                     //Determine Potentiometer channel 
    ADCON0 = 0b00110000;            // setting initial chanel
    ADCON1 = 0b00000000;            // setting limits of voltage and analog all.
    ADCON2 = 0b10001111;            // setting right justified
    ADIE = 1;                       // enable A/D interrupt
    ADIF = 0;						//Clear A/D Flag

    INTCONbits.TMR0IE = 1;          // enable TMR0
    INTCONbits.TMR0IF = 0;          // clear timer0  flag
    T0CON = 0b01000100;             // pre-scale is  1:32 and  8-bit TMR0
    TMR0 = TIMER_START;             //loop 250 times (250*(250-131)*32) = 1 000 000 instruction cycle

    TMR1IE = 1;                     // enable timer1 interrupts
    T1CON = 0b10110000;             // pre-scale 1:8, 16-Bit TMR1
    TMR1=TIMER1_START;        		// set timer1 initial value to produce 1sec

    TRISB6 = 1;                     // determine RB6 as input pin for RB interrupt
    TRISB7 = 1;                     // determine RB7 as input pin for RB interrupt
    TRISB5 = 0;                     // set other pins of RB as output pin for safety
    TRISB4 = 0;						// set other pins of RB as output pin for safety
    TRISB3 = 0;                     // set other pins of RB as output pin for safety
    TRISB2 = 0;						// set other pins of RB as output pin for safety
    TRISB1 = 0;                     // set other pins of RB as output pin for safety
    TRISB0 = 0;						// set other pins of RB as output pin for safety
    PORTB = 0;                      // clear PORTB for safety
    LATB = 0;						// Clear LATB for safety
    INTCONbits.RBIE = 1;            // enable RB interrupts
    INTCONbits.RBIF = 0;            // clear RB interrupt flag
    INTCON2bits.RBPU = 0;           //do not disable pull ups
    INTCON2bits.INTEDG0 = 0;        //enter the RB interrupt in the rising edge
    TRISE1 = 1;						// determine RE1 as input to start program
    TRISJ = 0;                      // determine 7 segment display as output
    LATJ = 0;                       // clear LATJ for safe
    LATH = 0;						// clear LATH for safe
    TRISH = TRISH & 0b11110000;   //to show the 
    counter_to_read_val = 0;		//index User is entering of pin
    for(unsigned int i =0;i<4;i++)			//Initialize blank val to make blink	
    {	
        blank_val[i] = '#';
    }
    for(unsigned int i =0;i<4;i++)			//Initialize user entered and set pins	
    {
        password_int[i] = -1;
        entered_pass[i]= -1;
    }
    
    pin_full = 0;					//Initialize pin is not full
    val_inPot = -1;					//potentiometer value initialize
    blink_timer = TIMER_250ms;		//In?tialize blink timer to produce 250ms
    ADC_timer = TIMER_100ms;		//In?tialize ADC timer to produce 100ms
    sh_pass_timer = TIMER_500ms;	//In?tialize settedpassword timer to produce 500ms
    cur_val_inPot = -1;				//Current potentiometer value initialize
    pre_val_inPot = -1;				//Previous potentiometer value initialize
    diff_val2 = 0;					//Difference btw cur- pre potentiometer value initialize
    diff_val = 0;					//Difference btw cur- pre potentiometer value initialize
    b6_pressed = 0;					//In?tialize if b6_pressed
    isBlank = 0;
    sh_pass_count = 0;				//In?tialize how many times is blinked after  setting pin
    timer_1sec=TIMER_1sec;			//In?tialize 1sec timer to produce 1sec
    counter_120sec=120;				//In?tial 120sec value
    counter_20sec=20;				//In?tial 20 sec value
    flag20sec=0;					//not 20sec interval in initial
    flag120sec=0;					//not 120sec interval in initial
    attempt_num=2;					//2 Attempts in inital
    toggle =0;						//toggle is zero in inital
    line_toggle = 0;                // same as isEnterPin(below) except this is for showing the set password
    isEnterPin = 0;					//User didn't enter a digit in initial
}


void first_message()
{
    WriteCommandToLCD(0x80);   // Start of the first line
    WriteStringToLCD(" $>Very  Safe<$ ");    // Write first row 
    WriteCommandToLCD(0xC0); // Start of the second line
    WriteStringToLCD(" $$$$$$$$$$$$$$ ");    // Write the second row

}

void display_7_segment(int d0, int d1, int d2, int d3)
{
    
    LATH = LATH & 0xF0;			//Clear last 4 digits
    
    int symbols[]={
        0b00111111,				// 7-Display-Segment ,0
        0b00000110,    	        // 7-Display-Segment,1
        0b01011011,             // 7-Display-Segment = 2
        0b01001111,            	// 7-Display-Segment = 3
        0b01100110,             // 7-Display-Segment = 4
        0b01101101,             // 7-Display-Segment = 5
        0b01111101,             // 7-Display-Segment = 6
        0b00000111,             // 7-Display-Segment = 7
        0b01111111,             // 7-Display-Segment = 8
        0b01100111,             // 7-Display-Segment = 9
        0b01000000,             // 7-Display-Segment = -
        0b00000000              // 7-Display-Segment = Blank
    };

    
    LATJ = symbols[d3];                 //Turn On digit 3
    LATH3 = 1;                          
    __delay_us(500);                    //little delay
    LATH3 = 0;                          

    LATJ = symbols[d2];                 //Turn On digit 2
    LATH2 = 1;                          
    __delay_us(500);                    //little delay
    LATH2 = 0;                          

    LATJ = symbols[d1];                 //Turn On digit 1 
    LATH1 = 1;                          
    __delay_us(500);                    //little delay
    LATH1 = 0;                          

    LATJ = symbols[d0];                 //Turn On digit 0
    LATH0 = 1;                          
    __delay_us(500);                    //little delay
    LATH0 = 0;                          

    return;
}
void delay_3_sec()
{
    int c;
    c = 0;
    while(c++ < 1481){					//Busy wait
        display_7_segment(10,10,10,10);	//Write 7-display-segment
    }
    c=0;
}

char num_to_char(int x,char y)
{
    if(x == -1) 			//If pin digit is not entered yet give ' ' or '#'char 
    {
        return y;
    }
    else					//othwise give entered pin digit
        return (char)(((int)'0') + x);
}

void set_a_pin_message()
{

    WriteCommandToLCD(0x80); //Start of the first line
    WriteStringToLCD(" Set a pin:");    // Write first part of first line


    WriteCommandToLCD(0x8B);
    WriteDataToLCD(num_to_char(password_int[0],blank_val[0]));    // First digit

    WriteCommandToLCD(0x8C);
    WriteDataToLCD(num_to_char(password_int[1],blank_val[1]));    // Second digit

    WriteCommandToLCD(0x8D);
    WriteDataToLCD(num_to_char(password_int[2],blank_val[2]));    // Third digit

    WriteCommandToLCD(0x8E);
    WriteDataToLCD(num_to_char(password_int[3],blank_val[3]));    // Fourth digit


    WriteCommandToLCD(0xC0); // Goto to the beginning of the second line
    WriteStringToLCD("                ");    // Write Blank line

}

void RE1button(){
    RBIE = 0;                           	// disable shortly RB interrupts
    while(PORTEbits.RE1){               	// Busy waiting
        display_7_segment(10,10,10,10);  	//Write 7-display-segment "-.-.-.-"
    }
    while(!PORTEbits.RE1){              	// Busy waiting 
        display_7_segment(10,10,10,10);  	//Write 7-display-segment "-.-.-.-"
    }
    ClearLCDScreen();                   	// Clear LCD
    RBIE = 1;                           	// Enable RB interrupts again

}


void next_char()
{	
//After user gives a one digit of password,it goes next index in setting
    if(counter_to_read_val<3 && password_int[counter_to_read_val] != -1) 
        counter_to_read_val++;
    b6_pressed = 0;
}

void next_char2()
{	
//After user gives a one digit of password,it goes next index in attempt
    if(counter_to_read_val<3 && entered_pass[counter_to_read_val] != -1)
        counter_to_read_val++;
    b6_pressed = 0;
}

void show_password()
{	
    if(isBlank == 1)
    {					
        ClearLCDScreen();							//when blank clear screen and after 500msec enters else part
	}
    else
    {
        WriteCommandToLCD(0x80); 					// Start of the first line
        WriteStringToLCD(" The new pin is ");    	// Write First line

        WriteCommandToLCD(0xC0);					
        WriteStringToLCD("   ---");	

        WriteCommandToLCD(0xC6);
        WriteDataToLCD((char)(((int)'0') + password_int[0]));	//First digit of setted pin

        WriteCommandToLCD(0xC7);
        WriteDataToLCD((char)(((int)'0') + password_int[1]));	//Second digit of setted pin

        WriteCommandToLCD(0xC8);
        WriteDataToLCD((char)(((int)'0') + password_int[2]));	//Third digit of setted pin

        WriteCommandToLCD(0xC9);
        WriteDataToLCD((char)(((int)'0') + password_int[3]));	//Fourth digit of setted pin

        WriteCommandToLCD(0xCA);
        WriteStringToLCD("---   ");
    }

    line_toggle = 1;


}

void display120sec_Int()
{

    if(counter_120sec==0)				//If 120 sec is over go reset
        Reset();	
        unsigned int a1,a2,a3,a4;		//Seperate digits
        a1=counter_120sec%10;		
        a2=(counter_120sec/10)%10;
        a3=counter_120sec/100;
        a4=0;
        display_7_segment(a4,a3,a2,a1);	//Write on 7-display-segment
}

int val_inPot_2Int()
{
    if(val_inPot<100)	//0 from Pot val
        return 0;
    if(val_inPot<200)	//1 from Pot val
        return 1;
    if(val_inPot<300)	//2 from Pot val
        return 2;
    if(val_inPot<400)	//3 from Pot val
        return 3;
    if(val_inPot<500)	//4 from Pot val
        return 4;
    if(val_inPot<600)	//5 from Pot val
        return 5;
    if(val_inPot<700)	//6 from Pot val
        return 6;
    if(val_inPot<800)	//7 from Pot val
        return 7;
    if(val_inPot<900)	//8 from Pot val
        return 8;
    if(val_inPot<1025)	//9 from Pot val
        return 9;
    else				//0 from Pot val for safe
        return 0;
}



void interrupt isr()
{

    if(RBIE && RBIF)									//Check RB interrupts 
    {
        if(PORTBbits.RB6 == 0)							//Check RB6 released
        {
            b6_pressed = 1;								//set flag
        }
        if(PORTBbits.RB7 == 0)							//Check RB7 released
        {
            if(counter_to_read_val == 3)				//Check entered fully
            {
                pin_full = 1;							//set pin_full entered flag
            }
        }
        RBIF = 0;										//clear interrupt flag
        return;
    }
    if(TMR0IE && TMR0IF)								//Check TMR0 interrupts for 100msec
    {
        TMR0IF = 0;										//clear flag
        if( --blink_timer == 0)							//if 250msec passed 
        {
            if(blank_val[counter_to_read_val] == '#')	//Toggle from '#' to ' '
            {
                blank_val[counter_to_read_val] = ' ';
            }
            else										//Toggle from ' ' to '#'
            {
                blank_val[counter_to_read_val] = '#';
            }
            blink_timer = TIMER_250ms;					//set again timer to calculate new 250msec
            toggle = 1;													
        }
        if(--ADC_timer==0)								//If 100msec is passed
        {
            ADCON0bits.GO_DONE=1;						//send digital value to ADRES 
            ADC_timer=TIMER_100ms;						//set again ADC_timer for next one 
        }
        if(pin_full == 1 && --sh_pass_timer==0)			//If Entered all pin 500msec blink
        {
            if(sh_pass_count<5)							// 3 Show password  2 Blank  
            {
                if(isBlank == 0) isBlank = 1;			//Toggle
                else isBlank = 0;
                sh_pass_timer=TIMER_500ms;				//Reset timer for 500msec
                sh_pass_count++;						//increment toggle count
                line_toggle =0;
            }
            else										//After Showing password 3 times
            {   for(unsigned int i=0;i<4;i++)					//Reset blank vals to show "####"
                    blank_val[i]='#';
                flag20sec=0;							//20sec interval not started yet 
                flag120sec=1;							//120sec iterval is started
                pin_full =0;							//pin is not entered 
                counter_to_read_val=0;					//User initial set index is Zero
                //line_toggle = 0;
            }
        }
        TMR0 = TIMER_START;								//reset timer0 
        return;
    }
    if(TMR1IE && TMR1IF )								//Check TMR interrupts for 1sec
    {
        TMR1IF=0;										//Clear flag
        if( flag120sec==1 && --timer_1sec==0  )			//if 120sec interval is started  
        {
            if(flag120sec==1 ){							//decrement each 1sec remaining from 120sec
                counter_120sec--;
            }
            if(flag20sec==1)							//if 20sec interval is started
            {
                counter_20sec--;						//decrement remaining from 20sec
                if(counter_20sec==0)
                {
                counter_20sec=20;						//reset it 
                flag20sec=0;							//clear flag to exit from 20sec
                isEnterPin = 0;							//clear flag  since current entered pin will be clear 
                }
                
            }
            timer_1sec=TIMER_1sec;						//reset timer1sec counter
        }
        TMR1=TIMER1_START;								//reset timer1
        return;
    }
    if(ADIE && ADIF )									//Check A/D interrupts
    {
        ADIF =0;										//Clear interrupts flag
            cur_val_inPot=ADRES;						//update current potval
            diff_val=cur_val_inPot-pre_val_inPot;		//take difference previous and current pot vals
            diff_val2=pre_val_inPot-cur_val_inPot;

            if(pre_val_inPot != -1 && ( diff_val>=10  || diff_val2>=10)  )	//If difference is larger than 10
        {
            val_inPot=cur_val_inPot;					//Update potval used for Enter pin digit
            if(flag120sec==0)							//If pin is not setted
            {		
                password_int[counter_to_read_val]= val_inPot_2Int();	//Show Int calue of potval in digit on LCD
            }
            else if(flag120sec==1)						//If pin is setted and attempting pin
            {
                entered_pass[counter_to_read_val]= val_inPot_2Int();	//Write digit of Entering pin
            }
            pre_val_inPot=cur_val_inPot;								//Update previous val of potval
        }
        else															//If not pass treshold
        {
            pre_val_inPot=cur_val_inPot;								//Update previous val of potval
        }
            return;
    }
}

void enter_pin()
{   
    if(isEnterPin == 0) //Check pin is entered before if not  write all LCD screen
    {
    display120sec_Int();
    WriteCommandToLCD(0x80); // Start of the first line
    WriteStringToLCD(" Enter pin:");    // Write first part of first line
    
	WriteCommandToLCD(0x8B);
    WriteDataToLCD(num_to_char(entered_pass[0],blank_val[0]));    // First digit entered Val
    
    WriteCommandToLCD(0x8C);
    WriteDataToLCD(num_to_char(entered_pass[1],blank_val[1]));    // Second digit entered Val

    WriteCommandToLCD(0x8D);
    WriteDataToLCD(num_to_char(entered_pass[2],blank_val[2]));    // Third digit entered val

    WriteCommandToLCD(0x8E);
    WriteDataToLCD(num_to_char(entered_pass[3],blank_val[3]));    // Fourth digit entered val

    WriteCommandToLCD(0xC0); 							// Start of the second line
    WriteStringToLCD("  Attempts:");    				// Write first part of first line

    WriteCommandToLCD(0xCB);							// write attempt Number
    WriteDataToLCD((char)(((int)'0') + attempt_num));	

    WriteCommandToLCD(0xCC); // remaining  of the second line
    WriteStringToLCD("    ");
    isEnterPin = 1;
    }
    else	//if is entered before just update being entered  pin
    {

        WriteCommandToLCD(0x8B);
        WriteDataToLCD(num_to_char(entered_pass[0],blank_val[0]));    	// First digit
 
        WriteCommandToLCD(0x8C);
        WriteDataToLCD(num_to_char(entered_pass[1],blank_val[1]));    	// Second digit
            
        WriteCommandToLCD(0x8D);
        WriteDataToLCD(num_to_char(entered_pass[2],blank_val[2]));    	// Third digit

        WriteCommandToLCD(0x8E);
        WriteDataToLCD(num_to_char(entered_pass[3],blank_val[3]));    	// Fourth digit
   
        WriteCommandToLCD(0xCB);										
        WriteDataToLCD((char)(((int)'0') + attempt_num));				//attempt number

    }
   
    toggle =0;
}



void endLCD(){

    WriteCommandToLCD(0x80);   					// Start of the first line
    WriteStringToLCD("Safe is opening!");    	// Write first line
    WriteCommandToLCD(0xC0); 					// Start of the second line
    WriteStringToLCD("$$$$$$$$$$$$$$$$");    	// Write second line

}

void write_wait_20sec()
{
    WriteCommandToLCD(0x80);   					// Start of the first line
    WriteStringToLCD(" Enter pin:XXXX ");    	// Write first line 
    WriteCommandToLCD(0xC0); 					// Start of the second line
    WriteStringToLCD("Try after 20sec.");    	// Write second line
}
int  check_correct_pass()
{
    for(unsigned int i=0 ; i<4; i++)
    {
        if(entered_pass[i]!=password_int[i]) 	//If m?smatch between masin pin and entered pin, return not correct
            return 0;
    }
    return 1; 									//Otherwise,return correct
}

void main(void)
{
    for(int i=0; i<5;i++)		//little delay at start of program 
    	__delay_ms(15);

    InitLCD();            		// Initialize LCD 

    ClearLCDScreen();           // Clear LCD screen
    init();						//initilizer of all variables
    first_message();			//First message on lCD screen
    RE1button();				//Check RE1 button is pushed and released

    first_message();			//write first message again
    delay_3_sec();				//3sec delay with busy waiting

    ADON = 1;					//A/D converter is enabled
    INTCONbits.GIE_GIEH = 1;	//Enable all interrupts
    INTCONbits.PEIE_GIEL = 1;

    TMR0ON = 1;					//Enable TMR0
    TMR1ON = 1;					//Enable TMR1

    while(1)
    {
        if(flag120sec == 0)		//If Pin not setted yet
        {
            if(pin_full == 0 )	//If not entered fully
            {
                if(b6_pressed == 1)		//If b6_presed
                {
                    next_char();		//Take next char
                }
                set_a_pin_message();	//Write pin message in setting stage

            }
            else if(pin_full == 1)		//If fully setted pin
            {
                display_7_segment(10,10,10,10);	
                if(line_toggle == 0)		//If line will not make toggle	
                    show_password();		//Show setted password
                display_7_segment(10,10,10,10);
            }
            }
        else
        {
            display120sec_Int();				//Write remaining time from 120sec on 7-display-segment
            if(pin_full==0 && flag20sec==0 )	//If pin not full entered and not locked
            {
                if(b6_pressed == 1)				//If b6_pressed
                {
                    next_char2();				//take next pin digit
                }
                
                if(toggle == 1)					//If digit make a toogle 
                    enter_pin();				//continue enter pin 
            }
            else if(pin_full==1)
            {					
                if(check_correct_pass()==1){	//If correct pin entered
                   endLCD();					//Write endString on  LCD
                   INTCONbits.GIE_GIEH = 0;		//Disable interrupts
                   INTCONbits.PEIE_GIEL = 0;	
                   while(1)						//Busy wait
                   {
                      display120sec_Int();		//Display remaining time constantly on 7-segment-display
                   }
                }
                else if(attempt_num !=0 )		//If user entered wrongly and  has more attempts
                {
                    pin_full=0;   				//pin is reset
                    for(unsigned int i=0 ; i<4 ;i++)		
                    {
                        blank_val[i]='#';		//Reset blank val to blink
                        entered_pass[i]=-1;		//Reset being entered pin
                    }
                    attempt_num--;				//decrement attempt-num
                }
                else if(attempt_num ==0 )		//If User entered wrongly and has no more attempts
                {
                    for(unsigned int i=0 ; i<4 ;i++)
                    {
                        blank_val[i]='#';		//Reset blank val to blink
                        entered_pass[i]=-1;		//Reset entered pass 
                    }
                    write_wait_20sec();			//Write For waiting 20sec on LCD
                    flag20sec=1;				//set flag 20sec interval is started and  it will not enterpin scope without clear flag20sec  
                    pin_full=0;					//set flag since pin will be made empty
                    attempt_num=2;				//new_attempt is given 
                }
                counter_to_read_val=0;			//after pin is full index will be zero
            }
        }
    }
}
