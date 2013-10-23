
typedef struct
{
   CHAR LabelName[30];
   CHAR *StartAddr;
} LABELSTRUCT;

typedef struct
{
   CHAR FileName[20];
   HFILE fHandle;
   ULONG fHAction;
   ULONG NumRec;
   ULONG RecLen;
   ULONG MaxBytes;
   ULONG RecPos;
   CHAR *RecData;
} RECORDSTRUCT;

RECORDSTRUCT RecStr[26];


/* Function Prototypes */

int main(int argc, char *argv[]);
SHORT Condition(CHAR exp);
LONG GetValue(PSZ Num);
LONG MiddleCopy(PSZ Num, LONG *StartPos, LONG *CLen);
CHAR *GetAddr(PSZ DAddr,PSZ Buffer,SHORT Type);
SHORT GetFileRecNum(PSZ *RecNum);
SHORT DeleteSpace(PSZ Buf);
LONG *GetAddrNum(PSZ DAddr, LONG *Hold);
SHORT FindType(PSZ szType);
SHORT FindCondition(PSZ Cond);
CHAR *FindLineLabel(PSZ Line, PSZ Buf);
CHAR *GetSubAddr(PSZ LineLabel,LABELSTRUCT *LabelStr, ULONG LineLabels);
CHAR *FixControl(PSZ Buf);
int initcomm(PSZ comport,ULONG bitrate,CHAR par,USHORT dbits,USHORT sbits);



