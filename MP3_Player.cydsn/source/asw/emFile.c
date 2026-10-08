/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

/*******************************************************************************
* File Name: main.c
*
* Version: 1.10
*
* Description:
*  The example project consists of 5 simple steps. Each of the steps performs 
*  single File System operation. 
*  Step#1: Gets SD card name and displays it on the LCD.
*  Step#2: Performs Format SD operation and displays the status of the format 
*  operation execution.
*  Step#3: Creates directory with the name - "Dir" and displays a status of 
*  directory creation operation.
*  Step#4: Creates a file - "File.txt" in the root directory and writes the 
*  following string to the file -"123456789". Displays a status of operations 
*  execution.
*  Step#5: Copies newly created file ("File.txt") to directory "Dir" with the 
*  name - "File1.txt". 
*
********************************************************************************
* Copyright 2011-2012, Cypress Semiconductor Corporation. All rights reserved.
* This software is owned by Cypress Semiconductor Corporation and is protected
* by and subject to worldwide patent and copyright laws and treaties.
* Therefore, you may use this software only as provided in the license agreement
* accompanying the software package from which you obtained this software.
* CYPRESS AND ITS SUPPLIERS MAKE NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* WITH REGARD TO THIS SOFTWARE, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT,
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*******************************************************************************/

#include <project.h>
#include <FS.h>
#include <string.h>
#include "global.h"
#include <stdio.h>
#include "emFile.h"

char sdFile[9] = "File.txt";


char UART_BUF[100];

void NextStep(U16 step, char * str)
{
    /* Clear LCD after previously completed operation */
    //LCD_ClearDisplay();
    
    /* This will display step number on the LCD prior
    * every operation on the SD card.
    */
    sprintf(UART_BUF,"\nStep# %d, %s\n",step,str);
    UART_LOG_PutString(UART_BUF);
    /*
    LCD_Position(0u, 0u);
    LCD_PrintString("Step#");
    LCD_PrintDecUint16(step);
    LCD_Position(1u, 0u);
    LCD_PrintString(str);
    */
    
    /* Need some delay to make step number visible */
    //CyDelay(2000u);
    
    /* Clear LCD after step number was prompted */
    //LCD_ClearDisplay();
}

