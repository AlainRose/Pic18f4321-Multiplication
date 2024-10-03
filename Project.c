/*
 * Author: Alain Rosenthal
 *
 * Description: This code samples data from two potentiometers, performs basic arithmetic
 * operations (+, -, x) based on switch inputs, and displays the results on a 16x2 LCD.
 */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "LiquidCrystal.h"

// Define system clock frequency
#define _XTAL_FREQ 1000000

// Define switch pins
#define SWITCH1 PORTDbits.RD3
#define SWITCH2 PORTDbits.RD4

// Interrupt function prototype
void __interrupt() pot_sample(void);

// Declare volatile variables for potentiometer values
volatile int num_pot1;
volatile int num_pot2;

// Function prototype to calculate product of two numbers
void getProduct(int num1, int num2);

int main() 
{
    // Configure LCD data pins on PORTB
    TRISB = 0x00; 
    
    // Configure control pins for LCD: RS = RD0, RW = RD1, E = RD2
    TRISD = 0xF8;
    
    // Setup and initialize the LCD (16x2)
    pin_setup(&PORTB, &PORTD);
    begin(16, 2, LCD_5x8DOTS);
    
    //-----------------------------------------------------------------------
    // 1 - Configure A/D Module for potentiometer inputs

    // Configure analog pins (RA0, RA1) and reference voltages (VSS, VDD)
    ADCON1 = 0x0D;
    TRISAbits.RA0 = 1; // Potentiometer 1 on RA0
    TRISAbits.RA1 = 1; // Potentiometer 2 on RA1

    // Configure A/D acquisition time and conversion clock
    ADCON2bits.ADCS = 0; // FOSC/2
    ADCON2bits.ACQT = 1; // Acquisition time = 2 TAD
    ADCON2bits.ADFM = 1; // Right justified result

    // Start with potentiometer 1 (AN0)
    ADCON0bits.CHS = 0;

    // Turn on A/D module
    ADCON0bits.ADON = 1;   
    
    //-----------------------------------------------------------------------
    // 2 - Configure A/D Interrupt

    PIR1bits.ADIF = 0;    // Clear ADIF flag
    PIE1bits.ADIE = 1;    // Enable A/D interrupt
    IPR1bits.ADIP = 1;    // Set interrupt priority
    RCONbits.IPEN = 0;    // Disable priority levels
    INTCONbits.PEIE = 1;  // Enable peripheral interrupts
    INTCONbits.GIE = 1;   // Enable global interrupts
    
    //-----------------------------------------------------------------------
    // Main loop
    while(1)
    {
        // Start A/D conversion
        ADCON0bits.GO = 1;
        
        // Display potentiometer values on LCD
        home();
        print("(");
        int pot1 = num_pot1 - 511;
        print_int(pot1);
        
        // Check switch combinations and perform corresponding operations
        if (SWITCH1 == 1 && SWITCH2 == 0){
            // Addition case
            print(")+(");
            int pot2 = num_pot2 - 511;
            print_int(pot2);
            print(")  ");
            
            setCursor(0, 1);
            int result_pot = pot1 + pot2;
            print("     ");
            print_int(result_pot);
            print("     ");
        }
        else if (SWITCH1 == 0 && SWITCH2 == 1){
            // Subtraction case
            print(")-(");
            int pot2 = num_pot2 - 511;
            print_int(pot2);
            print(")  ");
            
            setCursor(0, 1);
            int result_pot = pot1 - pot2;
            print("     ");
            print_int(result_pot);
            print("     ");
        }
        else if (SWITCH1 == 1 && SWITCH2 == 1) {
            // Multiplication case
            print(")x(");
            int pot2 = num_pot2 - 511;
            print_int(pot2);
            print(")  ");
            
            setCursor(0, 1);
            print("     ");
            getProduct(pot1, pot2);
            print("     ");
        }
        
        // Clear the display if no switches are pressed
        while (SWITCH1 == 0 && SWITCH2 == 0){
            clear();
            noDisplay();
        }
    }
    
    return 0;
}

// Interrupt Service Routine for A/D Conversion
void __interrupt() pot_sample(void)
{
    // Check if the A/D interrupt flag is set
    if (PIR1bits.ADIF && PIE1bits.ADIE)
    {
        PIR1bits.ADIF = 0; // Clear interrupt flag
        
        // Read A/D result and switch channels between AN0 and AN1
        if (ADCON0bits.CHS == 0) // Potentiometer 1
        {
            num_pot1 = (ADRESH << 8) | ADRESL;
            ADCON0bits.CHS = 1; // Switch to AN1
        }
        else if (ADCON0bits.CHS == 1) // Potentiometer 2
        {
            num_pot2 = (ADRESH << 8) | ADRESL;
            ADCON0bits.CHS = 0; // Switch back to AN0
        }
    }
    return;
}

// Function to calculate the product of two numbers and display the result
void getProduct(int num1, int num2){
    // Initialize variables and arrays for multiplication
    int prod[6] = {0};
    int dig1[6] = {0};
    int dig2[6] = {0};
    int carry = 0;
    int s1[6] = {0}, s10[6] = {0}, s100[6] = {0};
    int numFlag = 0, negFlag = 0;
    int posnum1 = abs(num1), posnum2 = abs(num2); // Absolute values
    int x = 5, y = 5, I = 5;
    
    // Check for negative product
    negFlag = (num1 < 0) ^ (num2 < 0);
    
    // Extract digits of both numbers
    dig1[5] = posnum1 % 10;
    dig1[4] = posnum1 / 10 % 10;
    dig1[3] = posnum1 / 100 % 10;
    dig2[5] = posnum2 % 10;
    dig2[4] = posnum2 / 10 % 10;
    dig2[3] = posnum2 / 100 % 10;
    
    // Perform long multiplication for ones, tens, and hundreds
    for(I = 5; I >= 0; I--){
        s1[I] = dig1[x] * dig2[y];
        y--;
    }
    
    // Add up results and handle carryovers
    for(I = 5; I >= 0; I--){
        prod[I] = s1[I] + s10[I] + s100[I] + carry;
        carry = prod[I] / 10;
        prod[I] %= 10;
    }
    
    // Display result on LCD
    for(I = 0; I < 6; I++){
        if (prod[I] == 0 && numFlag == 0){
            continue; // Skip leading zeros
        } else {
            if (negFlag){
                print("-");
                negFlag = 0;
            }
            print_int(prod[I]);
            numFlag = 1;
        }
    }
}
