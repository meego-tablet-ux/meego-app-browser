# patch for apostrophe handling (including Unicode apostrophe)
s/\(RIGHTHYPHENMIN.*\)/\1\
COMPOUNDLEFTHYPHENMIN 2\
COMPOUNDRIGHTHYPHENMIN 3\
1'.\
1's.\/'=s,1,2\
1't.\/'=t,1,2\
1’.\
1’s.\/’=s,1,2\
1’t.\/’=t,1,2\
NEXTLEVEL\
4'4\
4a4'4\
4b4'4\
4c4'4\
4d4'4\
4e4'4\
4f4'4\
4g4'4\
4h4'4\
4i4'4\
4j4'4\
4k4'4\
4l4'4\
4m4'4\
4n4'4\
4o4'4\
4p4'4\
4q4'4\
4r4'4\
4s4'4\
4t4'4\
4u4'4\
4v4'4\
4w4'4\
4x4'4\
4y4'4\
4z4'4\
'a4\
'b4\
'c4\
'd4\
'e4\
'f4\
'g4\
'h4\
'i4\
'j4\
'k4\
'l4\
'm4\
'n4\
'o4\
'p4\
'q4\
'r4\
's4\
't4\
'u4\
'v4\
'w4\
'x4\
'y4\
'z4\
4’4\
4a4’4\
4b4’4\
4c4’4\
4d4’4\
4e4’4\
4f4’4\
4g4’4\
4h4’4\
4i4’4\
4j4’4\
4k4’4\
4l4’4\
4m4’4\
4n4’4\
4o4’4\
4p4’4\
4q4’4\
4r4’4\
4s4’4\
4t4’4\
4u4’4\
4v4’4\
4w4’4\
4x4’4\
4y4’4\
4z4’4\
’a4\
’b4\
’c4\
’d4\
’e4\
’f4\
’g4\
’h4\
’i4\
’j4\
’k4\
’l4\
’m4\
’n4\
’o4\
’p4\
’q4\
’r4\
’s4\
’t4\
’u4\
’v4\
’w4\
’x4\
’y4\
’z4/