void emFile()
{
    char sdVolName[10];     /* Buffer that will hold SD card Volume name */
    U16 step = 1u;
    FS_FILE * pFile;
    
    /* Start LCD */
    //LCD_Start();
    
    /* Initialize file system */
    FS_Init();
    
    /* Display step number */
    NextStep(step++, "SD CARD NAME");
    
    //LCD_Position(0u, 0u);
    
    /* Get volume name of SD card #0 */
    if(0 != FS_GetVolumeName(0u, &sdVolName[0], 9u))
    {
        /* Getting volume name succeeded so prompt it on the LCD
        LCD_PrintString("SD card name:");
        LCD_Position(1u, 0u);
        LCD_PrintString(sdVolName); */
        sprintf(UART_BUF,"SD card name: %s\n", sdVolName);
        UART_LOG_PutString(UART_BUF);
    }
    else
    {
        /* Operation Failed - indicate this 
        LCD_PrintString("Failed to get");
        LCD_Position(1u, 0u);
        LCD_PrintString("SD card name");
        */
        UART_LOG_PutString("Failed to get SD card name\n");
    }
    
    /* Need some delay to indicate output on the LCD */
    //CyDelay(2000u);
    
    NextStep(step++, "SD CARD FORMAT");
        
    /*
    LCD_Position(0u, 0u);
    LCD_PrintString("SD card format");
    */
    UART_LOG_PutString("SD card format\n");
    
    if(0 == FS_FormatSD(sdVolName))
    {
        //LCD_Position(1u, 0u);
        //LCD_PrintString("Succeeded");
        UART_LOG_PutString("Succeeded\n");
    }
    else
    {
        //LCD_Position(1u, 0u);
        //LCD_PrintString("Failed");
        UART_LOG_PutString("Failed\n");
    }
    
    /* Need some delay to indicate output on the LCD */
    //CyDelay(2000u);
    
    NextStep(step++, "DIRECTORY");
    
    //LCD_Position(0u, 0u);
    
    /* This will create directory "Dir" on SD card */
    if(0 == FS_MkDir("Dir"))
    {
        /* Display successful directory creation message */
        //LCD_PrintString("\"Dir\" created");
        UART_LOG_PutString("\"Dir\" created");

    }
    else
    {
        /* Display failure message 
        LCD_PrintString("Failed to create");
        LCD_Position(1u, 0u);
        LCD_PrintString("directory");
        */
        
       UART_LOG_PutString("Failed to create directory\n");
    }
    
    /* Need some delay to indicate output on the LCD */
    //CyDelay(2000u);
    
    NextStep(step++, "FILE");
    
    /* Create specific file for modification */
    pFile = FS_FOpen(sdFile, "w");
    
    //LCD_Position(0u, 0u);
    
    /* Check if file was created */
    if(pFile)
    {
        /* Indicate successful file creation message 
        LCD_PrintString("File ");
        LCD_PrintString(sdFile);
        LCD_Position(1u, 0u);
        LCD_PrintString("was opened");
        */
        sprintf(UART_BUF,"File %s was opened\n", sdFile);
        UART_LOG_PutString(UART_BUF);
        
        /* Need some delay to indicate output on the LCD */
        //CyDelay(2000u);
        
        //LCD_ClearDisplay();
        
        //LCD_Position(0u, 0u);
        
        if(0 != FS_Write(pFile, "0123456789", 10u)) 
        {
            /* Inditate that data was written to a file
            LCD_PrintString("\"0123456789\"");
            LCD_PrintString(sdFile);
            LCD_Position(1u, 0u);
            LCD_PrintString("written to file");
            */
            
            sprintf(UART_BUF,"\"0123456789\" %s written to file \n", sdFile);
            UART_LOG_PutString(UART_BUF);
            
        }
        else
        {
            /*
            LCD_PrintString("Failed to write");
            LCD_Position(1u, 0u);
            LCD_PrintString("data to file");
            */
            UART_LOG_PutString("Failed to write data to file\n");
        }
        
        /* Need some delay to indicate output on the LCD */
        //CyDelay(2000u);
        
        //LCD_ClearDisplay();
        
        //LCD_Position(0u, 0u);
        
        if(0 == FS_FClose(pFile))
        {
            //LCD_PrintString("File was closed");
            UART_LOG_PutString("File was closed");
        }
        else
        {
            /*
            LCD_PrintString("Failed to close");
            LCD_Position(1u, 0u);
            LCD_PrintString("file");
            */
            
            UART_LOG_PutString("Failed to close file");
        }
    }
    else
    {
        /*
        LCD_PrintString("Failed to create");
        LCD_Position(1u, 0u);
        LCD_PrintString("file");
        */
        
        UART_LOG_PutString("Failed to create file");
    }
    
    /* Need some delay to indicate output on the LCD */
    //CyDelay(2000u);
        
    //LCD_ClearDisplay();
    
    NextStep(step++, "COPY");
    
    //LCD_Position(0u, 0u);
    
    /* Copy a file to a previously created directory */
    if(0 == FS_CopyFile(sdFile, "\\Dir\\file1.txt"))
    {
        /*LCD_PrintString("File was ");
        LCD_Position(1u, 0u);
        LCD_PrintString("copied to \"DIR\"");
        */
        
        UART_LOG_PutString("File was copied to \"DIR\" \n");
    }
    else
    {
        /*
        LCD_PrintString("Failed to copy");
        LCD_Position(1u, 0u);
        LCD_PrintString("file");
        */
        
        UART_LOG_PutString("Failed to copy file \n");
    }
    
    /* Need some delay to indicate output on the LCD */
    //CyDelay(2000);
    
    //LCD_ClearDisplay();
    /*
    LCD_Position(0u, 0u);
    LCD_PrintString("Example project");
    LCD_Position(1u, 0u);
    LCD_PrintString("completed");
    */
    
    
    UART_LOG_PutString("Example project completed \n");
    
    for(;;)
    {

    }
}


/* [] END OF FILE */



/* [] END OF FILE */
