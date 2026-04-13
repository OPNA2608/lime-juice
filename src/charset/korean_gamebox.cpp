//
// lime-juice: C++ port of Tomyun's "Juice" de/recompiler for PC-98 games
// Copyright (C) 2026 Fuzion
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "../charset.h"

// korean gamebox charset definition
// translated from _charset_korean-gamebox.rkt

void register_charset_korean_gamebox(Charset& cs) {
    // NOTE: does NOT chain pc98 - starts fresh

    // hangul syllables, row 1 col 1-28
    cs.register_kuten_range_str(1, 1,
        u8"가각간갇갈갉갊감갑값갓갔강갖갗같갚갛개객갠갤갬갭갯갰갱갸");

    // hangul syllables, row 1 col 30-94
    cs.register_kuten_range_str(1, 30,
        u8"갹갼걀걋걍걔걘걜거걱건걷걸걺검겁것겄겅겆겉겊겋게겐겔겜겝겟겠겡겨격겪견겯결겸겹겻겼경곁계곈곌곕곗고곡곤곧골곪곬곯곰곱곳공곶과곽관괄");

    // hangul syllables, row 2
    cs.register_kuten_range_str(2, 1,
        u8"괆괌괍괏광괘괜괠괩괬괭괴괵괸괼굄굅굇굉교굔굘굡굣구국군굳굴굵굶굻굼굽굿궁궂궈궉권궐궜궝궤궷귀귁귄귈귐귑귓규균귤그극근귿글긁금급긋긍긔기긱긴긷길긺김깁깃깄깅깆깊ㄲ까깍깎깐깔깖깜깝깟깠깡깥깨깩");

    // hangul syllables, row 3 col 1-28
    cs.register_kuten_range_str(3, 1,
        u8"깬깰깸깹깻깼깽꺄꺅꺌꺼꺽꺾껀껄껌껍껏껐껑께껙껜껨껫껭껴껸");

    // hangul syllables, row 3 col 30-94
    cs.register_kuten_range_str(3, 30,
        u8"껼꼇꼈꼍꼐꼬꼭꼰꼲꼴꼼꼽꼿꽁꽂꽃꽈꽉꽐꽜꽝꽤꽥꽹꾀꾄꾈꾐꾑꾕꾜꾸꾹꾼꿀꿇꿈꿉꿋꿍꿎꿔꿜꿨꿩꿰꿱꿴꿸뀀뀁뀄뀌뀐뀔뀜뀝뀨끄끅끈끊끌끎끓");

    // hangul syllables, row 4
    cs.register_kuten_range_str(4, 1,
        u8"끔끕끗끙끝끼끽낀낄낌낍낏낑ㄴ나낙낚난낟날낡낢남납낫났낭낮낯낱낳내낵낸낼냄냅냇냈냉냐냑냔냘냠냣냥너넉넋넌널넒넓넘넙넚넛넜넝넣네넥넨넬넴넵넷넸넹녀녁년녈념녑녔녕녘녜녠노녹논놀놂놈놉놋농높놓놔놘");

    // hangul syllables, row 5 col 1-28
    cs.register_kuten_range_str(5, 1,
        u8"놜놨뇌뇐뇔뇜뇝뇟뇨뇩뇬뇰뇹뇻뇽누눅눈눋눌눔눕눗눙눚눞눠눴");

    // hangul syllables, row 5 col 30-94
    cs.register_kuten_range_str(5, 30,
        u8"눼뉘뉜뉠뉨뉩뉴뉵뉼늄늅늉느늑는늘늙늚늠늡늣능늦늪늬늰늴니닉닌닐닒님닙닛닝닢ㄷ다닥닦단닫달닭닮닯닳담답닷닸당닺닻닿대댁댄댈댐댑댓댔댕");

    // hangul syllables, row 6
    cs.register_kuten_range_str(6, 1,
        u8"댜더덕덖던덛덜덞덟덤덥덧덩덫덮덯데덱덴델뎀뎁뎃뎄뎅뎌뎐뎔뎠뎡뎨뎬도독돈돋돌돎돐돔돕돗동돛돝돠돤돨돼됀됏됐되된될됨됩됫됴두둑둔둘둟둠둡둣둥둬뒀뒈뒝뒤뒨뒬뒵뒷뒹듀듄듈듐듕드득든듣들듦듬듭듯등듵");

    // hangul syllables, row 7 col 1-28
    cs.register_kuten_range_str(7, 1,
        u8"듸디딕딘딛딜딤딥딧딨딩딪딫ㄸ따딱딴딸땀땁땃땄땅땋때땍땐땔");

    // hangul syllables, row 7 col 30-94
    cs.register_kuten_range_str(7, 30,
        u8"땜땝땟땠땡떄떈떠떡떤떨떪떫떰떱떳떴떵떻떼떽뗀뗄뗌뗍뗏뗐뗑뗘뗬또똑똔똘똥똬똴뙈뙤뙨뚜뚝뚠뚤뚫뚬뚱뛔뛰뛴뛸뜀뜁뜅뜨뜩뜬뜯뜰뜸뜹뜻띄띈띌");

    // hangul syllables, row 8
    cs.register_kuten_range_str(8, 1,
        u8"띔띕띠띤띨띰띱띳띵ㄹ라락란랄람랍랏랐랑랒랖랗래랙랜랠램랩랫랬랭랴략랸럇량러럭런럴럼럽럿렀렁렇레렉렌렐렘렙렛렝려력련렬렴렵렷렸령례롄롑롓로록론롤롬롭롯롱롸롼뢍뢨뢰뢴뢸룀룁룃룅료룐룔룝룟룡루룩");

    // hangul syllables, row 12 (continues from row 8)
    cs.register_kuten_range_str(12, 1,
        u8"룬룰룸룹룻룽뤄뤘뤠뤼뤽륀륄륌륏륑류륙륜률륨륩륫륭르륵른를름릅릇릉릊릍릎리릭린릴림립릿링ㅁ마막만많맏말맑맒맘맙맛맜망맞맡맣매맥맨맬맴맵맷맸맹맺먀먁먈먕머먹먼먿멀멂멈멉멋멍멎멓메멕멘멜멤멥멧멨");

    // hangul syllables, row 13 col 1-28
    cs.register_kuten_range_str(13, 1,
        u8"멩며멱면멸몃몄명몇몌모목몫몬몰몲몸몹못몽뫄뫈뫘뫙뫼묀묄묍");

    // hangul syllables, row 13 col 30-94
    cs.register_kuten_range_str(13, 30,
        u8"묏묑묘묜묠묩묫무묵묶문묻물묽묾뭄뭅뭇뭉뭍뭏뭐뭔뭘뭡뭣뭬뮈뮌뮐뮤뮨뮬뮴뮷므믄믈믐믓미믹민믿밀밂밈밉밋밌밍및밑ㅂ바박밖밗반받발밝밞밟밤");

    // hangul syllables, row 14
    cs.register_kuten_range_str(14, 1,
        u8"밥밧방밭배백밴밸뱀뱁뱃뱄뱅뱉뱌뱍뱐뱝버벅번벋벌벎범법벗벙벚벛베벡벤벧벨벰벱벳벴벵벼벽변별볍볏볐병볓볕볘볜보복볶본볼봄봅봇봉봐봔봣봤봬뵀뵈뵉뵌뵐뵘뵙뵤뵨부북분붇불붉붊붐붑붓붕붙붚붜붤붰붸뷔뷕");

    // hangul syllables, row 15 col 1-28
    cs.register_kuten_range_str(15, 1,
        u8"뷘뷜뷩뷰뷴뷸븀븃븅브븍븐블븜븝븟비빅빈빌빎빔빕빗빙빚빛ㅃ");

    // hangul syllables, row 15 col 30-94
    cs.register_kuten_range_str(15, 30,
        u8"빠빡빢빤빨빪빰빱빳빴빵빻빼빽뺀뺄뺌뺍뺏뺐뺑뺘뺙뺨뺴뻐뻑뻔뻗뻘뻠뻣뻤뻥뻬뼁뼈뼉뼌뼘뼙뼛뼜뼝뽀뽁뽄뽈뽐뽑뽕뾔뾰뿅뿌뿍뿐뿔뿜뿟뿡쀼쁑쁘쁜");

    // hangul syllables, row 16
    cs.register_kuten_range_str(16, 1,
        u8"쁠쁨쁩삐삑삔삘삠삡삣삥ㅅ사삭삯산삳살삵삶삼삽삿샀상샅새색샌샐샘샙샛샜생샤샥샨샬샴샵샷샹샾섀섄섈섐섕서석섞섟선섣설섦섧섬섭섯섰성섶세섹센셀셈셉셋셌셍셔셕션셜셤셥셧셨셩셰셴셸솅소속솎손솔솖솜솝");

    // hangul syllables, row 17 col 1-28
    cs.register_kuten_range_str(17, 1,
        u8"솟송솥솨솩솬솰솽쇄쇈쇌쇔쇗쇘쇠쇤쇨쇰쇱쇳쇼쇽숀숄숌숍숏숑");

    // hangul syllables, row 17 col 30-94
    cs.register_kuten_range_str(17, 30,
        u8"수숙순숟술숨숩숫숭숯숱숲숴쉈쉐쉑쉔쉘쉠쉥쉬쉭쉰쉴쉼쉽쉿슁슈슉슐슘슛슝스슥슨슬슭슴습슷승시식신싣실싫심십싯싱싵싶ㅆ싸싹싻싼쌀쌈쌉쌌쌍");

    // hangul syllables, row 18
    cs.register_kuten_range_str(18, 1,
        u8"쌓쌔쌕쌘쌜쌤쌥쌨쌩썅써썩썬썰썲썸썹썼썽쎄쎈쎌쎼쏀쏘쏙쏜쏟쏠쏢쏨쏩쏭쏴쏵쏸쐈쐐쐤쐬쐰쐴쐼쐽쑈쑤쑥쑨쑬쑴쑵쑹쒀쒔쒜쒸쒼쓩쓰쓱쓴쓸쓺쓿씀씁씌씐씔씜씨씩씬씰씸씹씻씽ㅇ아악안앉않알앍앎앓암압앗았앙앝");

    // hangul syllables, row 19 col 1-28
    cs.register_kuten_range_str(19, 1,
        u8"앞애액앤앨앰앱앳앴앵야약얀얄얇얌얍얏양얕얗얘얜얠얩어억언");

    // hangul syllables, row 19 col 30-94
    cs.register_kuten_range_str(19, 30,
        u8"얹얻얼얽얾엄업없엇었엉엊엌엎에엑엔엘엠엡엣엥여역엮연열엶엷엹염엽엾엿였영옅옆옇예옌옐옘옙옛옜오옥온올옭옮옰옳옴옵옷옹옻와왁완왈왐왑");

    // hangul syllables, row 20
    cs.register_kuten_range_str(20, 1,
        u8"왓왔왕왜왝왠왬왯왱외왹왼욀욈욉욋욍요욕욘욜욤욥욧용우욱운울욹욺움웁웃웅워웍원월웜웝웠웡웨웩웬웰웸웹웽위윅윈윌윔윕윗윙유육윤율윰윱윳융윷으윽은을읊음읍읏응읒읓읔읕읖읗의읜읠읨읫이익인일읽읾잃");

    // hangul syllables, row 21 col 1-28
    cs.register_kuten_range_str(21, 1,
        u8"임입잇있잉잊잌잎ㅈ자작잔잖잗잘잚잠잡잣잤장잦잫재잭잰잴잼");

    // hangul syllables, row 21 col 30-94
    cs.register_kuten_range_str(21, 30,
        u8"잽잿쟀쟁쟈쟉쟌쟎쟐쟘쟝쟤쟨쟬저적전절젊젋점접젓정젖제젝젠젤젬젭젯젱져젹젼졀졈졉졌졍졔조족존졸졺좀좁좃종좆좇좋좌좍좔좝좟좡좨좼좽죄죈");

    // hangul syllables, row 22
    cs.register_kuten_range_str(22, 1,
        u8"죌죔죕죗죙죠죡죤죵주죽준줄줅줆줌줍줏중줘줬줴쥐쥑쥔쥘쥠쥡쥣쥬쥰쥴쥼즈즉즌즐즘즙즛증지직진짆짇질짊짐집짓징짖짙짚ㅉ짜짝짠짢짤짦짧짬짭짯짰짱째짹짼쨀쨈쨉쨋쨌쨍쨔쨘쨩쨰쩄쩌쩍쩐쩔쩜쩝쩟쩠쩡쩨쩽쪄");

    // hangul syllables, row 23 col 1-28
    cs.register_kuten_range_str(23, 1,
        u8"쪘쪼쪽쫀쫄쫌쫍쫎쫏쫑쫒쫓쫘쫙쫠쫬쫴쬈쬐쬔쬘쬠쬡쭁쭈쭉쭌쭐");

    // hangul syllables, row 23 col 30-94
    cs.register_kuten_range_str(23, 30,
        u8"쭘쭙쭝쭤쭸쭹쮜쮸쯔쯤쯧쯩찌찍찐찔찜찝찟찡찢찧ㅊ차착찬찮찰참찹찻찼창찾채책챈챌챔챕챗챘챙챠챤챦챨챰챵처척천철첨첩첫첬청체첵첸첼쳄쳅쳇");

    // hangul syllables, row 24
    cs.register_kuten_range_str(24, 1,
        u8"쳉쳐쳔쳤쳬쳰촁초촉촌촐촘촙촛총촤촥촨촬촹쵀최쵠쵤쵬쵭쵯쵱쵸춈추축춘출춤춥춧충춰췄췌췐취췬췰췸췹췻췽츄츈츌츔츙츠측츤츨츰츱츳층치칙친칟칠칡침칩칫칭ㅋ카칵칸칼캄캅캇캉캐캑캔캘캠캡캣캤캥캬캭컁커");

    // hangul syllables, row 25 col 1-28
    cs.register_kuten_range_str(25, 1,
        u8"컥컨컫컬컴컵컷컸컹케켁켄켈켐켑켓켕켜켠켤켬켭켯켰켱켸코콕");

    // hangul syllables, row 25 col 30-94
    cs.register_kuten_range_str(25, 30,
        u8"콘콜콤콥콧콩콰콱콴콸쾀쾅쾌쾍쾡쾨쾰쿄쿠쿡쿤쿨쿰쿱쿳쿵쿼퀀퀄퀑퀘퀭퀴퀵퀸퀼큄큅큇큉큐큔큘큠크큭큰클큼큽큿킁키킥킨킬킴킵킷킹ㅌ타탁탄탈");

    // hangul syllables, row 26
    cs.register_kuten_range_str(26, 1,
        u8"탉탐탑탓탔탕태택탠탤탬탭탯탰탱탸턍터턱턴털턺텀텁텃텄텅테텍텐텔템텝텟텡텨텬텼톄톈토톡톤톨톰톱톳통톺톼퇀퇘퇴퇸툇툉툐투툭툰툴툼툽툿퉁퉈퉜퉤퉷튀튁튄튈튐튑튕튜튠튤튬튱트특튼튿틀틂틈틉틋틑틔틘틜");

    // hangul syllables, row 27 col 1-28
    cs.register_kuten_range_str(27, 1,
        u8"틤틥티틱틴틸팀팁팃팅ㅍ파팍팎판팔팖팜팝팟팠팡팥패팩팬팰팸");

    // hangul syllables, row 27 col 30-94
    cs.register_kuten_range_str(27, 30,
        u8"팹팻팼팽퍄퍅퍼퍽펀펄펌펍펏펐펑페펙펜펠펨펩펫펭펴편펼폄폅폈평폐폘폡폣포폭폰폴폼폽폿퐁퐈퐝푀푄표푠푤푭푯푸푹푼푿풀풂품풉풋풍풔풩퓌퓐");

    // hangul syllables, row 28
    cs.register_kuten_range_str(28, 1,
        u8"퓔퓜퓟퓨퓬퓰퓸퓻퓽프픈플픔픕픗피픽핀필핌핍핏핑ㅎ하학한할핥함합핫핬항해핵핸핼햄햅햇했행햐향허헉헌헐헒험헙헛헝헤헥헨헬헴헵헷헹혀혁현혈혐협혓혔형혜혠혤혭호혹혼홀홅홈홉홋홍홑화확환활홧황홰홱홴");

    // hangul syllables, row 29 col 1-28
    cs.register_kuten_range_str(29, 1,
        u8"횃횅회획횐횔횝횟횡효횬횰횹횻후훅훈훌훑훔훗훙훠훤훨훰훵훼");

    // hangul syllables, row 29 col 30-94
    cs.register_kuten_range_str(29, 30,
        u8"훽휀휄휑휘휙휜휠휨휩휫휭휴휵휸휼흄흇흉흐흑흔흖흗흘흙흠흡흣흥흩흫희흰흴흼흽힁히힉힌힐힘힙힛힝");

    // distinguish space characters
    cs.register_kuten_range(33, 1, { U'\u2000' }); // en quad space
    cs.register_kuten_range(33, 2, { U'\u3000' }); // ideographic space

    // punctuation and symbols, row 33 col 3
    cs.register_kuten_range_str(33, 3,
        u8"、。·‥…¨〃\u2013\u2015∥＼∼\u2018\u2019\u201C\u201D〔〕〈〉《》「」『』");

    // jamo control codes and symbols, row 33 col 30
    // '□' => "이/가" => U+1141, '■' => "을/를" => U+111B
    // '△' => "아/야" => U+1199, '▲' => "은/는" => U+11AB
    // '▽' => "와/과" => U+116A, replaced '´' => '∵' (duplicate)
    cs.register_kuten_range_str(33, 30,
        u8"【】±×÷≠≤≥∞∴°′″℃Å￠￡￥♂♀∠⊥⌒∂∇≡≒§※☆★○●◎◇◆ᅁᄛᆙᆫᅪ▼→←↑↓↔〓≪≫√∽∝∵∫∬∈∋⊆⊇⊂⊃∪∩∧∨￢⇒⇔∀∃∵");

    // ASCII characters, row 39 col 1
    cs.register_kuten_range(39, 1, {
        U' ',
        U'!', U'"', U'#', U'$', U'%', U'&', U'\'', U'(', U')', U'*', U'+', U',', U'-', U'.', U'/',
        U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9',
        U':', U';',
    });

    // ASCII characters continued, row 39 col 30
    // replaced '￣' => '～'
    cs.register_kuten_range(39, 30, {
        U'<', U'=', U'>', U'?', U'@',
        U'A', U'B', U'C', U'D', U'E', U'F', U'G', U'H', U'I', U'J', U'K', U'L', U'M',
        U'N', U'O', U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W', U'X', U'Y', U'Z',
        U'[', U'\\', U']', U'^', U'_', U'`',
        U'a', U'b', U'c', U'd', U'e', U'f', U'g', U'h', U'i', U'j', U'k', U'l', U'm',
        U'n', U'o', U'p', U'q', U'r', U's', U't', U'u', U'v', U'w', U'x', U'y', U'z',
        U'{', U'|', U'}', U'\uFF5E',
    });

    cs.set_fontwidth(2);
    cs.set_space_char(U'\u3000');
    cs.set_newline_char(U'%');
}
