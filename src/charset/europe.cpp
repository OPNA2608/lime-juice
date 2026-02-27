#include "../charset.h"

// europe charset definition
// translated from _charset_europe.rkt

void register_charset_europe(Charset& cs) {
    // chains pc98 as base
    register_charset_pc98(cs);

    // ASCII at kuten row 48, starting at column 1
    // (charset* 48 1 #\! #\" ... #\~)
    cs.register_kuten_range(48, 1, {
        U'!', U'"', U'#', U'$', U'%', U'&', U'\'', U'(', U')', U'*', U'+', U',', U'-', U'.', U'/',
        U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9',
        U':', U';', U'<', U'=', U'>', U'?', U'@',
        U'A', U'B', U'C', U'D', U'E', U'F', U'G', U'H', U'I', U'J', U'K', U'L', U'M',
        U'N', U'O', U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W', U'X', U'Y', U'Z',
        U'[', U'\\', U']', U'^', U'_', U'`',
        U'a', U'b', U'c', U'd', U'e', U'f', U'g', U'h', U'i', U'j', U'k', U'l', U'm',
        U'n', U'o', U'p', U'q', U'r', U's', U't', U'u', U'v', U'w', U'x', U'y', U'z',
        U'{', U'|', U'}', U'~',
    });

    // european accented characters at kuten row 49, starting at column 1
    // (charset* 49 1 #\Ǎ #\ǎ ... #\έ)
    cs.register_kuten_range_str(49, 1,
        u8"ǍǎǐḾḿǸǹǑǒǔǖǘǚǜ"
        u8"¿"
        u8"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝ"
        u8"Þßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ"
        u8"ĀĪŪĒŌāīūēō"
        u8"ĄŁĽŚŠŞŤŹŽŻ"
        u8"ąłľśšşťźžż"
        u8"ŔĂĹĆČĘĚĎŃŇŐŘŮŰŢ"
        u8"ŕăĺćčęěďđńňőřůűţ"
        u8"ĈĜĤĴŜŬ"
        u8"ĉĝĥĵŝŭ"
        u8"ɱʋɾʃʒɬɮɹʈɖɳɽʂʐɻɭɟɲʝʎɡŋɰʁħʕʔɦʘǂɓɗʄɠƓœŒɨʉɘɵəɜɞɐɯʊɤʌɔɑɒʍɥʢʡɕʑɺɧɚ"
        u8"æǽὰάɔɔʌʌəəɚɚὲέ");

    cs.set_fontwidth(1);
    cs.set_space_char(U' ');
}
