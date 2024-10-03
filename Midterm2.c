/* 
 * File:   Midterm2.c
 * Author: Alain Rosenthal
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "LiquidCrystal.h"


# define _XTAL_FREQ 1000000
# define SWITCH1 PORTDbits.RD3 
# define SWITCH2 PORTDbits.RD4
void __interrupt() pot_sample(void);

volatile int num_pot1;
volatile int num_pot2;

void getProduct(int num1, int num2);

int main() 
{
    
    // Configure LCD Pins
    // Data pins connected to PORTB
    TRISB = 0x00; 
    
    // RS = RD0
    // RW = RD1
    // E  = RD2
    TRISD = 0xF8;
    
//    INTCON = 0x00;
    // connect the LCD pins to the appropriate PORT pins
    pin_setup(&PORTB, &PORTD);
    
    // initialize the LCD to be 16x2 
    begin(16, 2, LCD_5x8DOTS);
    
    
    //--------------------------------------------------------------------------
    // 1 - Configure the A/D Module

    // * Configure analog pins, voltage reference and digital I/O 
    // Reference voltages are VSS and VDD
    ADCON1 = 0x0D;
    TRISAbits.RA0 = 1; // connected to potentiometer1
    TRISAbits.RA1 = 1; // connected to potentiometer2

    // * Select A/D acquisition time
    // * Select A/D conversion clock
    // Right justified, ACQT = 2 TAD, ADCS = FOSC/2
    ADCON2bits.ADCS = 0; // FOSC/2
    ADCON2bits.ACQT = 1; // ACQT = 2 TAD
    ADCON2bits.ADFM = 1; // Right justified

    // * Select A/D input channel
    ADCON0bits.CHS = 0; // Channel 0 (AN0), starting with the potentiometer

    // * Turn on A/D module
    ADCON0bits.ADON = 1;   
    
    // 2 - Configure A/D interrupt (if desired)
    // * Clear ADIF bit
    // * Set ADIE bit
    // * Select interrupt priority ADIP bit
    // * Set GIE bit
    
    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1;
    IPR1bits.ADIP = 1;
    RCONbits.IPEN = 0; // disable priority levels
    INTCONbits.PEIE = 1; // enable peripheral interrupts
    INTCONbits.GIE = 1;
    
 
 
    while(1)
    {
        // 4- Start conversion: Set GO/DONE(bar) bit
        ADCON0bits.GO = 1;   
        // Display pot num1 on the first line
        home();
        print("(");
        int pot1= num_pot1-511;
        print_int(pot1);
        
        // Check operation
        if (SWITCH1 == 1 && SWITCH2 == 0){
            print(")+(");
            int pot2= num_pot2-511;
            print_int(pot2);
            print(")  ");
            
            setCursor(0, 1);
            int result_pot = pot1+pot2;
            print("     ");
            print_int(result_pot);
            print("     ");
        }
        else if (SWITCH1 == 0 && SWITCH2 == 1){
            print(")-(");
            int pot2= num_pot2-511;
            print_int(pot2);
            print(")  ");
            
            setCursor(0, 1);
            int result_pot = pot1-pot2;
            print("     ");
            print_int(result_pot);
            print("     ");
        }
        else if (SWITCH1 == 1 && SWITCH2 == 1) {
            print(")x(");
            int pot2= num_pot2-511;
            print_int(pot2);
            print(")  ");
            
            setCursor(0, 1);
            print("     ");
            getProduct(pot1, pot2);
            print("     ");
        }
        else{
        }
        
        //clear loop to stop flickering
        while (SWITCH1 == 0 && SWITCH2 == 0){
            clear();
            noDisplay;
        }              
    }
    
    return 0;

}

void __interrupt() pot_sample(void)
{
    
    // test which interrupt called this interrupt service routine
    
    // ADC Interrupt
    if (PIR1bits.ADIF && PIE1bits.ADIE)
    {
        // 5 Wait for A/D conversion to complete by either
        // * Polling for the GO/Done bit to be cleared
        // * Waiting for the A/D interrupt
  
        // 6 - Read A/D result registers (ADRESH:ADRESL); clear bit ADIF, if required
        
        // reset the flag to avoid recursive interrupt calls
        PIR1bits.ADIF = 0;
        
        if (ADCON0bits.CHS == 0) // channel AN0 (potentiometer1)
        {
            num_pot1 = (ADRESH << 8) | ADRESL;
            ADCON0bits.CHS = 1;
        }
        else if (ADCON0bits.CHS == 1) // channel AN1 (potentiometer2))
        {
            num_pot2 = (ADRESH << 8) | ADRESL;
            ADCON0bits.CHS = 0;
        }
    }
    return;
}

void getProduct(int num1, int num2){
    //initialization
    int prod[6] = {0,0,0,0,0,0};
    int dig1[6] = {0,0,0,0,0,0};
    int dig2[6] = {0,0,0,0,0,0};
    int carry=0;
    int s1[6] = {0,0,0,0,0,0};
    int s10[6] = {0,0,0,0,0,0};
    int s100[6] = {0,0,0,0,0,0};
    int numFlag = 0;
    int negFlag = 0;
    int posnum1;
    int posnum2;
    int x =5;
    int y =5;
    int I=5;
    
    //negative flag check
    // make readings into positive
    if (num1 < 0){
        posnum1= abs(num1);
    }
    else{
        posnum1=num1;
    }
    
    if (num2 < 0){
        posnum2= abs(num2);
    }
    else{
        posnum2=num2;
    }
    
    if (num1<0 && num2>0){
        negFlag=1;
    }
    else if (num1>0 && num2<0){
        negFlag=1;
    }
    else{
        negFlag=0;
    }
    //get individual digits to start multiplication
        dig1[5]= posnum1%10;
        dig1[4]= posnum1%100 / 10;
        dig1[3]= posnum1%1000 / 100;
        dig2[5]= posnum2%10;
        dig2[4]= posnum2%100 / 10;
        dig2[3]= posnum2%1000 / 100;
        
        // start of long multiplication and store each digit into an array
        // remember the carry and zeroes
        //ones
            for(I=5; I>=0; I--){
            s1[I]= dig1[x]*dig2[y] + s1[I];
            if (s1[I]>=10){
               s1[I-1] = s1[I]%100/10; 
               s1[I] = s1[I]%10;
            }
            else{
                carry=0;
            }
            y=y-1;
            }
        
        // tens
            carry=0;
            y=5;
            for(I=4; I>=0; I--){
            s10[I]= dig1[x-1]*dig2[y] + carry;
            if (s10[I]>=10){
               carry = s10[I]%100/10; 
               s10[I] = s10[I]%10;
            }
            else {
                carry=0;
            }
            
            y=y-1;
            }
        
        // hundreds
            y=5;
            carry=0;
            for(I=3; I>=0; I--){
            s100[I]= dig1[x-2]*dig2[y] + carry;
            if (s100[I]>=10){
               carry = s100[I]%100/10; 
               s100[I] = s100[I]%10;
            }
            else {
                carry=0;
            }
            y=y-1;
            }
            
        // store the end addition into another carry while watching for the carry
            carry=0;
            for(I=5; I>=0; I--){
                prod[I]= s1[I]+s10[I]+s100[I]+carry;
            if (prod[I]>=10){
               carry = prod[I]%100/10; 
               prod[I] = prod[I]%10;
            }
            else{
                carry=0;
            }
                     
            }
          
        // print the end product array and check for the possible zeroes before the number starts to appear within the array
        // numFlag watches for when the number actually starts
            for(I=0; I<6; I++){
                if (prod[I]==0 && numFlag==0){
                    
                }
                else{
        // negFlag end check to print the "-"
                    if (negFlag==1){
                        print("-");
                        print_int(prod[I]);
                        negFlag=0;
                    } 
                    else{
                        print_int(prod[I]); 
                    }
                    numFlag=1;
                }
           }
        return;
}

