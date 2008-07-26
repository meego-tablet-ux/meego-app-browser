/* Automatically generated.  Do not edit */
/* See the mkopcodeh.awk script for details */
#define OP_MemLoad                              1
#define OP_VNext                                2
#define OP_HexBlob                            126   /* same as TK_BLOB     */
#define OP_Column                               3
#define OP_SetCookie                            4
#define OP_IfMemPos                             5
#define OP_Real                               125   /* same as TK_FLOAT    */
#define OP_Sequence                             6
#define OP_MoveGt                               7
#define OP_Ge                                  72   /* same as TK_GE       */
#define OP_RowKey                               8
#define OP_Eq                                  68   /* same as TK_EQ       */
#define OP_OpenWrite                            9
#define OP_NotNull                             66   /* same as TK_NOTNULL  */
#define OP_If                                  10
#define OP_ToInt                              141   /* same as TK_TO_INT   */
#define OP_String8                             88   /* same as TK_STRING   */
#define OP_Pop                                 11
#define OP_VRowid                              12
#define OP_CollSeq                             13
#define OP_OpenRead                            14
#define OP_Expire                              15
#define OP_AutoCommit                          17
#define OP_Gt                                  69   /* same as TK_GT       */
#define OP_IntegrityCk                         18
#define OP_Sort                                19
#define OP_Function                            20
#define OP_And                                 61   /* same as TK_AND      */
#define OP_Subtract                            79   /* same as TK_MINUS    */
#define OP_Noop                                21
#define OP_Return                              22
#define OP_Remainder                           82   /* same as TK_REM      */
#define OP_NewRowid                            23
#define OP_Multiply                            80   /* same as TK_STAR     */
#define OP_IfMemNeg                            24
#define OP_Variable                            25
#define OP_String                              26
#define OP_RealAffinity                        27
#define OP_VRename                             28
#define OP_ParseSchema                         29
#define OP_VOpen                               30
#define OP_Close                               31
#define OP_CreateIndex                         32
#define OP_IsUnique                            33
#define OP_NotFound                            34
#define OP_Int64                               35
#define OP_MustBeInt                           36
#define OP_Halt                                37
#define OP_Rowid                               38
#define OP_IdxLT                               39
#define OP_AddImm                              40
#define OP_Statement                           41
#define OP_RowData                             42
#define OP_MemMax                              43
#define OP_Push                                44
#define OP_Or                                  60   /* same as TK_OR       */
#define OP_NotExists                           45
#define OP_MemIncr                             46
#define OP_Gosub                               47
#define OP_Divide                              81   /* same as TK_SLASH    */
#define OP_Integer                             48
#define OP_ToNumeric                          140   /* same as TK_TO_NUMERIC*/
#define OP_MemInt                              49
#define OP_Prev                                50
#define OP_Concat                              83   /* same as TK_CONCAT   */
#define OP_BitAnd                              74   /* same as TK_BITAND   */
#define OP_VColumn                             51
#define OP_CreateTable                         52
#define OP_Last                                53
#define OP_IsNull                              65   /* same as TK_ISNULL   */
#define OP_IncrVacuum                          54
#define OP_IdxRowid                            55
#define OP_MakeIdxRec                          56
#define OP_ShiftRight                          77   /* same as TK_RSHIFT   */
#define OP_ResetCount                          57
#define OP_FifoWrite                           58
#define OP_Callback                            59
#define OP_ContextPush                         62
#define OP_DropTrigger                         63
#define OP_DropIndex                           64
#define OP_IdxGE                               73
#define OP_IdxDelete                           84
#define OP_Vacuum                              86
#define OP_MoveLe                              89
#define OP_IfNot                               90
#define OP_DropTable                           91
#define OP_MakeRecord                          92
#define OP_ToBlob                             139   /* same as TK_TO_BLOB  */
#define OP_Delete                              93
#define OP_AggFinal                            94
#define OP_ShiftLeft                           76   /* same as TK_LSHIFT   */
#define OP_Dup                                 95
#define OP_Goto                                96
#define OP_TableLock                           97
#define OP_FifoRead                            98
#define OP_Clear                               99
#define OP_IdxGT                              100
#define OP_MoveLt                             101
#define OP_Le                                  70   /* same as TK_LE       */
#define OP_VerifyCookie                       102
#define OP_AggStep                            103
#define OP_Pull                               104
#define OP_ToText                             138   /* same as TK_TO_TEXT  */
#define OP_Not                                 16   /* same as TK_NOT      */
#define OP_ToReal                             142   /* same as TK_TO_REAL  */
#define OP_SetNumColumns                      105
#define OP_AbsValue                           106
#define OP_Transaction                        107
#define OP_VFilter                            108
#define OP_Negative                            85   /* same as TK_UMINUS   */
#define OP_Ne                                  67   /* same as TK_NE       */
#define OP_VDestroy                           109
#define OP_ContextPop                         110
#define OP_BitOr                               75   /* same as TK_BITOR    */
#define OP_Next                               111
#define OP_IdxInsert                          112
#define OP_Distinct                           113
#define OP_Lt                                  71   /* same as TK_LT       */
#define OP_Insert                             114
#define OP_Destroy                            115
#define OP_ReadCookie                         116
#define OP_ForceInt                           117
#define OP_LoadAnalysis                       118
#define OP_Explain                            119
#define OP_IfMemZero                          120
#define OP_OpenPseudo                         121
#define OP_OpenEphemeral                      122
#define OP_Null                               123
#define OP_Blob                               124
#define OP_Add                                 78   /* same as TK_PLUS     */
#define OP_MemStore                           127
#define OP_Rewind                             128
#define OP_MoveGe                             129
#define OP_VBegin                             130
#define OP_VUpdate                            131
#define OP_BitNot                              87   /* same as TK_BITNOT   */
#define OP_VCreate                            132
#define OP_MemMove                            133
#define OP_MemNull                            134
#define OP_Found                              135
#define OP_NullRow                            136

/* The following opcode values are never used */
#define OP_NotUsed_137                        137

/* Opcodes that are guaranteed to never push a value onto the stack
** contain a 1 their corresponding position of the following mask
** set.  See the opcodeNoPush() function in vdbeaux.c  */
#define NOPUSH_MASK_0 0xeeb4
#define NOPUSH_MASK_1 0xf96b
#define NOPUSH_MASK_2 0xfbb6
#define NOPUSH_MASK_3 0xfe64
#define NOPUSH_MASK_4 0xffff
#define NOPUSH_MASK_5 0x6ef7
#define NOPUSH_MASK_6 0xfbfb
#define NOPUSH_MASK_7 0x8767
#define NOPUSH_MASK_8 0x7d9f
#define NOPUSH_MASK_9 0x0000
