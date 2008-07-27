""" Python Character Mapping Codec generated from '8859-5.TXT' with gencodec.py.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.
(c) Copyright 2000 Guido van Rossum.

"""#"

import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self,input,errors='strict'):

        return codecs.charmap_encode(input,errors,encoding_map)

    def decode(self,input,errors='strict'):

        return codecs.charmap_decode(input,errors,decoding_map)

class StreamWriter(Codec,codecs.StreamWriter):
    pass

class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():

    return (Codec().encode,Codec().decode,StreamReader,StreamWriter)

### Decoding Map

decoding_map = codecs.make_identity_dict(range(256))
decoding_map.update({
        0x00a1: 0x0401, #       CYRILLIC CAPITAL LETTER IO
        0x00a2: 0x0402, #       CYRILLIC CAPITAL LETTER DJE
        0x00a3: 0x0403, #       CYRILLIC CAPITAL LETTER GJE
        0x00a4: 0x0404, #       CYRILLIC CAPITAL LETTER UKRAINIAN IE
        0x00a5: 0x0405, #       CYRILLIC CAPITAL LETTER DZE
        0x00a6: 0x0406, #       CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
        0x00a7: 0x0407, #       CYRILLIC CAPITAL LETTER YI
        0x00a8: 0x0408, #       CYRILLIC CAPITAL LETTER JE
        0x00a9: 0x0409, #       CYRILLIC CAPITAL LETTER LJE
        0x00aa: 0x040a, #       CYRILLIC CAPITAL LETTER NJE
        0x00ab: 0x040b, #       CYRILLIC CAPITAL LETTER TSHE
        0x00ac: 0x040c, #       CYRILLIC CAPITAL LETTER KJE
        0x00ae: 0x040e, #       CYRILLIC CAPITAL LETTER SHORT U
        0x00af: 0x040f, #       CYRILLIC CAPITAL LETTER DZHE
        0x00b0: 0x0410, #       CYRILLIC CAPITAL LETTER A
        0x00b1: 0x0411, #       CYRILLIC CAPITAL LETTER BE
        0x00b2: 0x0412, #       CYRILLIC CAPITAL LETTER VE
        0x00b3: 0x0413, #       CYRILLIC CAPITAL LETTER GHE
        0x00b4: 0x0414, #       CYRILLIC CAPITAL LETTER DE
        0x00b5: 0x0415, #       CYRILLIC CAPITAL LETTER IE
        0x00b6: 0x0416, #       CYRILLIC CAPITAL LETTER ZHE
        0x00b7: 0x0417, #       CYRILLIC CAPITAL LETTER ZE
        0x00b8: 0x0418, #       CYRILLIC CAPITAL LETTER I
        0x00b9: 0x0419, #       CYRILLIC CAPITAL LETTER SHORT I
        0x00ba: 0x041a, #       CYRILLIC CAPITAL LETTER KA
        0x00bb: 0x041b, #       CYRILLIC CAPITAL LETTER EL
        0x00bc: 0x041c, #       CYRILLIC CAPITAL LETTER EM
        0x00bd: 0x041d, #       CYRILLIC CAPITAL LETTER EN
        0x00be: 0x041e, #       CYRILLIC CAPITAL LETTER O
        0x00bf: 0x041f, #       CYRILLIC CAPITAL LETTER PE
        0x00c0: 0x0420, #       CYRILLIC CAPITAL LETTER ER
        0x00c1: 0x0421, #       CYRILLIC CAPITAL LETTER ES
        0x00c2: 0x0422, #       CYRILLIC CAPITAL LETTER TE
        0x00c3: 0x0423, #       CYRILLIC CAPITAL LETTER U
        0x00c4: 0x0424, #       CYRILLIC CAPITAL LETTER EF
        0x00c5: 0x0425, #       CYRILLIC CAPITAL LETTER HA
        0x00c6: 0x0426, #       CYRILLIC CAPITAL LETTER TSE
        0x00c7: 0x0427, #       CYRILLIC CAPITAL LETTER CHE
        0x00c8: 0x0428, #       CYRILLIC CAPITAL LETTER SHA
        0x00c9: 0x0429, #       CYRILLIC CAPITAL LETTER SHCHA
        0x00ca: 0x042a, #       CYRILLIC CAPITAL LETTER HARD SIGN
        0x00cb: 0x042b, #       CYRILLIC CAPITAL LETTER YERU
        0x00cc: 0x042c, #       CYRILLIC CAPITAL LETTER SOFT SIGN
        0x00cd: 0x042d, #       CYRILLIC CAPITAL LETTER E
        0x00ce: 0x042e, #       CYRILLIC CAPITAL LETTER YU
        0x00cf: 0x042f, #       CYRILLIC CAPITAL LETTER YA
        0x00d0: 0x0430, #       CYRILLIC SMALL LETTER A
        0x00d1: 0x0431, #       CYRILLIC SMALL LETTER BE
        0x00d2: 0x0432, #       CYRILLIC SMALL LETTER VE
        0x00d3: 0x0433, #       CYRILLIC SMALL LETTER GHE
        0x00d4: 0x0434, #       CYRILLIC SMALL LETTER DE
        0x00d5: 0x0435, #       CYRILLIC SMALL LETTER IE
        0x00d6: 0x0436, #       CYRILLIC SMALL LETTER ZHE
        0x00d7: 0x0437, #       CYRILLIC SMALL LETTER ZE
        0x00d8: 0x0438, #       CYRILLIC SMALL LETTER I
        0x00d9: 0x0439, #       CYRILLIC SMALL LETTER SHORT I
        0x00da: 0x043a, #       CYRILLIC SMALL LETTER KA
        0x00db: 0x043b, #       CYRILLIC SMALL LETTER EL
        0x00dc: 0x043c, #       CYRILLIC SMALL LETTER EM
        0x00dd: 0x043d, #       CYRILLIC SMALL LETTER EN
        0x00de: 0x043e, #       CYRILLIC SMALL LETTER O
        0x00df: 0x043f, #       CYRILLIC SMALL LETTER PE
        0x00e0: 0x0440, #       CYRILLIC SMALL LETTER ER
        0x00e1: 0x0441, #       CYRILLIC SMALL LETTER ES
        0x00e2: 0x0442, #       CYRILLIC SMALL LETTER TE
        0x00e3: 0x0443, #       CYRILLIC SMALL LETTER U
        0x00e4: 0x0444, #       CYRILLIC SMALL LETTER EF
        0x00e5: 0x0445, #       CYRILLIC SMALL LETTER HA
        0x00e6: 0x0446, #       CYRILLIC SMALL LETTER TSE
        0x00e7: 0x0447, #       CYRILLIC SMALL LETTER CHE
        0x00e8: 0x0448, #       CYRILLIC SMALL LETTER SHA
        0x00e9: 0x0449, #       CYRILLIC SMALL LETTER SHCHA
        0x00ea: 0x044a, #       CYRILLIC SMALL LETTER HARD SIGN
        0x00eb: 0x044b, #       CYRILLIC SMALL LETTER YERU
        0x00ec: 0x044c, #       CYRILLIC SMALL LETTER SOFT SIGN
        0x00ed: 0x044d, #       CYRILLIC SMALL LETTER E
        0x00ee: 0x044e, #       CYRILLIC SMALL LETTER YU
        0x00ef: 0x044f, #       CYRILLIC SMALL LETTER YA
        0x00f0: 0x2116, #       NUMERO SIGN
        0x00f1: 0x0451, #       CYRILLIC SMALL LETTER IO
        0x00f2: 0x0452, #       CYRILLIC SMALL LETTER DJE
        0x00f3: 0x0453, #       CYRILLIC SMALL LETTER GJE
        0x00f4: 0x0454, #       CYRILLIC SMALL LETTER UKRAINIAN IE
        0x00f5: 0x0455, #       CYRILLIC SMALL LETTER DZE
        0x00f6: 0x0456, #       CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
        0x00f7: 0x0457, #       CYRILLIC SMALL LETTER YI
        0x00f8: 0x0458, #       CYRILLIC SMALL LETTER JE
        0x00f9: 0x0459, #       CYRILLIC SMALL LETTER LJE
        0x00fa: 0x045a, #       CYRILLIC SMALL LETTER NJE
        0x00fb: 0x045b, #       CYRILLIC SMALL LETTER TSHE
        0x00fc: 0x045c, #       CYRILLIC SMALL LETTER KJE
        0x00fd: 0x00a7, #       SECTION SIGN
        0x00fe: 0x045e, #       CYRILLIC SMALL LETTER SHORT U
        0x00ff: 0x045f, #       CYRILLIC SMALL LETTER DZHE
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
