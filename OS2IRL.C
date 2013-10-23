#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL
#define INCL_DOSFILEMGR

#define INCL_WINWINDOWMGR
#define INCL_WINMESSAGEMGR
#define INCL_WINFRAMEMGR
#define INCL_WINDIALOGS
#define INCL_WININPUT
#define INCL_WINSWITCHLIST
#define INCL_WINPROGRAMLIST
#define INCL_GPICONTROL
#define INCL_GPIPRIMITIVES
#define INCL_GPI
#define INCL_GPILCIDS
#define INCL_DEV
#define INCL_DOS
#define INCL_WIN
#define INCL_VIO
#define INCL_AVIO
#define MODEM_PORT "COM2"
#define TERMINAL_PORT "COM2"
#define COMPORT_1 "COM2"
#define COMPORT_2 "COM2"
#define COMPORT_3 "COM2"
#define COMPORT_4 "COM2"

#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <OS2.h>
#include <time.h>
#include <ctype.h>
#include <conio.h>
#include "os2irl.h"
CHAR *PrgStack[500];
ULONG PrgStackPtr;
ULONG NumReg[10];
CHAR  StrReg[10][256];
short SingleStep;
char cStrReg[55];


int main(int argc, char *argv[])
{
   HFILE IRLFile;
   HFILE ScrFile;
   ULONG action;
   ULONG BytesRead;
   ULONG ulFileSize;
   ULONG ulFileNew;
   CHAR *FileData;
   CHAR *ScriptData;
   CHAR *ScriptPtr;
   APIRET rc;
   ULONG LineLabels;
   CHAR *LinePos;
   CHAR Label[30];
   CHAR *LabelPtr;
   LABELSTRUCT *LabelStr;
   CHAR Instr[200];
   CHAR *InstrPtr;
   SHORT TheEnd;
#ifdef SPECIAL
   char    request[81];
   char    response[256];
   UCHAR   FileName[40];  /* Pipe name              */
   HPIPE   PipeHandle;    /* Pipe handle (returned) */
   ULONG   OpenMode;      /* Open-mode parameters   */
   ULONG   PipeMode;      /* Pipe-mode parameters   */
   ULONG   OutBufSize;    /* Size of the out-buffer */
   ULONG   InBufSize;     /* Size of the in-buffer  */
   ULONG   TimeOut;       /* Default value for DosWaitNPipe time-out parameter */
   ULONG   cbWrite;       /* Number of bytes to write to the pipe */
   ULONG   cbRead;        /* Number of bytes to read from the pipe */
   ULONG   cbActual;      /* Number of bytes actually read/written the pipe */

#endif
   if (argc == 1)
   {
      printf("Usage - OS2IRL .IRL NAME {\\PIPE\\ NAME} {SCRIP NAME}\r\n");
      printf("              { } optional");
      return(0);
   }

#ifdef SPECIAL
   /***************************************************************************/
   /* Create the named pipe                                                   */
   /***************************************************************************/
   if (argc > 2)
   {
      strcpy(FileName,"\\PIPE\\");
      strcat(FileName,argv[2]);
   }
   else
      strcpy(FileName,"\\PIPE\\Janus");

   OpenMode = NP_ACCESS_DUPLEX;            /* Full duplex, no inheritance,     */
                                           /* no write-through                 */

   PipeMode = NP_WAIT | NP_WMESG | NP_RMESG | 0x01;  /* Block on read and write, message */
                                           /* stream, instance count of 1      */

   OutBufSize = 4096;   /* The outgoing buffer must be 4KB in size             */

   InBufSize = 2048;    /* The incoming buffer must be 2KB in size             */

   TimeOut = 10000;     /* Time-out is 10 seconds (units are in milliseconds)  */

   rc = DosCreateNPipe( FileName, &PipeHandle, OpenMode,
                        PipeMode, OutBufSize, InBufSize,
                        TimeOut );

   if ( rc != 0 )
   {
     printf( "DosCreateNPipe error: return code = %ld", rc );
     exit( 1 );
   }

   /****************************************************************/
   /* DosConnectNPipe prepares a named pipe for a client process.  */
   /****************************************************************/

   puts( "Waiting for a client to connect to \\PIPE\\Janus..." );
   rc = DosConnectNPipe( PipeHandle );
   if ( rc != 0 )
   {
     printf( "DosConnectNPipe error: return code = %ld", rc );
     exit( 1 );
   }
   puts( "Connection established..." );
#endif

   PrgStackPtr = 0;
   SingleStep = FALSE;
   if (argc > 1)
   {
      printf("%s\r\n",argv[1]);
      rc = DosOpen(argv[1],&IRLFile,&action,0L,FILE_READONLY,
        OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,0L);
      if (rc == 0)
      {

         DosSetFilePtr(IRLFile,0,FILE_END,&ulFileSize);
         rc = DosAllocMem((VOID *)&FileData,ulFileSize,PAG_READ | PAG_WRITE | PAG_COMMIT);
         if (rc == 0)
         {
            DosSetFilePtr(IRLFile,0,FILE_BEGIN,&ulFileNew);
            DosRead(IRLFile,FileData,ulFileSize,&BytesRead);
            LineLabels = 0;
            LinePos = FileData;
            printf("Compiling - First Pass \r\n");
            while(LinePos < FileData + ulFileSize)
            {
               if (*LinePos == '.')
               {
                  LineLabels++;
                  LabelPtr = Label;
                  while(*LinePos != '\r' && *LinePos != ' ' && *LinePos != '\n')
                     if (LabelPtr < Label + 29)
                        *(LabelPtr++) = *(LinePos++);
                     else
                        LinePos++;
                  *LabelPtr = 0x00;
//                  printf("%s\n",Label);
               }
               while(*LinePos != '\n' && LinePos < FileData + ulFileSize)
                  LinePos++;
               if (LinePos < FileData + ulFileSize)
                  LinePos++;
            }
            printf("%d line lables\r\n",LineLabels);
            rc = DosAllocMem((VOID *)&LabelStr,LineLabels * sizeof(LABELSTRUCT),PAG_READ | PAG_WRITE | PAG_COMMIT);
            printf("Compiling - Second Pass \r\n");
            if (rc == 0)
            {

               LineLabels = 0;
               LinePos = FileData;
               while(LinePos < FileData + ulFileSize)
               {
                  if (*LinePos == '.')
                  {
                     LabelPtr = (LabelStr + LineLabels)->LabelName;
                     while(*LinePos != '\r' && *LinePos != ' ')
                        if (LabelPtr < (LabelStr + LineLabels)->LabelName + 29)
                           *(LabelPtr++) = *(LinePos++);
                        else
                           LinePos++;
                     *LabelPtr = 0x00;
//                     printf("%s\n",(LabelStr + LineLabels)->LabelName);
                     while(*LinePos != '\n')
                        LinePos++;
                     (LabelStr + LineLabels)->StartAddr = LinePos + 1;
                     LineLabels++;
                  }
                  while(*LinePos != '\n' && LinePos < FileData + ulFileSize)
                     LinePos++;
                  if (LinePos < FileData + ulFileSize)
                     LinePos++;
               }



               if (argc > 3)
               {
                  printf("Loading script %s\r\n",argv[3]);
                  rc = DosOpen(argv[3],&ScrFile,&action,0L,FILE_READONLY,
                     OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,0L);
                  if (rc == 0)
                  {

                     DosSetFilePtr(ScrFile,0,FILE_END,&ulFileSize);
                     rc = DosAllocMem((VOID *)&ScriptData,ulFileSize,PAG_READ | PAG_WRITE | PAG_COMMIT);
                     if (rc == 0)
                     {
                        DosSetFilePtr(ScrFile,0,FILE_BEGIN,&ulFileNew);
                        DosRead(ScrFile,ScriptData,ulFileSize,&BytesRead);
                     }
                     DosClose(ScrFile);
                     ScriptPtr = ScriptData;
                  }
               }
               else
                  ScriptData = NULL;

               LinePos = FileData;
               TheEnd = FALSE;

               while (!TheEnd)
               {

                  do
                  {
                     InstrPtr = Instr;
                     while ((*LinePos != '\n') && (LinePos < FileData + ulFileSize))
                        *(InstrPtr++) = *(LinePos++);
                     *(InstrPtr++) = *(LinePos++);
                     *InstrPtr = 0x00;
//                     if (Instr[0] == ':' || Instr[0] == ' ' || Instr[0] == '\r' || Instr[0] == '\n')
//                        printf("%s",Instr);
                     if (Instr[0] == ':')
                     {
                        if (strncmp(Instr,":STEP",5) == 0)
                        {
                           SHORT x;

                           switch(Instr[5])
                           {
                              case '+':
                                 SingleStep = TRUE;
                                 break;
                              case '-':
                                 SingleStep = FALSE;
                                 printf("\x1b[s");
                                 for (x=0;x<10;x++)
                                    printf("\x1b[%d;22H                                                        ",x+1);
                                 for (x=0;x<10;x++)
                                    printf("\x1b[%d;22H                                                        ",x+11);
                                 printf("\x1b[24;1H                                                                               ");
                                 printf("\x1b[u");
                                 fflush(NULL);
                                 break;
                           }
                        }
                     }
                  } while(Instr[0] == ':' || Instr[0] == ' ' || Instr[0] == '\r' || Instr[0] == '\n');
                  if (SingleStep)
                  {
                     CHAR ch;
                     SHORT x;

                     printf("\x1b[s");
                     printf("\x1b[24;1H                                                                               ");
                     fflush(NULL);
                     if (strlen(Instr) > 80)
                        Instr[79] = 0x00;
                     printf("\x1b[24;1H%s",Instr);
                     fflush(NULL);
                     for (x=0;x<10;x++)
                     {
                        printf("\x1b[%d;22H                                                        ",x+1);
                        strncpy(cStrReg,StrReg[x],55);
                        cStrReg[54] = 0x00;
                        printf("\x1b[%d;22H\"%s\"",x+1,cStrReg);
                        fflush(NULL);

                     }
                     for (x=0;x<10;x++)
                     {
                        printf("\x1b[%d;22H                                                        ",x+11);
                        printf("\x1b[%d;22H%ld",x+11,NumReg[x]);
                        fflush(NULL);
                     }
                     printf("\x1b[u");
                     fflush(NULL);
                     do
                     {
                        ch = getch();

                     }  while (ch != 0x0D);
                  }

                  switch(Instr[0])
                  {
                     case 'B':
                     {
                        CHAR *ptr;

                        ptr = &Instr[1];
                        if (isdigit(*ptr))
                        {
                           while (*ptr == '0' || *ptr == '1')
                           {
                              switch(*ptr)
                              {
                                 case '0':
                                    DosBeep(600,100);
                                    break;
                                 case '1':
                                    DosBeep(1200,100);
                                    break;
                              }
                              ptr++;
                           }
                        }
                        break;
                     }
                     case 'C':
                     {
                        CHAR *Num;
                        CHAR DCpy1[256];
                        CHAR *DCpy1Ptr;
                        CHAR DCpy2[256];
                        CHAR *DCpy2Ptr;
                        CHAR *Val1;
                        SHORT Val1Type;
                        CHAR *Val2;
                        SHORT Val2Type;

                        Num = &Instr[1];
                        DCpy1Ptr = DCpy1;
                        while(*Num != '=' && *Num != 0x00)
                           *(DCpy1Ptr++) = *(Num++);

                        *DCpy1Ptr = 0x00;

                        DeleteSpace(DCpy1);

                        if (*Num != '=')
                        {

                           printf("Syntax error Define - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        Num++;
                        while(*Num == ' ' && *Num != 0x00)
                           Num++;

                        if (*Num == 0x00)
                        {
                           printf("Syntax error Define - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        DCpy2Ptr = DCpy2;
                        while(*Num != 0x00 && *Num != '\r' && *Num != '\n' && *Num != ':')
                           *(DCpy2Ptr++) = *(Num++);
                        *DCpy2Ptr = 0x00;

                        DeleteSpace(DCpy2);

                        Val1Type = FindType(DCpy1);
                        Val2Type = FindType(DCpy2);

                        if (Val1Type == Val2Type)
                        {
                           printf("Invalid Convert - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        switch(Val1Type)
                        {
                           case 1:
                           {
                              LONG *Num1;
                              LONG *Num2;
                              LONG NumHold1;
                              LONG NumHold2;
                              CHAR *Buf2;
                              CHAR *Non;

                              NumReg[0] = 0;
                              rc = DosAllocMem((VOID *)&Buf2,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                              if (rc == 0)
                              {

                                 Num1 = GetAddrNum(DCpy1,&NumHold1);
                                 Val2 = GetAddr(DCpy2,Buf2,2);
                                 if (Num1 == NULL || Val2 == NULL)
                                 {
                                    printf("\r\n\r\nInvalid convert variable  - %s - Aborting\r\n",Instr);
                                    fflush(NULL);
                                    getch();
                                    return(0);
                                 }
                                 strcpy(Buf2,Val2);
                                 if (strlen(Buf2) > 7)
                                 {
                                    NumReg[0] = NumReg[0] | 1;
                                    Buf2[7] = 0x00;
                                 }
                                 Non = Buf2;
                                 while (*Non != 0x00)
                                 {
                                    if (!isdigit(*Non))
                                    {
                                       *Non = ' ';
                                       NumReg[0] = NumReg[0] | 2;
                                    }
                                    Non++;
                                 }
                                 *Num1 = atol(Buf2);
                                 DosFreeMem(Buf2);
                              }
                              else
                              {
                                 printf("Dos Alloc Memory Failure. RC = %d",rc);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              break;
                           }
                           case 2:
                           {
                              LONG *Num1;
                              LONG *Num2;
                              LONG NumHold1;
                              LONG NumHold2;
                              CHAR *Buf2;

                              NumReg[0] = 0;
                              rc = DosAllocMem((VOID *)&Buf2,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                              if (rc == 0)
                              {

                                 Num1 = GetAddrNum(DCpy2,&NumHold1);
                                 Val2 = GetAddr(DCpy1,Buf2,2);
                                 if (Num1 == NULL || Val2 == NULL)
                                 {
                                    printf("\r\n\r\nInvalid convert variable  - %s - Aborting\r\n",Instr);
                                    fflush(NULL);
                                    getch();
                                    return(0);
                                 }
                                 ltoa(*Num1,Val2,10);
                                 DosFreeMem(Buf2);
                              }
                              else
                              {
                                 printf("Dos Alloc Memory Failure. RC = %d",rc);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              break;
                           }
                        }
                        break;
                     }
                     case 'D':
                     {

                        CHAR *Num;
                        CHAR *Val1;
                        SHORT Val1Type;
                        CHAR *Val2;
                        SHORT Val2Type;
                        CHAR DCpy1[256];
                        CHAR *DCpy1Ptr;
                        CHAR DCpy2[256];
                        CHAR *DCpy2Ptr;

                        Num = &Instr[1];
                        DCpy1Ptr = DCpy1;
                        while(*Num != '=' && *Num != 0x00)
                           *(DCpy1Ptr++) = *(Num++);

                        *DCpy1Ptr = 0x00;

                        DeleteSpace(DCpy1);

                        if (*Num != '=')
                        {
                           printf("Syntax error Define - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        Num++;
                        while(*Num == ' ' && *Num != 0x00)
                           Num++;

                        if (*Num == 0x00)
                        {
                           printf("Syntax error Define - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        DCpy2Ptr = DCpy2;
                        while(*Num != 0x00 && *Num != '\r' && *Num != '\n' && *Num != ':')
                           *(DCpy2Ptr++) = *(Num++);
                        *DCpy2Ptr = 0x00;

                        DeleteSpace(DCpy2);

                        Val1Type = FindType(DCpy1);
                        Val2Type = FindType(DCpy2);

                        if (Val1Type != Val2Type)
                        {
                           printf("Invalid Define Move/Copy - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        switch(Val1Type)
                        {
                           case 1:
                           {
                              LONG *Num1;
                              LONG *Num2;
                              LONG NumHold1;
                              LONG NumHold2;

                              Num1 = GetAddrNum(DCpy1,&NumHold1);
                              Num2 = GetAddrNum(DCpy2,&NumHold2);
                              if (Num1 == NULL || Num2 == NULL)
                              {
                                 printf("\r\n\r\nInvalid Define - %s - Aborting\r\n",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              *Num1 = *Num2;
                              break;
                           }
                           case 2:
                           {
                              CHAR *Buf1;
                              CHAR *Buf2;

                              rc = DosAllocMem((VOID *)&Buf1,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                              if (rc == 0)
                              {

                                 rc = DosAllocMem((VOID *)&Buf2,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                                 if (rc == 0)
                                 {

                                    Val1 = GetAddr(DCpy1,Buf1,1);
                                    Val2 = GetAddr(DCpy2,Buf2,2);
                                    if (Val1 == NULL || Val2 == NULL)
                                    {
                                       printf("\r\n\r\nInvalid Define - %s - Aborting\r\n",Instr);
                                       fflush(NULL);
                                       getch();
                                       return(0);
                                    }
                                    strcpy(Val1,Val2);
                                    DosFreeMem(Buf2);
                                 }
                                 else
                                 {
                                    printf("Dos alloc memory failure rc = %d",rc);
                                    fflush(NULL);
                                    getch();
                                    return(0);
                                 }
                                 DosFreeMem(Buf1);
                              }
                              else
                              {
                                 printf("Dos alloc memory failure rc = %d",rc);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              break;
                           }
                        }

                     }
                     break;

                     case 'H':
                        switch (Instr[1])
                        {
                           case '#':
                           {
                              CHAR *ptr;
                              CHAR Exp1[256];
                              CHAR Exp2[256];
                              CHAR *Eptr;
                              CHAR Cond[10];
                              LONG *Num1;
                              LONG NumHold1;
                              SHORT RecNum;

                              ptr = &Instr[1];

                              while(*ptr == ' ')
                                 ptr++;

                              Eptr = Exp1;
                              while(!Condition(*ptr))
                                 *(Eptr++) = *(ptr++);
                              *Eptr = 0x00;

                              Eptr = Cond;
                              while(Condition(*ptr))
                                 *(Eptr++) = *(ptr++);
                              *Eptr = 0x00;
                              DeleteSpace(Cond);

                              while(*ptr == ' ')
                                 ptr++;

                              Eptr = Exp2;
                              while(*ptr != '.' && *ptr != 0x00 && *ptr != ' ' && *ptr != '\r' & *ptr != '\n')
                                 *(Eptr++) = *(ptr++);
                              *Eptr = 0x00;

                              if (FindType(Exp1) != 1)
                              {
                                 printf("Invalid register in Find record location - %s - Aborting",Exp1);
                                 fflush(NULL);
                                 return(0);
                              }
                              Num1 = GetAddrNum(Exp1,&NumHold1);
                              if (!isalpha(Exp2[0]))
                              {
                                 printf("File position Syntax error - %s - Aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              RecNum = Exp2[0] - 'A';
                              *Num1 = RecStr[RecNum].RecPos;

                              break;
                           }
                           default:
                           {

                              SHORT RecNum;
                              CHAR *Num;
                              CHAR StrHold[20];
                              CHAR *StrNum;

                              Num = &Instr[1];
                              while(*Num == ' ')
                                 Num++;
                              if (!isalpha(*Num))
                              {
                                 printf("File position Syntax error - %s - Aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              RecNum = *Num - 'A';
                              Num++;

                              while (*Num != '=' && *Num != 0x00)
                                 Num++;
                              if (*Num != '=')
                              {
                                 printf("Syntax Error %s - aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              Num++;

                              StrNum = StrHold;
                              while (*Num != ' ' && *Num != '\r' && *Num != '\n' && *Num != 0x00)
                                 *(StrNum++) = *(Num++);
                              *StrNum = 0x00;
                              RecStr[RecNum].RecPos = atol(StrHold);
                              if (RecStr[RecNum].RecPos < RecStr[RecNum].NumRec)
                              {
                                 ULONG x;

                                 for(x = RecStr[RecNum].RecPos;x < RecStr[RecNum].NumRec;x++)
                                    strcpy(&RecStr[RecNum].RecData[x * (RecStr[RecNum].RecLen + 2)],"");

                              }

                              break;
                           }
                        }
                        break;
                     case 'I':
                        printf("I Append command not implemented yet - Aborting");
                        fflush(NULL);
                        return(0);
                        break;
                     case 'L':
                        printf("Lookup Record command not implemented yet - Aborting");
                        fflush(NULL);
                        return(0);
                        break;
                     case 'O':
                        switch (Instr[2])
                        {
                           CHAR StrHold[20];
                           CHAR *Num;
                           CHAR *StrNum;
                           SHORT RecNum;

                           case '(':
                              RecNum = Instr[1] - 'A';
                              strcpy(RecStr[RecNum].FileName,"");
                              RecStr[RecNum].fHandle = 0;
                              Num = &Instr[3];
                              StrNum = StrHold;
                              while(*Num != ',' && *Num != 0x00)
                                 *(StrNum++) = *(Num++);
                              *StrNum = 0x00;
                              if (*Num != ',')
                              {
                                 printf("File Create Error %s - aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              RecStr[RecNum].NumRec = atoi(StrHold);
                              Num++;

                              StrNum = StrHold;
                              while(*Num != ')' && *Num != 0x00)
                                 *(StrNum++) = *(Num++);
                              *StrNum = 0x00;
                              if (*Num != ')')
                              {
                                 printf("File Create Error %s - aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              RecStr[RecNum].RecLen = atoi(StrHold);
                              RecStr[RecNum].MaxBytes = (RecStr[RecNum].RecLen + 2) * RecStr[RecNum].NumRec;
                              rc = DosAllocMem((VOID *)&RecStr[RecNum].RecData, RecStr[RecNum].MaxBytes,
                                 PAG_READ | PAG_WRITE | PAG_COMMIT);
                              break;
                           case '"':
                              RecNum = Instr[1] - 'A';
                              Num = &Instr[3];
                              StrNum = RecStr[RecNum].FileName;
                              while(*Num != '"' && *Num != 0x00)
                                 *(StrNum++) = *(Num++);
                              *StrNum = 0x00;
                              if (*Num != '"')
                              {
                                 printf("File Create Error %s - aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              Num++;
                              Num++;

                              StrNum = StrHold;
                              while(*Num != ',' && *Num != 0x00)
                                 *(StrNum++) = *(Num++);
                              *StrNum = 0x00;
                              if (*Num != ',')
                              {
                                 printf("File Create Error %s - aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              RecStr[RecNum].NumRec = atoi(StrHold);
                              Num++;

                              StrNum = StrHold;
                              while(*Num != ')' && *Num != 0x00)
                                 *(StrNum++) = *(Num++);
                              *StrNum = 0x00;
                              if (*Num != ')')
                              {
                                 printf("File Create Error %s - aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              RecStr[RecNum].RecLen = atoi(StrHold);
                              RecStr[RecNum].MaxBytes = (RecStr[RecNum].RecLen + 2) * RecStr[RecNum].NumRec;
                              rc = DosAllocMem((VOID *)&RecStr[RecNum].RecData, RecStr[RecNum].MaxBytes,
                                 PAG_READ | PAG_WRITE | PAG_COMMIT);
                              rc = DosOpen(RecStr[RecNum].FileName,&RecStr[RecNum].fHandle,
                                 &RecStr[RecNum].fHAction,0L,FILE_NORMAL,
                                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,0L);
                              break;
                           default:
                              break;
                        }
                        break;
                     case 'P':
                     {
                        CHAR *Num;
                        CHAR PCpy1[256];
                        CHAR *PCpy1Ptr;
                        CHAR *Val1;
                        SHORT Val1Type;

                        Num = &Instr[1];
                        PCpy1Ptr = PCpy1;
                        while(*Num != '\r' && *Num != '\n' && *Num != 0x00 && *Num != ':')
                           *(PCpy1Ptr++) = *(Num++);
                        *PCpy1Ptr = 0x00;

                        DeleteSpace(PCpy1);
                        Val1Type = FindType(PCpy1);

                        switch(Val1Type)
                        {
                           case 1:
                           {
                              LONG *Num1;
                              LONG NumHold1;

                              Num1 = GetAddrNum(PCpy1,&NumHold1);
                              printf("%ld",*Num1);
                              break;
                           }
                           case 2:
                           {
                              CHAR *Buf1;

                              rc = DosAllocMem((VOID *)&Buf1,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                              if (rc == 0)
                              {

                                 Val1 = GetAddr(PCpy1,Buf1,2);
                                 FixControl(Val1);
                                 printf("%s",Val1);
                                 DosFreeMem(Buf1);
                              }
                              else
                              {
                                 printf("Dos Alloc Memory failure. rc = %d",rc);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }
                              break;
                           }
                        }
                        fflush(NULL);
                        break;
                     }
                     case 'Q':
                     {
                        CHAR Exp1[256];
                        CHAR Exp2[256];
                        CHAR *ptr;
                        CHAR *Eptr;
                        CHAR Cond[10];
                        CHAR *Val1;
                        CHAR *Val2;
                        SHORT Val1Type;
                        SHORT Val2Type;
                        SHORT Branch;

                        ptr = &Instr[1];

                        while(*ptr == ' ')
                           ptr++;
                        if (*ptr == '\r' || *ptr == '\n' || *ptr == 0x00 && *ptr == ':')
                        {
                           /* Unconditional return */
                           Branch = TRUE;

                        }
                        else
                        {

                           /* Conditional return */

                           Eptr = Exp1;
                           while(!Condition(*ptr))
                              *(Eptr++) = *(ptr++);
                           *Eptr = 0x00;
                           while(*ptr == ' ')
                              ptr++;

                           Eptr = Cond;
                           while(Condition(*ptr))
                              *(Eptr++) = *(ptr++);
                           *Eptr = 0x00;

                           DeleteSpace(Cond);

                           while(*ptr == ' ')
                              ptr++;

                           Eptr = Exp2;
                           while(*ptr != '.' && *ptr != 0x00)
                              *(Eptr++) = *(ptr++);
                           *Eptr = 0x00;

                           Val1Type = FindType(Exp1);
                           Val2Type = FindType(Exp2);
                           if (Val1Type != Val2Type)
                           {
                              printf("Invalid Condition Compare in return from subroutine - %s - Aborting",Instr);
                              fflush(NULL);
                              getch();
                              return(0);
                           }

                           Branch = FALSE;
                           switch(Val1Type)
                           {
                              case 1:
                              {
                                 LONG *Num1;
                                 LONG *Num2;
                                 LONG NumHold1;
                                 LONG NumHold2;

                                 Num1 = GetAddrNum(Exp1,&NumHold1);
                                 Num2 = GetAddrNum(Exp2,&NumHold2);
                                 switch(FindCondition(Cond))
                                 {
                                    case 0:
                                       printf("Invalid Branch Condition - %s - Aborting",Cond);
                                       fflush(NULL);
                                       getch();
                                       return(0);
                                       break;
                                    case 1:
                                       Branch = (*Num1 != *Num2);
                                       break;
                                    case 2:
                                       Branch = (*Num1 >= *Num2);
                                       break;
                                    case 3:
                                       Branch = (*Num1 == *Num2);
                                       break;
                                    case 4:
                                       Branch = (*Num1 > *Num2);
                                       break;
                                    case 5:
                                       Branch = (*Num1 < *Num2);
                                       break;
                                    case 6:
                                       Branch = (*Num1 <= *Num2);
                                       break;
                                    default:
                                       printf("Invalid Branch Condition - %s - Aborting",Cond);
                                       fflush(NULL);
                                       getch();
                                       return(0);
                                       break;
                                 }
                                 break;
                              }
                              case 2:
                              {
                                 CHAR *Buf1;
                                 CHAR *Buf2;
                                 SHORT CDig;
                                 CHAR *CDigPtr;

                                 rc = DosAllocMem((VOID *)&Buf1,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                                 if (rc == 0)
                                 {

                                    rc = DosAllocMem((VOID *)&Buf2,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                                    if (rc == 0)
                                    {

                                       Val1 = GetAddr(Exp1,Buf1,1);
                                       Val2 = GetAddr(Exp2,Buf2,2);
                                       CDigPtr = Val2;
                                       CDig = FALSE;
                                       while (*CDigPtr != 0x00)
                                       {

                                          if (*CDigPtr == '#' || *CDigPtr == '?' || *CDigPtr == '@')
                                             CDig = TRUE;
                                          CDigPtr++;
                                       }
                                       switch(FindCondition(Cond))
                                       {
                                          case 0:
                                             printf("Invalid Branch Condition - %s - Aborting",Cond);
                                             fflush(NULL);
                                             getch();
                                             return(0);
                                             break;
                                          case 1:
                                             Branch = strcmp(Val1,Val2);
                                             break;
                                          case 2:
                                             Branch = !strcmp(Val1,Val2);
                                             if (!Branch)
                                                if (strcmp(Val1,Val2) > 0)
                                                   Branch = TRUE;
                                             break;
                                          case 3:
                                             if (CDig)
                                             {
                                                CHAR *DtPtr;

                                                CDigPtr = Val2;
                                                DtPtr = Val1;
                                                Branch = TRUE;
                                                while (Branch == TRUE && *CDigPtr != 0x00)
                                                {

                                                   switch (*CDigPtr)
                                                   {
                                                      case '#':
                                                         if (!isdigit(*(DtPtr++)))
                                                            Branch = FALSE;
                                                         break;
                                                      case '@':
      /*                                                      if (!isalpha(*(DtPtr++))) */
                                                            Branch = FALSE;
                                                         break;
                                                      case '?':
                                                         if (!isdigit(*DtPtr) || !isalpha(*DtPtr))
                                                            Branch = FALSE;
                                                         DtPtr++;
                                                         break;
                                                      default:
                                                         if (!isdigit(*DtPtr) && !isalpha(*DtPtr))
                                                         {
                                                            printf("Invalid branch command - %s - Aborting\r\n",Instr);
                                                            fflush(NULL);
                                                            getch();
                                                            return(0);
                                                         }
                                                         if (*CDigPtr != *(DtPtr++))
                                                            Branch = FALSE;
                                                         break;
                                                   }
                                                   CDigPtr++;
                                                }

                                             }
                                             else
                                                if (Val1 != NULL && Val2 != NULL)
                                                   Branch = !strcmp(Val1,Val2);
                                                else
                                                {
                                                   printf("Invalid command - %s - Aborting\r\n",Instr);
                                                   fflush(NULL);
                                                   getch();
                                                   return(0);
                                                }
                                             break;
                                          case 4:
                                             Branch = (strcmp(Val1,Val2) > 0);
                                             break;
                                          case 5:
                                             Branch = (strcmp(Val1,Val2) < 0);
                                             break;
                                       }
                                       DosFreeMem(Buf2);
                                    }
                                    else
                                    {
                                       printf("Dos Alloc Memory Failure. RC = %d",rc);
                                       fflush(NULL);
                                       getch();
                                       return(0);
                                    }
                                    DosFreeMem(Buf1);
                                 }
                                 else
                                 {
                                    printf("Dos Alloc Memory Failure. RC = %d",rc);
                                    fflush(NULL);
                                    getch();
                                    return(0);
                                 }
                                 break;
                              }

                           }
                        }
                        if (Branch)
                        {

                           if (PrgStackPtr > 0)
                              LinePos = PrgStack[--PrgStackPtr];
                           else
                           {
                              printf("Stack Space Error - aborting");
                              fflush(NULL);
                              getch();
                              return(0);
                           }
                        }
                        break;
                     }



                     case 'G':
                     case 'S':
                     {
                        CHAR *NewAddr;
                        CHAR *LineLabel;
                        CHAR PrgLabel[50];


                        switch(Instr[1])
                        {
                           case '.':
                           {
                              LineLabel = FindLineLabel(&Instr[1], PrgLabel);
                              if (LineLabel != NULL)
                              {

                                 NewAddr = GetSubAddr(PrgLabel, LabelStr, LineLabels);
                                 if (NewAddr != NULL)
                                 {
                                    if (Instr[0] == 'S')
                                    {
                                       PrgStack[PrgStackPtr++] = LinePos;
                                       if (PrgStackPtr > 499)
                                       {
                                          printf("Out of Stack Space - aborting");
                                          fflush(NULL);
                                          getch();
                                          return(0);
                                       }
                                    }
                                    LinePos = NewAddr;

                                 }
                                 else
                                 {
                                    SHORT x;

                                    x = 1;
                                    while(Instr[x] != 0x00)
                                    {
                                       if (Instr[x] == 0x0a || Instr[x] == 0x0d)
                                          Instr[x] = 0x00;
                                       else
                                          x++;
                                    }

                                    printf("\r\nInvalid Line Label %s not found - aborting",&Instr[1]);
                                    fflush(NULL);
                                    getch();
                                    return(0);
                                 }
                              }

                              break;
                           }
                           default:
                           {
                              CHAR Exp1[256];
                              CHAR Exp2[256];
                              CHAR *ptr;
                              CHAR *Eptr;
                              CHAR Cond[10];
                              CHAR *Val1;
                              CHAR *Val2;
                              SHORT Val1Type;
                              SHORT Val2Type;
                              SHORT Branch;

                              ptr = &Instr[1];

                              while(*ptr == ' ')
                                 ptr++;

                              Eptr = Exp1;
                              while(!Condition(*ptr))
                                 *(Eptr++) = *(ptr++);
                              *Eptr = 0x00;
                              while(*ptr == ' ')
                                 ptr++;

                              Eptr = Cond;
                              while(Condition(*ptr))
                                 *(Eptr++) = *(ptr++);
                              *Eptr = 0x00;

                              DeleteSpace(Cond);

                              while(*ptr == ' ')
                                 ptr++;

                              Eptr = Exp2;
                              while(*ptr != '.' && *ptr != 0x00)
                                 *(Eptr++) = *(ptr++);
                              *Eptr = 0x00;

                              Val1Type = FindType(Exp1);
                              Val2Type = FindType(Exp2);
                              if (Val1Type != Val2Type)
                              {
                                 printf("Invalid Condition Compare in subroutine - %s - Aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                              }

                              Branch = FALSE;
                              switch(Val1Type)
                              {
                                 case 1:
                                 {
                                    LONG *Num1;
                                    LONG *Num2;
                                    LONG NumHold1;
                                    LONG NumHold2;

                                    Num1 = GetAddrNum(Exp1,&NumHold1);
                                    Num2 = GetAddrNum(Exp2,&NumHold2);
                                    switch(FindCondition(Cond))
                                    {
                                       case 0:
                                          printf("Invalid Branch Condition - %s - Aborting",Cond);
                                          fflush(NULL);
                                          getch();
                                          return(0);
                                          break;
                                       case 1:
                                          Branch = (*Num1 != *Num2);
                                          break;
                                       case 2:
                                          Branch = (*Num1 >= *Num2);
                                          break;
                                       case 3:
                                          Branch = (*Num1 == *Num2);
                                          break;
                                       case 4:
                                          Branch = (*Num1 > *Num2);
                                          break;
                                       case 5:
                                          Branch = (*Num1 < *Num2);
                                          break;
                                       case 6:
                                          Branch = (*Num1 <= *Num2);
                                          break;
                                       default:
                                          printf("Invalid Branch Condition - %s - Aborting",Cond);
                                          fflush(NULL);
                                          getch();
                                          return(0);
                                          break;
                                    }
                                    break;
                                 }
                                 case 2:
                                 {
                                    CHAR *Buf1;
                                    CHAR *Buf2;
                                    SHORT CDig;
                                    CHAR *CDigPtr;

                                    rc = DosAllocMem((VOID *)&Buf1,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                                    if (rc == 0)
                                    {

                                       rc = DosAllocMem((VOID *)&Buf2,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                                       if (rc == 0)
                                       {

                                          Val1 = GetAddr(Exp1,Buf1,1);
                                          Val2 = GetAddr(Exp2,Buf2,2);
                                          CDigPtr = Val2;
                                          CDig = FALSE;
                                          while (*CDigPtr != 0x00)
                                          {

                                             if (*CDigPtr == '#' || *CDigPtr == '?' || *CDigPtr == '@')
                                                CDig = TRUE;
                                             CDigPtr++;
                                          }
                                          switch(FindCondition(Cond))
                                          {
                                             case 0:
                                                printf("Invalid Branch Condition - %s - Aborting",Cond);
                                                fflush(NULL);
                                                getch();
                                                return(0);
                                                break;
                                             case 1:
                                                Branch = strcmp(Val1,Val2);
                                                break;
                                             case 2:
                                                Branch = !strcmp(Val1,Val2);
                                                if (!Branch)
                                                   if (strcmp(Val1,Val2) > 0)
                                                      Branch = TRUE;
                                                break;
                                             case 3:
                                                if (CDig)
                                                {
                                                   CHAR *DtPtr;

                                                   CDigPtr = Val2;
                                                   DtPtr = Val1;
                                                   Branch = TRUE;
                                                   while (Branch == TRUE && *CDigPtr != 0x00)
                                                   {

                                                      switch (*CDigPtr)
                                                      {
                                                         case '#':
                                                            if (!isdigit(*(DtPtr++)))
                                                               Branch = FALSE;
                                                            break;
                                                         case '@':
      /*                                                      if (!isalpha(*(DtPtr++))) */
                                                               Branch = FALSE;
                                                            break;
                                                         case '?':
                                                            if (!isdigit(*DtPtr) || !isalpha(*DtPtr))
                                                               Branch = FALSE;
                                                            DtPtr++;
                                                            break;
                                                         default:
                                                            if (!isdigit(*DtPtr) && !isalpha(*DtPtr))
                                                            {
                                                               printf("Invalid branch command - %s - Aborting\r\n",Instr);
                                                               fflush(NULL);
                                                               getch();
                                                               return(0);
                                                            }
                                                            if (*CDigPtr != *(DtPtr++))
                                                               Branch = FALSE;
                                                            break;
                                                      }
                                                      CDigPtr++;
                                                   }

                                                }
                                                else
                                                   if (Val1 != NULL && Val2 != NULL)
                                                      Branch = !strcmp(Val1,Val2);
                                                   else
                                                   {
                                                      printf("Invalid command - %s - Aborting\r\n",Instr);
                                                      fflush(NULL);
                                                      getch();
                                                      return(0);
                                                   }
                                                break;
                                             case 4:
                                                Branch = (strcmp(Val1,Val2) > 0);
                                                break;
                                             case 5:
                                                Branch = (strcmp(Val1,Val2) < 0);
                                                break;
                                          }
                                          DosFreeMem(Buf2);
                                       }
                                       else
                                       {
                                          printf("Dos Alloc Memory Failure. RC = %d",rc);
                                          fflush(NULL);
                                          getch();
                                          return(0);
                                       }
                                       DosFreeMem(Buf1);
                                    }
                                    else
                                    {
                                       printf("Dos Alloc Memory Failure. RC = %d",rc);
                                       fflush(NULL);
                                       getch();
                                       return(0);
                                    }
                                    break;
                                 }

                              }

                              if (Branch)
                              {

                                 LineLabel = FindLineLabel(&Instr[1], PrgLabel);
                                 if (LineLabel != NULL)
                                 {

                                    NewAddr = GetSubAddr(PrgLabel, LabelStr, LineLabels);
                                    if (NewAddr != NULL)
                                    {
                                       if (Instr[0] == 'S')
                                       {
                                          PrgStack[PrgStackPtr++] = LinePos;
                                          if (PrgStackPtr > 499)
                                          {
                                             printf("Out of Stack Space - aborting");
                                             fflush(NULL);
                                             getch();
                                             return(0);
                                          }
                                       }
                                       LinePos = NewAddr;
                                    }
                                    else
                                    {
                                       printf("Invalid Subroutine label - %s - Aborting",Instr);
                                       fflush(NULL);
                                       getch();
                                       return(0);
                                    }
                                 }
                                 else
                                 {
                                    printf("Invalid Line Label - %s - Aborting\r\n",Instr);
                                    fflush(NULL);
                                    getch();
                                    return(0);
                                 }
                              }
                              break;
                           }
                        }
                        break;
                     }
                     case 'V':
                     {
                        SHORT FlagB;
                        SHORT FlagD;
                        SHORT FlagE;
                        SHORT FlagK;
                        SHORT FlagM;
                        SHORT FlagS;
                        SHORT FlagT;
                        SHORT FlagP;
                        SHORT FlagN;
                        CHAR *ptr;
                        CHAR ch;
                        SHORT Len;
                        CHAR ComPort[10];
                        HFILE comhandle;
                        ULONG comaction;
                        ULONG timer;
                        typedef struct
                        {
                           USHORT numchar;
                           USHORT QSize;
                        } RECQueue;
                        RECQueue RxQ;
                        ULONG PIO;
                        ULONG LIO;


                        ptr = &Instr[1];

                        FlagB = FALSE;
                        FlagD = FALSE;
                        FlagE = FALSE;
                        FlagK = FALSE;
                        FlagM = FALSE;
                        FlagS = FALSE;
                        FlagT = FALSE;
                        FlagP = FALSE;
                        FlagN = FALSE;
                        strcpy(ComPort,"");
                        while(*ptr != ' ' && *ptr != 0x0d && *ptr != 0x0a)
                        {
                           switch (*ptr)
                           {
                              case 'B':
                                 FlagB = TRUE;
                                 break;
                              case 'D':
                                 FlagD = TRUE;
                                 break;
                              case 'E':
                                 FlagE = TRUE;
                                 break;
                              case 'K':
                                 FlagK = TRUE;
                                 break;
                              case 'M':
                                 FlagM = TRUE;
                                 break;
                              case 'N':
                                 FlagN = TRUE;
                                 break;
                              case 'P':
                                 FlagP = TRUE;
                                 break;
                              case 'S':
                                 FlagS = TRUE;
                                 break;
                              case 'T':
                                 FlagT = TRUE;
                                 break;
#ifndef SPECIAL
                              case '1':
                                 strcpy(ComPort,COMPORT_1);
                                 break;
                              case '2':
                                 strcpy(ComPort,COMPORT_2);
                                 break;
                              case '3':
                                 strcpy(ComPort,COMPORT_3);
                                 break;
                              case '4':
                                 strcpy(ComPort,COMPORT_4);
                                 break;
#endif
                           }
                           ptr++;
                        }
                        if (strlen(ComPort) > 0)
                        {

                           initcomm(ComPort,9600L, 'N', 8L, 1L);
                           rc = DosOpen(ComPort,&comhandle,&comaction,0L,FILE_NORMAL,
                              OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                              OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,0L);
                           if (rc != 0)
                              strcpy(ComPort,"");
                        }
                        timer = 0;
                        do
                        {
                           ch = ' ';
                           if (strlen(ComPort) > 0)
                           {
                              CHAR Rxbuf[10];
                              ULONG numbytes;

                              PIO = 0;
                              LIO = sizeof(RxQ);
                              rc = DosDevIOCtl(comhandle,IOCTL_ASYNC,ASYNC_GETINQUECOUNT,
                                    0L,PIO,&PIO,&RxQ,LIO,&LIO);
                              if (RxQ.numchar > 0)
                              {
                                 timer = 0;
                                 while (RxQ.numchar > 0)
                                 {

                                    rc = DosRead(comhandle,Rxbuf,1,&numbytes);
                                    ch = Rxbuf[0];
                                    if (ch != 0x0d)
                                    {
                                       Len = strlen(StrReg[0]);
                                       StrReg[0][Len] = ch;
                                       StrReg[0][Len + 1] = 0x00;
                                    }
                                    PIO = 0;
                                    LIO = sizeof(RxQ);
                                    rc = DosDevIOCtl(comhandle,IOCTL_ASYNC,ASYNC_GETINQUECOUNT,
                                          0L,PIO,&PIO,&RxQ,LIO,&LIO);
                                 }
                              }
                           }

                           if (FlagK)
                           {

                              if (_kbhit() && ScriptData == NULL)
                              {

                                 timer = 0;
                                 while (_kbhit())
                                 {

                                    ch = getch();
                                    if (ch == 0x00)
                                    {
                                       ch = getch();
                                       switch (ch)
                                       {
                                          case 59:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F1");
                                             break;
                                          case 60:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F2");
                                             break;
                                          case 61:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F3");
                                             break;
                                          case 62:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F4");
                                             break;
                                          case 63:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F5");
                                             break;
                                          case 64:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F6");
                                             break;
                                          case 65:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F7");
                                             break;
                                          case 66:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F8");
                                             break;
                                          case 67:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F9");
                                             break;
                                          case 68:
                                             ch = 0x0d;
                                             strcat(StrReg[0],"F0");
                                             break;
                                          default:
                                             ch = 0x00;
                                       }
                                    }
                                    switch (ch)
                                    {
                                       case 0x0d:
                                          break;
                                       case 0x08:
                                          Len = strlen(StrReg[0]);
                                          if (Len > 0)
                                          {
                                             StrReg[0][Len - 1] = 0x00;
                                             if (FlagD)
                                             {
                                                printf("%c %c",ch,ch);
                                                fflush(NULL);
                                             }
                                          }
                                          break;
                                       default:
                                          Len = strlen(StrReg[0]);
                                          StrReg[0][Len] = ch;
                                          StrReg[0][Len + 1] = 0x00;
                                          if (FlagD)
                                          {
                                             printf("%c",ch);
                                             fflush(NULL);
                                          }
                                          break;
                                    }
                                 }
                              }
                              else
                              {
                                 CHAR ScrBuf[256];
                                 CHAR *ScrBufPtr;

                                 if (ScriptData != NULL)
                                 {

                                    ScrBufPtr = ScrBuf;
                                    while(*ScriptPtr != 0x0d && *ScriptPtr != ';' && *ScriptPtr != 0x00)
                                       *(ScrBufPtr++) = *(ScriptPtr++);
                                    *ScrBufPtr = 0x00;
                                    if (*ScriptPtr == ';')
                                    {
                                       while(*ScriptPtr != 0x0d)
                                          ScriptPtr++;
                                       while(*ScriptPtr == 0x0d || *ScriptPtr == 0x0a)
                                          ScriptPtr++;
                                    }
                                    if (*ScriptPtr == 0x0D || *ScriptPtr == 0x0A)
                                    {
                                       while(*ScriptPtr == 0x0d || *ScriptPtr == 0x0a)
                                          ScriptPtr++;
                                    }
                                    if (*ScriptPtr == 0x00 || *ScriptPtr == 0x1A)
                                    {
                                       DosFreeMem(ScriptData);
                                       ScriptData = 0x00;
                                    }
                                    DeleteSpace(ScrBuf);
                                    strcat(StrReg[0],ScrBuf);
                                    printf("%s",ScrBuf);
                                    fflush(NULL);
                                    ch = 0x0d;
                                 }
                              }
                           }
#ifdef SPECIAL
                           else
                           {

                              cbRead = 255;
                              rc = DosRead( PipeHandle, &response, cbRead, &cbActual);
                              if ( rc )
                              {
                                 printf( "DosRead error: return code = %ld", rc );
                                 fflush(NULL);
                                 getch();
                                 exit( 1 );
                              }

                              response[cbActual] = '\0';
                              strcat(StrReg[0],response);
                              ch = 0x0d;
                           }
#endif
                           DosSleep(100);
                           if (!FlagK)
                              timer += 100;
                        } while (ch != 0x0d && timer < 30000);

                        if (timer >= 30000)
                           strcpy(StrReg[0],"");
                        if (strlen(ComPort))
                           DosClose(comhandle);

                        if (FlagB)
                           DosBeep(1200,100);
                        break;
                     }
                     case 'W':
                     {
                        CHAR WaitNum[10];
                        CHAR *WaitPtr;
                        CHAR *ptr;

                        ptr = &Instr[1];

                        if (isdigit(*ptr))
                        {
                           WaitPtr = WaitNum;
                           while(isdigit(*ptr))
                              *(WaitPtr++) = *(ptr++);

                           *WaitPtr = 0x00;

                           DosSleep(atoi(WaitNum) * 1000);

                        }
                        else
                        {
                           printf("Invalid wait command - %s - Aborting",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        break;
                     }
                     case 'X':
                     {

                        CHAR TxPort[10];
                        SHORT Protocol;
                        CHAR *ptr;
                        CHAR ch;
                        SHORT Len;
                        CHAR Data[256];
                        CHAR *DataPtr;
                        LONG ValType;
                        CHAR *Val;
                        CHAR *Buf1;
                        HFILE comhandle;
                        ULONG comaction;

                        ptr = &Instr[1];
                        while(*ptr != ',' && *ptr != 0x00)
                        {
                           switch (*ptr)
                           {
                              case 'M':
                                 strcpy(TxPort,MODEM_PORT);
                                 break;
                              case 'T':
                                 strcpy(TxPort,TERMINAL_PORT);
                                 break;
                              case 'P':
                                 Protocol = TRUE;
                                 break;
                              case 'N':
                                 Protocol = FALSE;
                                 break;
                           }
                           ptr++;
                        }

                        if (*ptr != ',')
                        {
                           printf("Transmit Syntax Error - %s - Aborting\r\n",Instr);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        ptr++;

                        DataPtr = Data;
                        while(*ptr != '\r' && *ptr != '\n' && *ptr != 0x00 && *ptr != ':' && *ptr != ';')
                           *(DataPtr++) = *(ptr++);
                        *DataPtr = 0x00;

                        DeleteSpace(Data);
                        ValType = FindType(Data);
                        rc = DosAllocMem((VOID *)&Buf1,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
                        if (rc == 0)
                        {


                           switch(ValType)
                           {
                              case 1:
                                 printf("Invalid Transmit type - %s - Aborting",Instr);
                                 fflush(NULL);
                                 getch();
                                 return(0);
                                 break;

                              case 2:
                                 Val = GetAddr(Data,Buf1,1);
                                 break;
                           }
#ifdef SPECIAL
                           rc = 0;
#else

                           initcomm(TxPort,9600L, 'N', 8L, 1L);
                           rc = DosOpen(TxPort,&comhandle,&comaction,0L,FILE_NORMAL,
                              OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                              OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,0L);
#endif
                           if (rc == 0)
                           {
                              ULONG DataSize;
                              ULONG BytesRead;

                              DataSize = strlen(Val);
#ifdef SPECIAL
                              rc = DosWrite( PipeHandle, Val, DataSize, &BytesRead );
#else
                              DosWrite(comhandle,Val,DataSize,&BytesRead);
                              DosClose(comhandle);
#endif
                           }
                           else
                           {
                              printf("Error Communicating - Aborting");
                              fflush(NULL);
                              getch();
                              return(0);
                           }
                           DosFreeMem(Buf1);
                        }
                        else
                        {
                           printf("Dos Alloc Memory Failure. RC = %d",rc);
                           fflush(NULL);
                           getch();
                           return(0);
                        }
                        break;
                     }
                     case '.':
                     case ' ':
                     {

                        // printf("%s",Instr);
                        break;
                     }
                     case 0x1A:
                        return(0);
                        break;
                     default:
                        printf("Unknown Command - %s - Aborting",Instr);
                        fflush(NULL);
                        getch();
                        return(0);
                  }
               }

               DosFreeMem(LabelStr);
            }
            DosFreeMem(FileData);
         }

         DosClose(IRLFile);
      }

   }
   return(0);
}
SHORT Condition(CHAR exp)
{
   switch (exp)
   {
      case '=':
      case '+':
      case '-':
      case '>':
      case '<':
      case ' ':
         return(TRUE);
         break;
      default:
         return(FALSE);
         break;
   }
   return(FALSE);
}

LONG GetValue(PSZ Num)
{
   ULONG RecNum;
   switch(*Num)
   {
      case '#':
         if (!isdigit(*(Num +1)))
         {
            printf("Get Value Syntax Error");
            return(-1);
         }
         RecNum = NumReg[*(Num + 1) - '0'];
         return(RecNum);
         break;
      default:
      {
         CHAR *Start;
         LONG Val;

         Start = Num;
         while(*Num != 0x00)
         {
            if (!isdigit(*Num))
            {
               printf("Get Value syntax error - Aborting");
               return(-1);
            }
            Num++;
         }
         return(atol(Start));
         break;
      }
   }
   return(-1);
}

LONG MiddleCopy(PSZ Num, LONG *StartPos, LONG *CLen)
{
   CHAR Hold[10];
   CHAR *HoldPtr;

   Num++;
   HoldPtr = Hold;

   while(*Num == ' ')
      Num++;

   while(*Num != ',' && *Num != 0x00)
      *(HoldPtr++) = *(Num++);

   *HoldPtr = 0x00;
   if (*Num != ',')
   {
      printf("Middle Copy Syntax Error - Aborting");
      return(-1);
   }
   *CLen = GetValue(Hold);
   if (*CLen == -1)
      return(*CLen);

   Num++;
   HoldPtr = Hold;

   while(*Num == ' ')
      Num++;

   while(*Num != ' ' && *Num != 0x00 && *Num != '\r' && *Num != '\n')
      *(HoldPtr++) = *(Num++);

   *HoldPtr = 0x00;

   *StartPos = GetValue(Hold);
   if (*StartPos == -1)
      return(*StartPos);

  return(0);

}

LONG RightCopy(PSZ Num, LONG *EndPos)
{
   CHAR Hold[10];
   CHAR *HoldPtr;

   Num++;
   HoldPtr = Hold;

   while(*Num == ' ')
      Num++;

   while(*Num != ' ' && *Num != 0x00 && *Num != '\r' && *Num != '\n')
      *(HoldPtr++) = *(Num++);

   *HoldPtr = 0x00;
   *EndPos = GetValue(Hold);
   if (*EndPos == -1)
      return(*EndPos);

  return(0);

}

LONG LeftCopy(PSZ Num, LONG *BeginPos)
{
   CHAR Hold[10];
   CHAR *HoldPtr;

   Num++;
   HoldPtr = Hold;

   while(*Num == ' ')
      Num++;

   while(*Num != ' ' && *Num != 0x00 && *Num != '\r' && *Num != '\n')
      *(HoldPtr++) = *(Num++);

   *HoldPtr = 0x00;
   *BeginPos = GetValue(Hold);
   if (*BeginPos == -1)
      return(*BeginPos);

  return(0);

}

CHAR *GetAddr(PSZ DAddr,PSZ Buffer,SHORT Type)
{
   SHORT RecNum;
   CHAR *ptr;
   CHAR *ptr1;
   SHORT Offset;
   CHAR *RetPtr;

   switch(*DAddr)
   {
      case '$':
         DAddr++;
         if (isdigit(*DAddr))
            RetPtr = StrReg[*DAddr - '0'];
         else
            RetPtr = NULL;
         DAddr++;
         break;
      case '"':
         DAddr++;
         ptr1 = Buffer;
         while(*DAddr != '"' && *DAddr != 0x00)
            *(ptr1++) = *(DAddr++);
         *ptr1 = 0x00;

         if (*DAddr == 0x00)
            RetPtr = NULL;
         else
            RetPtr = Buffer;
         DAddr++;
         break;
      default:
         if (!isalpha(DAddr[0]))
            RetPtr = NULL;
         else
         {

            RecNum = DAddr[0] - 'A';
            DAddr++;
            Offset = GetFileRecNum(&DAddr);
            if (Offset != -1)
               if (Offset <= RecStr[RecNum].NumRec)
               {
                  RetPtr = &RecStr[RecNum].RecData[ (RecStr[RecNum].RecLen + 2) * Offset];
                  if (RecStr[RecNum].fHandle != 0)
                  {
                     switch(Type)
                     {
                        case 1:
                           break;
                        case 2:
                        {
                           ULONG CpyRec;
                           ULONG ulFileNew;
                           ULONG BytesRead;

                           CpyRec = (Offset * (RecStr[RecNum].RecLen + 1)) + 10;
                           DosSetFilePtr(RecStr[RecNum].fHandle,CpyRec,FILE_BEGIN,&ulFileNew);
                           DosRead(RecStr[RecNum].fHandle,RetPtr,RecStr[RecNum].RecLen,&BytesRead);
                           break;
                        }
                     }
                  }
               }
               else
                  RetPtr = NULL;
            else
               RetPtr = NULL;
         }
         break;
   }
   while (*DAddr == ' ')
      DAddr++;

   switch(*DAddr)
   {
      case 'L':
      {
         LONG EndPos;
         LONG rc;

         rc = LeftCopy(DAddr,&EndPos);
         if (rc == 0)
         {

            strncpy(Buffer,RetPtr,EndPos);
            Buffer[EndPos] = 0x00;
            RetPtr = Buffer;
         }
         else
         {

            printf("Syntax error in Left copy command %ld - aborting",rc);
            return(0);
         }
         break;
      }
      case 'M':
      {
         LONG StartPos;
         LONG CLen;
         LONG rc;


         rc = MiddleCopy(DAddr,&StartPos,&CLen);
         if (rc == 0)
         {

            strncpy(Buffer,&RetPtr[StartPos-1],CLen);
            Buffer[CLen] = 0x00;
            RetPtr = Buffer;
         }
         else
         {
            printf("Syntax error in Middle copy command %ld - aborting",rc);
            return(0);
         }
         break;
      }
      case 'R':
      {
         CHAR Hold[256];
         LONG StartPos;
         LONG Len;
         LONG rc;


         rc = RightCopy(DAddr,&StartPos);
         if (rc == 0)
         {
            Len = strlen(RetPtr);
            if (Len > StartPos)
            {
               strcpy(Hold,&RetPtr[Len - StartPos]);
               strcpy(Buffer,Hold);
               RetPtr = Buffer;
            }
         }
         else
         {
            printf("Syntax error in Right copy command %ld - aborting",rc);
            return(0);
         }

         break;
      }
      case '\r':
      case '\n':
      case 0x00:
         break;
      case '+':
      {
         CHAR *Cat;
         CHAR *NewVal;
         CHAR *NewBuf;
         SHORT rc;

         DAddr++;
         while (*DAddr == ' ')
            DAddr++;
         Cat = DAddr;
         rc = DosAllocMem((VOID *)&NewBuf,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
         if (rc == 0)
         {

            switch (FindType(Cat))
            {
               case 1:
               {
                  LONG NumHold1;
                  LONG *Num1;

                  Num1 = GetAddrNum(Cat,&NumHold1);
                  sprintf(NewBuf,"%d", *Num1);
                  NewVal = NewBuf;
                  break;
               }
               case 2:
                  NewVal = GetAddr(Cat,NewBuf,1);
                  break;
            }
            strcpy(Buffer,RetPtr);
            strcat(Buffer,NewVal);
            DosFreeMem(NewBuf);
         }
         else
         {
            printf("Dos Alloc Memory Failure. RC = %d",rc);
            return(0);
         }
         RetPtr = Buffer;
         break;
      }
      default:
         RetPtr = NULL;
         break;
   }
   return(RetPtr);
}

SHORT GetFileRecNum(PSZ *RecNum)
{
   CHAR Hold[20];
   CHAR *HoldPtr;
   CHAR *Start;
   LONG lVal;

   Start = *RecNum;

   if (**RecNum != '(')
   {
      printf("Syntax Error Get File Record Num - %s - Aborting",Start);
      return(-1);
   }
   (*RecNum)++;
   HoldPtr = Hold;
   while(**RecNum == ' ')
      (*RecNum)++;

   while(**RecNum != ')' && *RecNum != 0x00)
      *(HoldPtr++) = *((*RecNum)++);

   *HoldPtr = 0x00;
   if (**RecNum != ')')
   {
      printf("Syntax Error Get File Record Num - %s - Aborting",Start);
      return(-1);
   }
   (*RecNum)++;

   lVal = GetValue(Hold);
   return(lVal);
}

SHORT DeleteSpace(PSZ Buf)
{
   PSZ Buf1;

   Buf1 = Buf;

   while(*Buf1)
      Buf1++;
   Buf1--;
   while(*Buf1 == ' ')
      *(Buf1--) = 0x00;
   return(0);
}

LONG *GetAddrNum(PSZ DAddr, LONG *Hold)
{
   SHORT RecNum;
   LONG *RetPtr;
   CHAR szHold[20];
   CHAR *pszHold;


   switch(*DAddr)
   {
      case '#':
         DAddr++;
         if (isdigit(*DAddr))
            RetPtr = (LONG *)&NumReg[*DAddr - '0'];
         else
            RetPtr = NULL;
         DAddr++;
         break;
      case '[':
      {
         CHAR *BufPtr;
         CHAR *NewVal;
         SHORT rc;
         CHAR Buffer[256];
         CHAR *ptr1;

         rc = DosAllocMem((VOID *)&BufPtr,2000,PAG_READ | PAG_WRITE | PAG_COMMIT);
         if (rc == 0)
         {
            DAddr++;
            ptr1 = Buffer;
            while(*DAddr != ']' && *DAddr != 0x00)
               *(ptr1++) = *(DAddr++);
            *ptr1 = 0x00;
            NewVal = GetAddr(Buffer,BufPtr,1);
            *Hold = strlen(NewVal);
            RetPtr = Hold;
            DosFreeMem(BufPtr);
            DAddr++;
         }
         else
         {
            printf("Dos Alloc Memory Failure. RC = %d",rc);
            fflush(NULL);
            return(0);
         }

         break;
      }
      default:
         while(*DAddr == ' ')
            DAddr++;

         if (!isdigit(*DAddr))
            RetPtr = NULL;
         else
         {
            pszHold = szHold;
            while(isdigit(*DAddr))
               *(pszHold++) = *(DAddr++);
            *pszHold = 0x00;
            *Hold = atol(szHold);
            RetPtr = Hold;
         }
         break;
   }
   while (*DAddr == ' ')
      DAddr++;
   switch (*DAddr)
   {
      case '\r':
      case '\n':
      case 0x00:
         break;
      case '+':
      {
         CHAR *Cat;
         LONG *NewVal;
         LONG NewNum;

         DAddr++;
         while (*DAddr == ' ')
            DAddr++;
         Cat = DAddr;
         switch (FindType(Cat))
         {
            case 1:
               NewVal = GetAddrNum(Cat,&NewNum);
               *Hold = *RetPtr;
               *Hold += *NewVal;
               RetPtr = Hold;
               break;
            case 2:
               printf("\x1b[2J\x1b[3;1HCannot add alpha to numeric");
               exit(0);
               break;
         }
         break;
      }
      case '-':
      {
         CHAR *Cat;
         LONG *NewVal;
         LONG NewNum;

         DAddr++;
         while (*DAddr == ' ')
            DAddr++;
         Cat = DAddr;
         NewVal = GetAddrNum(Cat,&NewNum);
         *Hold = *RetPtr;
         *Hold -= *NewVal;
         RetPtr = Hold;
         break;
      }
      default:
         RetPtr = NULL;
         break;
   }
   return(RetPtr);
}

SHORT FindType(PSZ szType)
{
   SHORT Type;

   switch(szType[0])
   {
      case '[':
      case '#':
         Type = 1;
         break;
      case '$':
      case '"':
         Type = 2;
         break;
      default:
         if (isalpha(szType[0]))
            Type = 2;
         if (isdigit(szType[0]))
            Type = 1;
         break;
   }
   return(Type);
}
SHORT FindCondition(PSZ Cond)
{
   if (strcmp(Cond,"<>") == 0)
      return(1);
   if (strcmp(Cond,"=>") == 0)
      return(2);
   if (strcmp(Cond,"=") == 0)
      return(3);
   if (strcmp(Cond,">") == 0)
      return(4);
   if (strcmp(Cond,"<") == 0)
      return(5);
   if (strcmp(Cond,"<=") == 0)
      return(6);
   return(0);
}
CHAR *FindLineLabel(PSZ Line, PSZ Buf)
{
   PSZ BufStart;
   CHAR *RetAddr;

   BufStart = Buf;
   RetAddr = NULL;

   while(*Line != '.' && *Line != 0x00)
      Line++;
   if (*Line == '.')
   {
      while(*Line != ' ' && *Line != 0x00 && *Line != '\r' && *Line != '\n')
         *(Buf++) = *(Line++);

      *Buf = 0x00;
      RetAddr = BufStart;
   }
   return(RetAddr);
}

CHAR *GetSubAddr(PSZ LineLabel,LABELSTRUCT *LabelStr, ULONG LineLabels)
{
   SHORT x;
   CHAR *RetAddr;

   x = 0;
   RetAddr = NULL;
   while (strcmp(LineLabel,(LabelStr + x)->LabelName) != 0 && x < LineLabels)
      x++;
   if (x < LineLabels)
      RetAddr = (LabelStr + x)->StartAddr;
   return(RetAddr);
}
CHAR *FixControl(PSZ Buf)
{
   CHAR *Cpy1;
   CHAR *Cpy2;
   CHAR *StartBuf;

   StartBuf = Buf;

   while(*Buf != 0x00)
   {
      if (*Buf == '\\')
      {
         switch(*(Buf + 1))
         {
            case 'e':
            case 'E':
               *Buf = 27;
               break;
            case 'n':
            case 'N':
               *Buf = 0x0A;
               break;
            case 'r':
            case 'R':
               *Buf = 0x0D;
               break;
         }
         Cpy1 = Buf + 1;
         Cpy2 = Buf + 2;
         while(*Cpy2)
            *(Cpy1++) = *(Cpy2++);
         *Cpy1 = 0x00;
      }

      Buf++;
   }

   return(StartBuf);
}

int initcomm(PSZ comport,ULONG bitrate,CHAR par,USHORT dbits,USHORT sbits)

{
   ULONG LIO;
   ULONG PIO;
   APIRET rc;
   ULONG comerr,comaction;
   DCBINFO  dcb;
   LINECONTROL linechar;
   MODEMSTATUS modemctrl;
   HFILE comhandle;


/*  format:  DosDevIOCtl(&data,&parm,functcode,category,handle);  */

  /* Open com device driver: */

  if (DosOpen(comport,&comhandle,&comaction,0L,0,0x01,0x0012,0L))
      {
/*      fprintf(stderr,"\nError opening %s\r\n",comport); */
      return(1);
      }

  /* Set bitrate: */

   PIO = 4L;
   LIO = 0L;
  if (DosDevIOCtl(comhandle,01,0x41,&bitrate,4L,&PIO,0L,0L,&LIO))
      {
/*      fprintf(stderr,"\nError setting bitrate of %i\r\n",bitrate);  */
      return(1);
      }

  /* Set databits, stopbits, parity: */

  if (par == 'N') linechar.bParity = 0;
  if (par == 'O') linechar.bParity = 1;
  if (par == 'E') linechar.bParity = 2;
  if (par == 'M') linechar.bParity = 3;
  if (par == 'S') linechar.bParity = 4;

  if (sbits == 2) linechar.bStopBits = 2;
  if (sbits == 1) linechar.bStopBits = 0;

  linechar.bDataBits = dbits;
   PIO = 5L;
   LIO = 0L;
  if (DosDevIOCtl(comhandle,01,0x42,&linechar,5L,&PIO,0L,0L,&LIO))
      {
/*      puts("Error setting line characteristics"); */
      return(1);
      }

  /* Set modem control signals: */

  modemctrl.fbModemOn = 0x03;     /* turn DTR and RTS on */
  /* modemctrl.fbModemOff = 0xff;    turn no signals off */
  modemctrl.fbModemOff = 0xff;    /* turn no signals off */
   PIO = 5L;
   LIO = 4L;
  if (DosDevIOCtl(comhandle,01,0x46,&modemctrl,5L,&PIO,&comerr,4L,&LIO))
      {
/*      puts("Error setting modem control signals"); */
      return(1);
      }

  /* Set com device processing parameters: */

  dcb.usWriteTimeout = 20;      /* set 2 second transmit timeout */
  dcb.usReadTimeout = 300;     /* set 30 second receive timeout */
  dcb.fbCtlHndShake = 0x01;   /* enable DTR, disable hardware handshaking */
  dcb.fbFlowReplace = 0x43;   /* enable RTS, enable XON/XOFF  */
  dcb.fbTimeout = 0x04;   /* no read timeout processing */
  dcb.bErrorReplacementChar = 0x00;  /* no error character translation */
  dcb.bBreakReplacementChar = 0x00;  /* no break character translation */
  dcb.bXONChar = 0x11;  /* standard XON  */
  dcb.bXOFFChar = 0x13; /* standard XOFF */
   PIO = sizeof(dcb);
   LIO = 0L;
  if (DosDevIOCtl(comhandle,IOCTL_ASYNC,ASYNC_SETDCBINFO,&dcb,
                        4L,&PIO,0L,0L,&LIO))
      {
/*       puts("Error setting device control block"); */
      return(1);
      }

  DosClose(comhandle); /* Close com device driver - all
                          parameters will be retained */

  return(0);

}


